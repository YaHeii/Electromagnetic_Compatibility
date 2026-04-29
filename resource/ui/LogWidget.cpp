#include "LogWidget.h"
#include <ElaListView.h>
#include <QDesktopServices>
#include <QUrl>
#include <QCoreApplication>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include "ElaText.h"
#include "Resource/ui/BasePage.h"
#include "ModelView/T_LogModel.h"
#include "ElaMessageBar.h"
LogWidget::LogWidget(QWidget* parent)
    : BasePage{parent}
{
    setContentsMargins(2, 2, 0, 0);
   // 创建工具栏
    toolbarWidget = new QWidget(this);
    QHBoxLayout* toolbarLayout = new QHBoxLayout(toolbarWidget);
    toolbarLayout->setContentsMargins(0, 0, 0, 0);
    toolbarLayout->setSpacing(8);
    // 日志级别过滤
    ElaText *levelLabel = new ElaText("级别:", this);
    levelLabel->setTextPixelSize(12);
    levelLabel->setFixedWidth(40);
    
    levelFilter = new ElaComboBox(this);
    levelFilter->addItem("信息");
    levelFilter->addItem("调试");
    levelFilter->addItem("错误");
    levelFilter->addItem("警告");

    levelFilter->setFixedWidth(80);
    levelFilter->setFixedHeight(30);
    connect(levelFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &LogWidget::onLevelFilterChanged);
    levelFilter->setCurrentIndex(0);
    
    // 清空按钮
    clearButton = new ElaToolButton(this);
    clearButton->setFixedSize(30, 30);
    clearButton->setElaIcon(ElaIconType::Trash);
    clearButton->setIsTransparent(false);
    connect(clearButton, &ElaToolButton::clicked, this, &LogWidget::on_clearbtn_clicked);
    
    // 暂停按钮
    pauseButton = new ElaToolButton(this);
    pauseButton->setFixedSize(30, 30);
    pauseButton->setElaIcon(ElaIconType::Pause);
    pauseButton->setIsTransparent(false);
    connect(pauseButton, &ElaToolButton::clicked, this, &LogWidget::on_pauseBtn_clicked);
    
    // 导出按钮
    openFileButton = new ElaToolButton(this);
    openFileButton->setFixedSize(30, 30);
    openFileButton->setElaIcon(ElaIconType::Download);
    openFileButton->setIsTransparent(false);
    connect(openFileButton, &ElaToolButton::clicked, this, &LogWidget::on_openFileBtn_clicked);
    
    toolbarLayout->addWidget(levelLabel);
    toolbarLayout->addWidget(levelFilter);
    toolbarLayout->addStretch();
    toolbarLayout->addWidget(clearButton);
    toolbarLayout->addWidget(pauseButton);
    toolbarLayout->addWidget(openFileButton);
    
    // 创建日志显示区域
    logListView = new ElaListView(this);
    logListView->setIsTransparent(true);

    _logModel = new T_LogModel(this);
    logListView->setModel(_logModel);
    _logEmitter = new LogEmitter(this);
    connect(_logEmitter, &LogEmitter::newLog,
                _logModel, &T_LogModel::onNewLog, 
                Qt::QueuedConnection);
    connect(_logModel, &T_LogModel::statisticsChanged, this, [this](int total, int error, int info){
    });
    connect(_logModel, &QAbstractItemModel::rowsInserted, this, [this]() {
        logListView->scrollToBottom();
    });
    QWidget* centralWidget = new QWidget(this);
    centralWidget->setWindowTitle("日志监控");
    QVBoxLayout* centerVLayout = new QVBoxLayout(centralWidget);
    centerVLayout->setContentsMargins(0, 0, 0, 0);
    centerVLayout->addWidget(toolbarWidget);
    centerVLayout->addWidget(logListView, 1);
    // centerVLayout->addStretch();
    addCentralWidget(centralWidget);

    onLevelFilterChanged();
}

LogWidget::~LogWidget()
{
    auto logger = spdlog::default_logger();

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
    qt_sink->set_pattern("[%H:%M:%S.%e] [%l] %v"); // 确保带有级别等前缀给Model去解析
    return qt_sink;
}



void LogWidget::onLevelFilterChanged()
{
   QString text = levelFilter->currentText();
    QString filterStr = "info"; // 默认

    // 将中文映射为 T_LogModel 能够识别的 spdlog internal level string
    if (text == "错误") {
        filterStr = "err";
    } else if (text == "警告") {
        filterStr = "warn";
    } else if (text == "信息") {
        filterStr = "info";
    } else if (text == "调试") {
        filterStr = "debug";
    }

    _logModel->filterLogList(filterStr);
}

void LogWidget::on_clearbtn_clicked()
{
    _logModel->clearLogList();
    ElaMessageBar::success(ElaMessageBarType::BottomRight, "成功", "日志已清空", 1500);
}

void LogWidget::on_pauseBtn_clicked()
{
    bool isPaused = !_logModel->isPaused();
    _logModel->setPaused(isPaused);
    
    if (isPaused) {
        pauseButton->setElaIcon(ElaIconType::Play);
        ElaMessageBar::warning(ElaMessageBarType::BottomLeft, "暂停", "日志接收已暂停", 1500);
    } else {
        pauseButton->setElaIcon(ElaIconType::Pause);
        ElaMessageBar::warning(ElaMessageBarType::BottomLeft, "恢复", "日志接收已恢复", 1500);
    }
}

void LogWidget::on_openFileBtn_clicked()
{
QString logFilePath = QCoreApplication::applicationDirPath() + "/logs/app_log.txt";
    
    // 使用本地桌面服务打开文件
    if (QDesktopServices::openUrl(QUrl::fromLocalFile(logFilePath))) {
        ElaMessageBar::success(ElaMessageBarType::BottomRight, "成功", "已调用系统程序打开日志", 1500);
    } else {
        ElaMessageBar::error(ElaMessageBarType::BottomRight, "失败", "无法打开日志文件，请确认文件是否存在", 2000);
    }
}
