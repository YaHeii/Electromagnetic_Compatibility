#include "../../include/utils/LogQueue.hpp"
#include <QDateTime>
#include <QDebug>
#include <cstdarg>

class LogWorker : public QObject
{
    Q_OBJECT
public:
    explicit LogWorker(QObject *parent = nullptr) : QObject(parent), m_isCanRun(true) {}

public slots:
    void doWork() {
        while (m_isCanRun) {
            Log log;
            m_logQueue.wait_and_pop(log);
            if (!m_isCanRun) break;

            emit newLogMessage(log.message, log.level);
        }
    }

    void stop() {
        m_isCanRun = false;
        // Add a dummy log to wake up the worker thread if it's waiting
        Log dummy;
        m_logQueue.push(dummy);
    }

signals:
    void newLogMessage(const QString& message, LogLevel level);

private:
    volatile bool m_isCanRun;
    QUEUE_DATA<Log> m_logQueue;

public:
    void addLog(const Log& log) {
        m_logQueue.push(log);
    }
};

LogQueue& LogQueue::GetInstance() {
    static LogQueue instance;
    return instance;
}

LogQueue::LogQueue(QObject *parent) : QObject(parent) {
    init();
}

LogQueue::~LogQueue() {
    m_worker->stop();
    m_workerThread->quit();
    m_workerThread->wait();
    delete m_workerThread;
    delete m_worker;
}

void LogQueue::init() {
    m_workerThread = new QThread();
    m_worker = new LogWorker();
    m_worker->moveToThread(m_workerThread);

    connect(m_workerThread, &QThread::started, m_worker, &LogWorker::doWork);
    connect(m_worker, &LogWorker::newLogMessage, this, &LogQueue::newLogMessage);
    
    m_workerThread->start();
}

void LogQueue::log(LogLevel level, const char* file, const char* func, int line, const char* fmt, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    QString message = QString("[%1] [%2:%3:%4] %5")
                          .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz"))
                          .arg(file)
                          .arg(func)
                          .arg(line)
                          .arg(buffer);

    Log log_entry;
    log_entry.message = message;
    log_entry.level = level;
    m_worker->addLog(log_entry);
}

#include "LogQueue.moc"
