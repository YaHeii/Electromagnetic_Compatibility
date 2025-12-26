#ifndef LOG_GLOBAL_H
#define LOG_GLOBAL_H

#include <QString>

enum LogLevel {
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_ERROR
};

struct Log {
    QString message;
    LogLevel level;
};

#endif // LOG_GLOBAL_H
