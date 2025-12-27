#include <QMainWindow>
#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <QFuture>
#include <QFutureWatcher>
#include "core/ship.h"
#include "core/equipment.h"
#include "utils/point_2D.h"
#include "utils/conversions.h"
#include "core/fleet.h"
#include "models/EMC_Engine.h"
#include "models/datamodel.h"
#include "../resource/ui/TreeViewManager.h"
#include "utils/TransferToEngin.h"
#include "utils/PaintImage.hpp"
#include "models/PEModel.h"
#include "utils/QtSpdlogSink.h"
#include "utils/PaintImage.hpp"

class ShipWidget;
class DeviceWidget;

namespace Ui {
    class MainWindow;
}

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    // void onCollectAllDeviceData();
private:
    static QRect pos;
    bool updateDeviceModelFromView();
    bool updateShipModelFromView();
    Ui::MainWindow *ui;
    TreeViewManager *m_treeView;
    LogEmitter* m_logEmitter;// 日志发射器
    QFutureWatcher<GridMap> *m_simWatcher = nullptr; // 仿真任务监视器
private  slots:
    // 接收日志并分发到不同的 TextEdit
    void onLogReceived(const QString& message, int level);
    void on_addShipButton_clicked();
    void on_addDeviceButton_clicked();
    void on_DeviceSave_clicked();
    void on_ShipSave_clicked();
    void on_StartSimulate_clicked(); // 添加仿真开始函数声明
    void onSimulationFinished();
};



#endif //MAINWINDOW_H