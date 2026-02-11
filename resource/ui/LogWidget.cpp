#include "T_LogWidget.h"

#include <ElaListView.h>

#include <QVBoxLayout>
#include "Resource/ui/BasePage.h"
#include "ElaLog.h"
#include "ModelView/T_LogModel.h"
LogWidget::LogWidget(QWidget* parent)
    : BasePage{parent}
{
    setTitleVisible(true);
    setContentsMargins(2, 2, 0, 0);
    setWindowTitle("日志监控");
    // createCustomWidget("")

    // 创建工具栏
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
    
    // 创建日志显示区域
    ElaListView* logView = new ElaListView(this);
    logView->setIsTransparent(true);
    _logModel = new T_LogModel(this);
    logView->setModel(_logModel);


    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(toolbarLayout);
    mainLayout->addWidget(logView);
    mainLayout->setContentsMargins(0, 5, 5, 0);

    connect(ElaLog::getInstance(), &ElaLog::logMessage, this, [=](QString log) {
        _logModel->appendLogList(log);
    });

}

LogWidget::~LogWidget()
{
    // get the global logger
    auto logger = spdlog::default_logger();

    // find QtTextEditSink and detach
    if (logger) {
        for (auto& sink : logger->sinks()) {
            auto qt_sink = std::dynamic_pointer_cast<QtTextEditSink_mt>(sink);
            if (qt_sink) {
                qt_sink->detach();
            }
        }
    }
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
