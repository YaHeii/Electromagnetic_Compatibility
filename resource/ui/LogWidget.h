#pragma once
#include <QWidget>
#include <QAbstractListModel>
#include <QListView>
#include <memory>
#include <QString>
#include "Utils/QtSpdlogSink.h"
#include "ElaComboBox.h"
#include "ElaToolButton.h"
#include "ElaListView.h"
#include "Resource/ui/BasePage.h"

class T_LogModel;
class LogWidget : public BasePage
{
    Q_OBJECT
public:
    explicit LogWidget(QWidget* parent = nullptr);
    ~LogWidget();
    // 创建并返回一个指向UI日志接收器的指针
    T_LogModel* _logModel{nullptr};
    std::shared_ptr<spdlog::sinks::sink> createGuiLogSink();
signals:

private slots:
    void onLevelFilterChanged();
    void on_clearbtn_clicked();
    void on_pauseBtn_clicked();
    void on_openFileBtn_clicked();

private:
    // UI控件成员
    QWidget *titleWidget;
    QWidget *toolbarWidget;
    QWidget *logDisplayWidget;
    
    // 工具栏控件
    ElaComboBox *levelFilter;
    ElaToolButton *pauseButton;
    ElaToolButton *clearButton;
    ElaToolButton *openFileButton;
    // 日志显示控件
    ElaListView *logListView;
    
    // 日志发射器
    LogEmitter* _logEmitter;

    
};

