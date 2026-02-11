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
    return logEntries.count();
}

QVariant T_LogModel::data(const QModelIndex& index, int role) const
{
    // if (role == Qt::DisplayRole)
    // {
    //     return _logList[index.row()];
    // }
    // return QVariant();

        if (!index.isValid() || index.row() >= logEntries.count()) {
        return QVariant();
    }
    
    const LogEntry &entry = logEntries.at(index.row());
    
    switch (role) {
    case Qt::DisplayRole:
        return entry.message;
    case Qt::ForegroundRole:
        return entry.color;
    case Qt::FontRole:
        return entry.font;
    default:
        return QVariant();
    }
}

void T_LogModel::setLogList(QStringList list)
{
    beginResetModel();
    this->_logList = list;
    endResetModel();
}

void T_LogModel::appendLogList(QString log)
{
    beginResetModel();
    this->_logList.append(log);
    endResetModel();
}

void LogListModel::appendLogList(const QString &message, spdlog::level::level_enum level)
{
    beginInsertRows(QModelIndex(), logEntries.count(), logEntries.count());
    
    LogEntry entry;
    entry.message = message;
    entry.level = level;
    
    // 设置颜色
    QColor color;
    QFont font;
    font.setFamily("Consolas");
    font.setPixelSize(11);
    
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
    entry.font = font;
    
    logEntries.append(entry);
    
    // 限制最大条目数
    if (logEntries.count() > 1000) {
        beginRemoveRows(QModelIndex(), 0, 0);
        logEntries.removeFirst();
        endRemoveRows();
    }
    
    endInsertRows();
}
QStringList T_LogModel::getLogList() const
{
    return this->_logList;
}

void LogListModel::clearLogList()
{
    beginResetModel();
    logEntries.clear();
    endResetModel();
}

void LogListModel::filterLogList(const QString &level)
{
    // TODO: 实现日志过滤
    Q_UNUSED(level);
}