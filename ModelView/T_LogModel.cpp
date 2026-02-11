#include "T_LogModel.h"

T_LogModel::T_LogModel(QObject* parent)
    : QAbstractListModel{parent}
{
}

T_LogModel::~T_LogModel()
{
}

int T_LogModel::rowCount(const QModelIndex& parent) const
{
    // return this->_logList.count();
        Q_UNUSED(parent);
    return _logEntries.count();
}

QVariant T_LogModel::data(const QModelIndex& index, int role) const
{
    // if (role == Qt::DisplayRole)
    // {
    //     return _logList[index.row()];
    // }
    // return QVariant();

        if (!index.isValid() || index.row() >= _logEntries.count()) {
        return QVariant();
    }
    
    const LogEntry &entry = _logEntries.at(index.row());
    
    switch (role) {
    case Qt::DisplayRole:
        return entry.message;
    case Qt::ForegroundRole:
        return entry.color;
    default:
        return QVariant();
    }
}

void T_LogModel::setLogList(QList<LogEntry> logEntries)
{
    beginResetModel();
    this->_logEntries = logEntries;
    endResetModel();
}

void T_LogModel::appendLogList(LogEntry log)
{
    beginResetModel();
    this->_logEntries.push_back(log);
    endResetModel();
}

void T_LogModel::appendLogList(const QString &message, spdlog::level::level_enum level)
{
    beginInsertRows(QModelIndex(), _logEntries.count(), _logEntries.count());
    
    LogEntry entry;
    entry.message = message;
    entry.level = level;
    
    // 设置颜色
    QColor color;
    
    switch (level) {
    case spdlog::level::err:
    case spdlog::level::critical:
        color = QColor(255, 0, 0);
        break;
    case spdlog::level::warn:
        color = QColor(255, 165, 0);
        break;
    case spdlog::level::info:
        color = QColor(0, 128, 0);
        break;
    case spdlog::level::debug:
        color = QColor(128, 128, 128);
        break;
    default:
        color = QColor(0, 0, 0);
        break;
    }
    
    entry.color = color;
    
    _logEntries.append(entry);
    
    // 限制最大条目数
    if (_logEntries.count() > 1000) {
        beginRemoveRows(QModelIndex(), 0, 0);
        _logEntries.removeFirst();
        endRemoveRows();
    }
    
    endInsertRows();
}

QList<T_LogModel::LogEntry> T_LogModel::getLogList() const
{
    return this->_logEntries;
}

void T_LogModel::clearLogList()
{
    beginResetModel();
    _logEntries.clear();
    endResetModel();
}

void T_LogModel::filterLogList(const QString &level)
{
    // TODO: 实现日志过滤
    Q_UNUSED(level);
}