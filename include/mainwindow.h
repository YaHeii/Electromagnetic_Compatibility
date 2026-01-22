#pragma once

#include <QMainWindow>
#include <QObject>
#include <QRect>
#include <iostream>
#include <memory>
#include <future>   
#include <thread>   
#include "ElaWindow.h"
#include "utils/QtSpdlogSink.h" 
namespace Ui {
    class MainWindow;
}

class LogWidget; 
class ElaContentDialog;

class MainWindow : public ElaWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    std::shared_ptr<spdlog::sinks::sink> createGuiLogSink();

    void initWindow();
    void initEdgeLayout();
    //void initContent();

private:
    Ui::MainWindow *ui;
    LogWidget* _logWidget;
    ElaContentDialog* _closeDialog{ nullptr };
    ElaSuggestBox* _windowSuggestBox{ nullptr };
    QString _settingKey{ "" };
};

