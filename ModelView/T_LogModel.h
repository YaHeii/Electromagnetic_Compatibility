#pragma once
#include <QAbstractListModel>

class T_LogModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit T_LogModel(QObject* parent = nullptr);
    ~T_LogModel();
    void setLogList(QStringList list);
    void appendLogList(QString log);
    QStringList getLogList() const;

protected:
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    // cuetom method
    void appendLog(const QString &message, spdlog::level::level_enum level);
    void clearLogs();
    void filterLogs(const QString &level);
private:
    QStringList _logList;
};

