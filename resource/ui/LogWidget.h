#pragma once

#include <QWidget>
#include <QAbstractListModel>
#include <QListView>
#include <vector>
#include <memory>
#include <QString>
#include "spdlog/spdlog.h"
#include "Utils/QtSpdlogSink.h"

// 前向声明自定义控件
class ElaText;
class ElaListView;
class ElaToolButton;
class ElaComboBox;
class ElaScrollBar;
class LogListModel;

class LogWidget : public QWidget {
    Q_OBJECT
public:
    explicit LogWidget(QWidget* parent = nullptr);
    ~LogWidget();
    
    // 创建并返回一个指向UI日志接收器的指针
    std::shared_ptr<spdlog::sinks::sink> createGuiLogSink();

private slots:
    void onLogReceived(const QString& message, int level);
    void onLevelFilterChanged();
    void clearLogs();
    void togglePause();
    void exportLogs();

private:
    // UI控件成员
    QWidget *titleWidget;
    QWidget *toolbarWidget;
    QWidget *logDisplayWidget;
    
    // 工具栏控件
    ElaComboBox *levelFilter;
    ElaToolButton *pauseButton;
    
    // 日志显示控件
    ElaListView *logListView;
    
    // 统计标签
    ElaText *totalLabel;
    ElaText *errorLabel;
    ElaText *infoLabel;
    
    // 日志发射器
    LogEmitter* _logEmitter;
    
    // 统计数据
    int totalLogs = 0;
    int errorLogs = 0;
    int infoLogs = 0;
    bool isPaused = false;
    
    // UI设置方法
    void setupUI();
    void setupTitleWidget();
    void setupToolbarWidget();
    void setupLogDisplayWidget();
    
    // 辅助方法
    void updateStatistics(spdlog::level::level_enum level);
    bool shouldDisplayLog(spdlog::level::level_enum level);

    LogListModel* _logModel;
};

// 自定义日志模型
class LogListModel : public QAbstractListModel {
    Q_OBJECT
    
public:
    explicit LogListModel(QObject *parent = nullptr);
    
    // QAbstractListModel 接口
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    
    // 自定义方法
    void appendLog(const QString &message, spdlog::level::level_enum level);
    void clearLogs();
    void filterLogs(const QString &level);
    
private:
    struct LogEntry {
        QString message;
        spdlog::level::level_enum level;
        QColor color;
        QFont font;
    };
    
    QList<LogEntry> logEntries;
};

