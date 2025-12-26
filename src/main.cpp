#include "../include/mainwindow.h"
#include <QApplication>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// 引入 spdlog 头文件
#include "spdlog/spdlog.h"
#include "spdlog/async.h" // 异步日志必须
#include "spdlog/sinks/stdout_color_sinks.h" // 控制台输出
#include "spdlog/sinks/basic_file_sink.h"    // 文件输出

void init_logger() {
    try {
        // 1. 初始化线程池
        spdlog::init_thread_pool(8192, 1);

        // 2. 创建 Sinks
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/app_log.txt", true);

        std::vector<spdlog::sink_ptr> sinks{ console_sink, file_sink };

        // 3. 创建异步 Logger
        auto logger = std::make_shared<spdlog::async_logger>(
            "global_logger",
            sinks.begin(),
            sinks.end(),
            spdlog::thread_pool(),
            spdlog::async_overflow_policy::block
        );

        // 4. 注册为全局默认 Logger
        spdlog::set_default_logger(logger);

        // 5. 设置级别和刷新策略
        spdlog::set_level(spdlog::level::debug);
        spdlog::flush_every(std::chrono::seconds(3));

    }
    catch (const spdlog::spdlog_ex& ex) {
        fprintf(stderr, "Log initialization failed: %s\n", ex.what());
    }
}

// 定义一个 Qt 消息处理函数
void qt_message_handler(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
    QByteArray localMsg = msg.toLocal8Bit();

    // 根据 Qt 的日志级别映射到 spdlog 的级别
    switch (type) {
    case QtDebugMsg:
        spdlog::debug("[Qt] {}", localMsg.constData());
        break;
    case QtInfoMsg:
        spdlog::info("[Qt] {}", localMsg.constData());
        break;
    case QtWarningMsg:
        spdlog::warn("[Qt] {}", localMsg.constData());
        break;
    case QtCriticalMsg:
        spdlog::error("[Qt] {}", localMsg.constData());
        break;
    case QtFatalMsg:
        spdlog::critical("[Qt] {}", localMsg.constData());
        // Qt Fatal 默认会 abort，但在 spdlog 记录后可能需要手动处理或让其继续
        break;
    }
}

// 在 main 函数中，init_logger() 之后调用：

int main(int argc, char *argv[])
{
    init_logger();
    qInstallMessageHandler(qt_message_handler);
    spdlog::debug("Initializing");
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    spdlog::info("MainWindow shown.");
    int exit_code = a.exec();

    spdlog::info("Application exiting with code {}", exit_code);
    spdlog::shutdown();
    return exit_code;
}
