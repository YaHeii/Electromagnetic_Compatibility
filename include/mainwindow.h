#include <QMainWindow>
#include <QObject>
#include <QRect>
#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <future>   
#include <thread>   
#include "utils/QtSpdlogSink.h"
#include "ElaWindow.h"

namespace Ui {
    class MainWindow;
}

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

class MainWindow : public ElaWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // 创建并返回一个指向UI日志接收器的指针
    std::shared_ptr<spdlog::sinks::sink> createGuiLogSink();
signals:

private:
    static QRect pos;
    Ui::MainWindow *ui;
    
    LogEmitter* _logEmitter; // 日志发射器

public slots:
    void onLogReceived(const QString& message, int level);

};



#endif //MAINWINDOW_H