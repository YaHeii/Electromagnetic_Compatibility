#ifndef LOGQUEUE_H
#define LOGQUEUE_H
#include <QMutex>
#include <QThread>
#include <QDateTime>
#include <QString>
#include <QDebug>
// #include <memory>
// #include <queue>
// #include "netheader.h"
#include "ThreadSafeQueue.h"
#include "log_global.h"

//TODO:LogQueue不直接继承QTread，只负责管理和创建一个QTread实例和LogWorker
class LogWorker;

class LogQueue : public QObject
{
    Q_OBJECT
public:
    static LogQueue& GetInstance();
    ~LogQueue();

    void log(LogLevel level, const char* file, const char* func, int line, const char* fmt, ...);

signals:
    void newLogMessage(const QString& message, LogLevel level);

private:
    explicit LogQueue(QObject *parent = nullptr);
    LogQueue(const LogQueue&) = delete;
    LogQueue& operator=(const LogQueue&) = delete;

    void init();

    QThread* m_workerThread;
    LogWorker* m_worker;
};

#define LOG_ERROR(...) LogQueue::GetInstance().log(LOG_LEVEL_ERROR, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define LOG_DEBUG(...) LogQueue::GetInstance().log(LOG_LEVEL_DEBUG, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#endif // LOGQUEUE_H
