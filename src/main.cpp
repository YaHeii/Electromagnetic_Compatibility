#include "../include/mainwindow.h"
#include <QApplication>
#include <QDebug>
#include <Windows.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// 引入 spdlog 头文件
#include "spdlog/spdlog.h"
#include "spdlog/async.h" // 异步日志必须
#include "spdlog/sinks/stdout_color_sinks.h" // 控制台输出
#include "spdlog/sinks/basic_file_sink.h"    // 文件输出


using GridMap = std::vector<std::vector<double>>;
using Matrix = std::vector<std::vector<double>>;
using LineMap = std::vector<double>;
// 注册自定义类型以便在 Qt 信号槽中使用
Q_DECLARE_METATYPE(GridMap)
Q_DECLARE_METATYPE(LineMap)

// 将 MainWindow 作为参数，以便从中获取 GUI sink
void init_logger(MainWindow& w) {
    try {
        // 1. 初始化线程池
        spdlog::init_thread_pool(8192, 1);

        // 2. 创建 Sinks
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/app_log.txt", true);
        auto gui_sink = w.createGuiLogSink(); // 从 MainWindow 获取 GUI sink

        std::vector<spdlog::sink_ptr> sinks{ console_sink, file_sink, gui_sink };

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
        fprintf(stderr, "Log initialization failed: %s", ex.what());
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
        break;
    }
}

int main(int argc, char *argv[])
{
    SetConsoleOutputCP(65001);
    
    QApplication a(argc, argv);


    //TODO：使用QSS替代Fusion
    a.setStyle(QStyleFactory::create("Fusion"));

    // 2. 初始化调色板
    QPalette lightPalette;

    // --- 背景色设置 ---
    // Window: 窗口的一般背景色 (设为极浅的灰色，比纯白更有质感)
    lightPalette.setColor(QPalette::Window, QColor(240, 240, 240));
    // WindowText: 窗口上的文字颜色
    lightPalette.setColor(QPalette::WindowText, Qt::black);

    // --- 输入控件背景色 (QLineEdit, QTextEdit 等) ---
    // Base: 主要用于输入框的背景 (通常设为纯白)
    lightPalette.setColor(QPalette::Base, Qt::white);
    // AlternateBase: 用于列表控件(QTableWidget)的交替行背景
    lightPalette.setColor(QPalette::AlternateBase, QColor(233, 233, 233));
    // Text: 输入框内的文字颜色
    lightPalette.setColor(QPalette::Text, Qt::black);
    // ToolTipBase: 提示框背景
    lightPalette.setColor(QPalette::ToolTipBase, Qt::white);
    lightPalette.setColor(QPalette::ToolTipText, Qt::black);

    // --- 按钮设置 ---
    // Button: 按钮背景色 (通常与 Window 颜色一致或稍深)
    lightPalette.setColor(QPalette::Button, QColor(240, 240, 240));
    lightPalette.setColor(QPalette::ButtonText, Qt::black);
    
    // --- 禁用状态 (Disabled) ---
    // 设置禁用状态下的文字颜色为灰色，体现"不可点"的感觉
    lightPalette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(120, 120, 120));
    lightPalette.setColor(QPalette::Disabled, QPalette::Text, QColor(120, 120, 120));
    lightPalette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(120, 120, 120));

    // --- 高亮/选中状态 (Highlight) ---
    // Link: 超链接颜色 (类似于浏览器蓝)
    lightPalette.setColor(QPalette::Link, QColor(0, 122, 204));
    // Highlight: 选中项目时的背景色 (经典蓝)
    lightPalette.setColor(QPalette::Highlight, QColor(0, 120, 215));
    // HighlightedText: 选中项目时的文字颜色 (通常为白)
    lightPalette.setColor(QPalette::HighlightedText, Qt::white);

    // 3. 应用调色板
    a.setPalette(lightPalette);

    qRegisterMetaType<GridMap>("GridMap");
    qRegisterMetaType<Matrix>("Matrix");
    qRegisterMetaType<LineMap>("LineMap");
    MainWindow w;

    // 在创建 MainWindow 之后，初始化日志系统
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
