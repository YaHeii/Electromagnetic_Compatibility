#include "LogWidget.h"
#include <iostream>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>

#include "ElaText.h"
#include "ElaListView.h"
#include "ElaLog.h"
#include "ElaTheme.h"
#include "ElaMessageBar.h"
#include "ElaScrollBar.h"
#include "ElaToolButton.h"
#include "ElaComboBox.h"

LogWidget::LogWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    
    // 初始化日志发射器并连接信号
    _logEmitter = new LogEmitter(this);
    connect(_logEmitter, &LogEmitter::newLog, this, &LogWidget::onLogReceived);
    
    // 主题切换支持
    connect(eTheme, &ElaTheme::themeModeChanged, this, [=]() {
        update();
    });
    
    // 初始化提示
    // ElaMessageBar::info(ElaMessageBarType::BottomRight, "日志系统", "日志组件已初始化", 2000);
}

LogWidget::~LogWidget()
{
    // 获取 logger
    auto logger = spdlog::default_logger();

    // 遍历所有 sink，找到我们的 QtTextEditSink 并解绑
    if (logger) {
        for (auto& sink : logger->sinks()) {
            auto qt_sink = std::dynamic_pointer_cast<QtTextEditSink_mt>(sink);
            if (qt_sink) {
                qt_sink->detach();
            }
        }
    }
}

void LogWidget::setupUI()
{
    // 设置窗口属性
    setWindowTitle("日志监控");
    setMinimumSize(400, 300);
    
    // 创建主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(8);
    
    // 创建标题区域
    setupTitleWidget();
    
    // 创建工具栏
    setupToolbarWidget();
    
    // 创建日志显示区域
    setupLogDisplayWidget();
    
    // 添加到主布局
    mainLayout->addWidget(titleWidget);
    mainLayout->addWidget(toolbarWidget);
    mainLayout->addWidget(logDisplayWidget);
}

void LogWidget::setupTitleWidget()
{
    titleWidget = new QWidget(this);
    titleWidget->setFixedHeight(50);
    
    QHBoxLayout *titleLayout = new QHBoxLayout(titleWidget);
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->setSpacing(10);
    
    // 标题文本
    ElaText *titleText = new ElaText("系统日志", this);
    titleText->setTextPixelSize(18);
    // titleText->setTextBold(true);
    
    // 状态指示器
    ElaText *statusText = new ElaText("运行中", this);
    statusText->setTextPixelSize(12);
    // statusText->setTextColor(QColor(0, 200, 0));
    
    titleLayout->addWidget(titleText);
    titleLayout->addStretch();
    titleLayout->addWidget(statusText);
}

void LogWidget::setupToolbarWidget()
{
    toolbarWidget = new QWidget(this);
    toolbarWidget->setFixedHeight(40);
    
    QHBoxLayout *toolbarLayout = new QHBoxLayout(toolbarWidget);
    toolbarLayout->setContentsMargins(0, 0, 0, 0);
    toolbarLayout->setSpacing(8);
    
    // 日志级别过滤
    ElaText *levelLabel = new ElaText("级别:", this);
    levelLabel->setTextPixelSize(12);
    levelLabel->setFixedWidth(40);
    
    levelFilter = new ElaComboBox(this);
    levelFilter->addItem("全部");
    levelFilter->addItem("错误");
    levelFilter->addItem("警告");
    levelFilter->addItem("信息");
    levelFilter->addItem("调试");
    levelFilter->setFixedWidth(80);
    levelFilter->setFixedHeight(30);
    connect(levelFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &LogWidget::onLevelFilterChanged);
    
    // 清空按钮
    ElaToolButton *clearButton = new ElaToolButton(this);
    clearButton->setFixedSize(30, 30);
    clearButton->setElaIcon(ElaIconType::Trash);
    clearButton->setIsTransparent(false);
    connect(clearButton, &ElaToolButton::clicked, this, &LogWidget::clearLogs);
    
    // 暂停按钮
    pauseButton = new ElaToolButton(this);
    pauseButton->setFixedSize(30, 30);
    pauseButton->setElaIcon(ElaIconType::Pause);
    pauseButton->setIsTransparent(false);
    connect(pauseButton, &ElaToolButton::clicked, this, &LogWidget::togglePause);
    
    // 导出按钮
    ElaToolButton *exportButton = new ElaToolButton(this);
    exportButton->setFixedSize(30, 30);
    exportButton->setElaIcon(ElaIconType::Download);
    exportButton->setIsTransparent(false);
    connect(exportButton, &ElaToolButton::clicked, this, &LogWidget::exportLogs);
    
    toolbarLayout->addWidget(levelLabel);
    toolbarLayout->addWidget(levelFilter);
    toolbarLayout->addStretch();
    toolbarLayout->addWidget(clearButton);
    toolbarLayout->addWidget(pauseButton);
    toolbarLayout->addWidget(exportButton);
}

