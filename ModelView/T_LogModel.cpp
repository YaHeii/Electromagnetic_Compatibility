#include "T_LogModel.h"

T_LogModel::T_LogModel(QObject* parent)
    : QAbstractListModel{parent}, _currentFilterLevel("debug")
{
}

T_LogModel::~T_LogModel()
{
}

int T_LogModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return _displayEntries.count();
}

QVariant T_LogModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= _displayEntries.count()) {
        return QVariant();
    }
    
    const LogEntry &entry = _displayEntries.at(index.row());
    
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
    _logEntries = logEntries;
    
    // 强制截断为最多 1000 条
    while (_logEntries.count() > 1000) {
        _logEntries.removeFirst();
    }

    // 重构展示列表
    _displayEntries.clear();
    for (const LogEntry& entry : _logEntries) {
        if (isLevelMatch(entry.level, _currentFilterLevel)) {
            _displayEntries.append(entry);
        }
    }
    endResetModel();
}

QList<T_LogModel::LogEntry> T_LogModel::getLogList() const
{
    return this->_logEntries;
}

void T_LogModel::appendLogList(LogEntry log)
{
    // 1. 检查是否达到 1000 条上限，先处理移除
    if (_logEntries.count() >= 1000) {
        const LogEntry& oldest = _logEntries.first();
        // 如果最老的这条刚好在当前的展示列表中，需要通知 View 移除
        if (isLevelMatch(oldest.level, _currentFilterLevel)) {
            beginRemoveRows(QModelIndex(), 0, 0);
            _displayEntries.removeFirst();
            endRemoveRows();
        }
        _logEntries.removeFirst();
    }

    // 2. 添加新条目
    _logEntries.append(log);
    
    // 3. 如果符合当前的过滤条件，再插入到 UI 展示列表
    if (isLevelMatch(log.level, _currentFilterLevel)) {
        beginInsertRows(QModelIndex(), _displayEntries.count(), _displayEntries.count());
        _displayEntries.append(log);
        endInsertRows();
    }
}void T_LogModel::appendLogList(const QString &message, spdlog::level::level_enum level)
{
    // 拆解字段：抛弃时间与级别。
    // spdlog 默认/常见的格式后缀通常是 "] " (例如 "[2025...] [info] 核心提示信息")
    // 截取最后一个 "] " 之后的内容即可获取纯粹的 message
    QString coreMsg = message;
    int lastBracketIdx = coreMsg.lastIndexOf("] ");
    if (lastBracketIdx != -1) {
        coreMsg = coreMsg.mid(lastBracketIdx + 2);
    }

    LogEntry entry;
    entry.message = coreMsg;
    entry.level = level;
    
    // 设置颜色
    switch (level) {
    case spdlog::level::err:
    case spdlog::level::critical: entry.color = QColor(255, 0, 0); break;
    case spdlog::level::warn:     entry.color = QColor(255, 165, 0); break;
    case spdlog::level::info:     entry.color = QColor(0, 128, 0); break;
    case spdlog::level::debug:    entry.color = QColor(128, 128, 128); break;
    default:                      entry.color = QColor(0, 0, 0); break;
    }
    
    // 复用 appendLogList(LogEntry) 的并发及插入/删除逻辑
    appendLogList(entry);
}



void T_LogModel::filterLogList(const QString &level)
{
    // 避免重复刷新
    if (_currentFilterLevel == level) return;
    
    _currentFilterLevel = level;

    // 重新根据过滤条件构建展示列表
    beginResetModel();
    _displayEntries.clear();
    for (const LogEntry& entry : _logEntries) {
        if (isLevelMatch(entry.level, _currentFilterLevel)) {
            _displayEntries.append(entry);
        }
    }
    endResetModel();
}

bool T_LogModel::isLevelMatch(spdlog::level::level_enum level, const QString& filter) const
{
    if (filter.isEmpty() || filter.compare("All", Qt::CaseInsensitive) == 0) {
        return true;
    }
    
    // 获取 spdlog 对应的 level 字符串 (如 "info", "warn")
    auto sv = spdlog::level::to_string_view(level);
    QString levelStr = QString::fromUtf8(sv.data(), static_cast<int>(sv.size()));
    
    return levelStr.compare(filter, Qt::CaseInsensitive) == 0;
}

void T_LogModel::onNewLog(const QString& msg, int level)
{
    // 如果处于暂停状态，直接丢弃（不存入Model）
    if (_isPaused) {
        return; 
    }

    auto logLevel = static_cast<spdlog::level::level_enum>(level);
    
    // 1. 更新统计数据并通知 UI
    updateStatistics(logLevel);

    // 2. 插入数据
    appendLogList(msg, logLevel);
}

void T_LogModel::updateStatistics(spdlog::level::level_enum level)
{
    _totalLogs++;
    if (level == spdlog::level::err || level == spdlog::level::critical) {
        _errorLogs++;
    } else if (level == spdlog::level::info) {
        _infoLogs++;
    }
    
    // 发射信号，让绑定的 UI 自动更新
    emit statisticsChanged(_totalLogs, _errorLogs, _infoLogs);
}

void T_LogModel::clearLogList()
{
    beginResetModel();
    _logEntries.clear();
    _displayEntries.clear();
    
    // 清空时重置统计数据并通知 UI
    _totalLogs = 0;
    _errorLogs = 0;
    _infoLogs = 0;
    emit statisticsChanged(_totalLogs, _errorLogs, _infoLogs);
    
    endResetModel();
}