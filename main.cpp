#include "Resource/ui/mainwindow.h"
#include <QApplication>
#include <QDebug>
#include <Windows.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "spdlog/spdlog.h"
#include "spdlog/async.h" 
#include "spdlog/sinks/stdout_color_sinks.h" // 控制台输出
#include "spdlog/sinks/basic_file_sink.h"    // 文件输出

//Introduce ui
#include "ElaApplication.h"

using GridMap = std::vector<std::vector<double>>;
using Matrix = std::vector<std::vector<double>>;
using LineMap = std::vector<double>;
Q_DECLARE_METATYPE(GridMap)
Q_DECLARE_METATYPE(LineMap)

void init_logger(MainWindow& w) {
    try {
//1. Initialize the thread pool
        spdlog::init_thread_pool(8192, 1);

//2. Create Sinks
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/app_log.txt", true);
        auto gui_sink = w.createGuiLogSink();//Get the GUI sink from MainWindow

        std::vector<spdlog::sink_ptr> sinks{ console_sink, file_sink, gui_sink };

//3. Create an asynchronous Logger
        auto logger = std::make_shared<spdlog::async_logger>(
            "global_logger",
            sinks.begin(),
            sinks.end(),
            spdlog::thread_pool(),
            spdlog::async_overflow_policy::block
        );

//4. Register as global default Logger
        spdlog::set_default_logger(logger);

//5. Set level and refresh strategy
        spdlog::set_level(spdlog::level::debug);
        spdlog::flush_every(std::chrono::seconds(3));

    }
    catch (const spdlog::spdlog_ex& ex) {
        fprintf(stderr, "Log initialization failed: %s", ex.what());
    }
}

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
        break;
    }
}
#include <QApplication>
#include <QDirIterator>
#include <QDebug>
int main(int argc, char *argv[])
{
    //// --- 调试代码开始 ---
    //qDebug() << "--------- 已加载资源列表 ---------";
    //// 递归遍历资源根目录 ":"
    //QDirIterator it(":", QDirIterator::Subdirectories);
    //while (it.hasNext()) {
    //    QString resourcePath = it.next();
    //    // 过滤掉 Qt 内部资源，只看你自己的资源
    //    if (!resourcePath.startsWith(":/qt-project.org")) {
    //        qDebug() << resourcePath;
    //    }
    //}
    //qDebug() << "---------------------------------";
    //// --- 调试代码结束 ---
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
//initial size
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QApplication a(argc, argv);
    //初始化控件
    eApp->init();
//Initialize custom delivery signal
    qRegisterMetaType<GridMap>("GridMap");
    qRegisterMetaType<Matrix>("Matrix");
    qRegisterMetaType<LineMap>("LineMap");
    MainWindow w;

//Initialize the log system
    init_logger(w);
    qInstallMessageHandler(qt_message_handler);
    
    spdlog::debug("正在初始化....");
    w.show();

    spdlog::info("MainWindow shown.");
    int exit_code = a.exec();

    spdlog::info("Application exiting with code {}", exit_code);
    spdlog::shutdown();
    return exit_code;
}