void LogWidget::setupLogDisplayWidget()
{
    logDisplayWidget = new QWidget(this);
    
    QVBoxLayout *displayLayout = new QVBoxLayout(logDisplayWidget);
    displayLayout->setContentsMargins(0, 0, 0, 0);
    displayLayout->setSpacing(5);
    
    // 使用ElaListView显示日志 - 仿照T_LogWidget示例
    logListView = new ElaListView(this);
    logListView->setIsTransparent(true);
    logListView->setFixedHeight(200);
    
    // 创建自定义日志模型
    _logModel = new LogListModel(this);
    logListView->setModel(_logModel);
    
    // 添加自定义滚动条
    ElaScrollBar *listViewFloatScrollBar = new ElaScrollBar(logListView->verticalScrollBar(), logListView);
    listViewFloatScrollBar->setIsAnimation(true);
    
    // 统计信息
    QHBoxLayout *statsLayout = new QHBoxLayout();
    statsLayout->setContentsMargins(5, 5, 5, 5);
    
    ElaText *totalLabel = new ElaText("总计: 0", this);
    totalLabel->setTextPixelSize(11);
    //totalLabel->setTextColor(QColor(128, 128, 128));
    
    ElaText *errorLabel = new ElaText("错误: 0", this);
    errorLabel->setTextPixelSize(11);
    //errorLabel->setTextColor(QColor(255, 0, 0));
    
    ElaText *infoLabel = new ElaText("信息: 0", this);
    infoLabel->setTextPixelSize(11);
    //infoLabel->setTextColor(QColor(0, 128, 0));
    
    statsLayout->addWidget(totalLabel);
    statsLayout->addSpacing(20);
    statsLayout->addWidget(errorLabel);
    statsLayout->addSpacing(20);
    statsLayout->addWidget(infoLabel);
    statsLayout->addStretch();
    
    displayLayout->addWidget(logListView);
    displayLayout->addLayout(statsLayout);
    
    // 保存统计标签引用
    this->totalLabel = totalLabel;
    this->errorLabel = errorLabel;
    this->infoLabel = infoLabel;
}

std::shared_ptr<spdlog::sinks::sink> LogWidget::createGuiLogSink() {
    // 创建并配置 UI Sink
    auto qt_sink = std::make_shared<QtTextEditSink_mt>(_logEmitter);
    qt_sink->set_pattern("[%H:%M:%S.%e] %v");
    return qt_sink;
}

void LogWidget::onLogReceived(const QString& message, int level)
{
    if (isPaused) {
        return; // 如果暂停，不处理新日志
    }
    
    auto logLevel = static_cast<spdlog::level::level_enum>(level);
    std::cout << "UI Trace: Slot onLogReceived called. Msg: " << message.toStdString() << std::endl;
    
    // 更新统计信息
    updateStatistics(logLevel);
    
    // 根据级别过滤和显示
    if (shouldDisplayLog(logLevel)) {
        _logModel->appendLog(message, logLevel);
    }
}

