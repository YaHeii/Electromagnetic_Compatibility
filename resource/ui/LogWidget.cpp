#include "LogWidget.h"
#include "ui_LogWidget.h"
#include <iostream>

LogWidget::LogWidget(QWidget *parent)
  :QWidget(parent),ui(new Ui::LogWidget)
{
  ui->setupUi(this);
  // 设置日志控件最大行数
  ui->Debug_Edit->setMaximumBlockCount(5000);
  ui->Error_Edit->setMaximumBlockCount(5000);

  // 初始化日志发射器并连接信号
  _logEmitter = new LogEmitter(this);
  connect(_logEmitter, &LogEmitter::newLog, this, &LogWidget::onLogReceived);
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
    delete ui;
}

std::shared_ptr<spdlog::sinks::sink> LogWidget::createGuiLogSink() {
    // 创建并配置 UI Sink
    auto qt_sink = std::make_shared<QtTextEditSink_mt>(_logEmitter);
    qt_sink->set_pattern("[%H:%M:%S.%e] %v");
    return qt_sink;
}

void LogWidget::onLogReceived(const QString& message, int level)
{
    auto logLevel = static_cast<spdlog::level::level_enum>(level);
    std::cout << "UI Trace: Slot onLogReceived called. Msg: " << message.toStdString() << std::endl;
    // 1. 设置最大行数 (防止日志无限增长占满内存)
    // 2. 根据级别输出
    if (logLevel == spdlog::level::err || logLevel == spdlog::level::critical) {
        // Error 级别 -> 红色高亮
        // QPlainTextEdit 支持 appendHtml 来显示颜色
        ui->Error_Edit->appendHtml(QString("<font color='#FF0000'>%1</font>").arg(message));
    }
    else if (logLevel == spdlog::level::debug) {
        // Debug 级别 -> 纯文本 (性能最高)
        ui->Debug_Edit->appendPlainText(message);
        // Info 级别 -> 绿色高亮
        //ui->Debug_Edit->appendHtml(QString("<font color='green'>%1</font>").arg(message));
    }
    else if (logLevel == spdlog::level::info) {
        // Info 级别 -> 绿色高亮
        ui->Debug_Edit->appendHtml(QString("<font color='green'>%1</font>").arg(message));
    }
}
