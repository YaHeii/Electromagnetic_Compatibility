#ifndef THREADSAFEQUEUE_H
#define THREADSAFEQUEUE_H

#include <queue>
#include <QMutex>
#include <QWaitCondition>

template<typename T>
class QUEUE_DATA {
public:
    QUEUE_DATA() {}

    void push(const T& data) {
        QMutexLocker locker(&m_mutex);
        m_queue.push(data);
        m_condition.wakeOne();
    }

    bool try_pop(T& value) {
        QMutexLocker locker(&m_mutex);
        if (m_queue.empty()) {
            return false;
        }
        value = m_queue.front();
        m_queue.pop();
        return true;
    }

    void wait_and_pop(T& value) {
        QMutexLocker locker(&m_mutex);
        while (m_queue.empty()) {
            m_condition.wait(&m_mutex);
        }
        value = m_queue.front();
        m_queue.pop();
    }

    bool empty() const {
        QMutexLocker locker(&m_mutex);
        return m_queue.empty();
    }

    int size() const {
        QMutexLocker locker(&m_mutex);
        return m_queue.size();
    }

private:
    mutable QMutex m_mutex;
    std::queue<T> m_queue;
    QWaitCondition m_condition;
};

#endif // THREADSAFEQUEUE_H
