#pragma once
#include <QAbstractListModel>

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
    void setLogList(QStringList list);
    void appendLogList(QString log);
    QStringList getLogList() const;
    // custom method
    void appendLogList(const QString &message, spdlog::level::level_enum level);
    void clearLogList();
    void filterLogList(const QString &level);
protected:
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;

private:
    // QStringList _logList;
    QList<LogEntry> logEntries;
};

