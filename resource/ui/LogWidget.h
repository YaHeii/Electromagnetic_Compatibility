#pragma once
#include <QWidget>

#include <QWidget>
#include <QAbstractListModel>
#include <QListView>
#include <vector>
#include <memory>
#include <QString>
#include "spdlog/spdlog.h"
#include "Utils/QtSpdlogSink.h"

class T_LogModel;
class LogWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LogWidget(QWidget* parent = nullptr);
    ~LogWidget();
        // 创建并返回一个指向UI日志接收器的指针
    std::shared_ptr<spdlog::sinks::sink> createGuiLogSink();
signals:

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

    // 辅助方法
    void updateStatistics(spdlog::level::level_enum level);
    bool shouldDisplayLog(spdlog::level::level_enum level);

    T_LogModel* _logModel{nullptr};
};

