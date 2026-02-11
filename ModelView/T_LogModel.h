#pragma once
#include <QAbstractListModel>
#include <QColor>
#include "spdlog/spdlog.h"

class T_LogModel : public QAbstractListModel
{
    Q_OBJECT
public:
    struct LogEntry {
        QString message;
        spdlog::level::level_enum level;
        QColor color;
    };
public:
    explicit T_LogModel(QObject* parent = nullptr);
    ~T_LogModel();
    void setLogList(QList<LogEntry> logEntries);
    QList<LogEntry> getLogList() const;
    void appendLogList(LogEntry log);
    void appendLogList(const QString &message, spdlog::level::level_enum level);
    void clearLogList();
    void filterLogList(const QString &level);
protected:
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;

private:
    // QStringList _logList;
    QList<LogEntry> _logEntries;
};