void LogWidget::updateStatistics(spdlog::level::level_enum level)
{
    totalLogs++;
    
    if (level == spdlog::level::err || level == spdlog::level::critical) {
        errorLogs++;
    } else if (level == spdlog::level::info) {
        infoLogs++;
    }
    
    // 更新显示
    totalLabel->setText(QString("总计: %1").arg(totalLogs));
    errorLabel->setText(QString("错误: %1").arg(errorLogs));
    infoLabel->setText(QString("信息: %1").arg(infoLogs));
}

bool LogWidget::shouldDisplayLog(spdlog::level::level_enum level)
{
    QString filter = levelFilter->currentText();
    
    if (filter == "全部") {
        return true;
    } else if (filter == "错误") {
        return level == spdlog::level::err || level == spdlog::level::critical;
    } else if (filter == "警告") {
        return level == spdlog::level::warn;
    } else if (filter == "信息") {
        return level == spdlog::level::info;
    } else if (filter == "调试") {
        return level == spdlog::level::debug;
    }
    
    return true;
}

void LogWidget::onLevelFilterChanged()
{
    // 重新过滤现有日志
    _logModel->filterLogs(levelFilter->currentText());
}

void LogWidget::clearLogs()
{
    _logModel->clearLogs();
    totalLogs = 0;
    errorLogs = 0;
    infoLogs = 0;
    updateStatistics(spdlog::level::off);
    
    ElaMessageBar::success(ElaMessageBarType::BottomRight, "成功", "日志已清空", 1500);
}

void LogWidget::togglePause()
{
    isPaused = !isPaused;
    
    if (isPaused) {
        pauseButton->setElaIcon(ElaIconType::Play);
        //ElaMessageBar::info(ElaMessageBarType::BottomRight, "暂停", "日志接收已暂停", 1500);
    } else {
        pauseButton->setElaIcon(ElaIconType::Pause);
        //ElaMessageBar::info(ElaMessageBarType::BottomRight, "恢复", "日志接收已恢复", 1500);
    }
}

void LogWidget::exportLogs()
{
    // TODO: 实现日志导出功能
    //ElaMessageBar::info(ElaMessageBarType::BottomRight, "导出", "日志导出功能待实现", 2000);
}

// LogListModel 实现
LogListModel::LogListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int LogListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return logEntries.count();
}

QVariant LogListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= logEntries.count()) {
        return QVariant();
    }
    
    const LogEntry &entry = logEntries.at(index.row());
    
    switch (role) {
    case Qt::DisplayRole:
        return entry.message;
    case Qt::ForegroundRole:
        return entry.color;
    case Qt::FontRole:
        return entry.font;
    default:
        return QVariant();
    }
}

void LogListModel::appendLog(const QString &message, spdlog::level::level_enum level)
{
    beginInsertRows(QModelIndex(), logEntries.count(), logEntries.count());
    
    LogEntry entry;
    entry.message = message;
    entry.level = level;
    
    // 设置颜色
    QColor color;
    QFont font;
    font.setFamily("Consolas");
    font.setPixelSize(11);
    
    switch (level) {
    case spdlog::level::err:
    case spdlog::level::critical:
        color = QColor(255, 0, 0);
        break;
    case spdlog::level::warn:
        color = QColor(255, 165, 0);
        break;
    case spdlog::level::info:
        color = QColor(0, 128, 0);
        break;
    case spdlog::level::debug:
        color = QColor(128, 128, 128);
        break;
    default:
        color = QColor(0, 0, 0);
        break;
    }
    
    entry.color = color;
    entry.font = font;
    
    logEntries.append(entry);
    
    // 限制最大条目数
    if (logEntries.count() > 1000) {
        beginRemoveRows(QModelIndex(), 0, 0);
        logEntries.removeFirst();
        endRemoveRows();
    }
    
    endInsertRows();
}

void LogListModel::clearLogs()
{
    beginResetModel();
    logEntries.clear();
    endResetModel();
}

void LogListModel::filterLogs(const QString &level)
{
    // TODO: 实现日志过滤
    Q_UNUSED(level);
}