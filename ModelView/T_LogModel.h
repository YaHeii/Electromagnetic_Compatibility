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
    void setPaused(bool paused) { _isPaused = paused; }
    bool isPaused() const { return _isPaused; }
public slots:
    void onNewLog(const QString& msg, int level);
signals:
    void statisticsChanged(int total, int error, int info);
protected:
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;

private:
    void updateStatistics(spdlog::level::level_enum level);
    bool isLevelMatch(spdlog::level::level_enum level, const QString& filter) const;
private:
    QList<LogEntry> _logEntries;       // 底层全量数据，最大限制 1000 条
    QList<LogEntry> _displayEntries;   // 经过 UI 过滤后真正用于展示的数据
    QString _currentFilterLevel;    // 当前的过滤等级
    // 状态与统计成员
    bool _isPaused{false};
    int _totalLogs{0};
    int _errorLogs{0};
    int _infoLogs{0};
};

