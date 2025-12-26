#ifndef QTSPDLOGSINK_H
#define QTSPDLOGSINK_H

#include <QObject>
#include <QString>
#include <mutex>
#include "spdlog/sinks/base_sink.h"
#include "spdlog/details/null_mutex.h"

class LogEmitter : public QObject {
    Q_OBJECT
public:
    explicit LogEmitter(QObject* parent = nullptr) : QObject(parent) {}
    // 发送日志的信号：包含日志内容和日志等级
    void emitLog(const QString& msg, int level) {
        emit newLog(msg, level);
    }

signals:
    void newLog(const QString& msg, int level);
};

// 2. 定义 spdlog 的自定义 Sink
// 继承自 base_sink，负责拦截 spdlog 的日志
template<typename Mutex>
class QtTextEditSink : public spdlog::sinks::base_sink<Mutex> {
private:
    LogEmitter* emitter_; // 持有发射器的指针

public:
    explicit QtTextEditSink(LogEmitter* emitter) : emitter_(emitter) {}
    void detach() {
        std::lock_guard<Mutex> lock(this->mutex_);
        emitter_ = nullptr;
    }
protected:
    // spdlog 写入日志时会调用这个函数
    void sink_it_(const spdlog::details::log_msg& msg) override {
        if (!emitter_) return;
        std::cout << "Sink Trace: sink_it_ called, emitting signal..." << std::endl;
        // 1. 格式化日志消息 (将 spdlog 的 buffer 转为 string)
        spdlog::memory_buf_t formatted;
        this->formatter_->format(msg, formatted);

        // 2. 转换为 QString (移除末尾可能的换行符，因为 append 会自动加)
        QString qMsg = QString::fromUtf8(formatted.data(), static_cast<int>(formatted.size())).trimmed();

        // 3. 通过信号发送出去 (int 转换是为了跨线程安全，避免注册自定义类型)
        emitter_->emitLog(qMsg, static_cast<int>(msg.level));
    }

    void flush_() override {}
};

//以此方便创建线程安全的 Sink
using QtTextEditSink_mt = QtTextEditSink<std::mutex>;



#endif // QTSPDLOGSINK_H