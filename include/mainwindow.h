#include <QMainWindow>
#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <future>   
#include <thread>   
#include "core/ship.h"
#include "core/Equipment.h"
#include "utils/point_2D.h"
#include "utils/conversions.h"
#include "core/fleet.h"
#include "models/EMC_Engine.h"
#include "../resource/ui/TreeViewManager.h"
#include "utils/TransferToEngin.h"
#include "utils/PaintImage.hpp"
#include "models/PEModel.h"
#include "utils/QtSpdlogSink.h"
#include "utils/PaintImage.hpp"
#include "ElaWindow.h"
class ShipWidget;
class DeviceWidget;

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
    // 用于从工作线程安全地将结果传递到UI线程
    void simulationDone(const GridMap& result);
 
private:
    static QRect pos;
    bool updateDeviceModelFromView();
    bool updateShipModelFromView();
    Ui::MainWindow *ui;
    TreeViewManager *_treeView;
    LogEmitter* _logEmitter; // 日志发射器
    EMC_Engine* _emcEngine;
    // 在后台线程中等待仿真结果
    void simulationWaiter(std::future<GridMap> future);

public slots:
    void onLogReceived(const QString& message, int level);
    void on_addShipButton_clicked();
    void on_addDeviceButton_clicked();
    void on_DeviceSave_clicked();
    void on_ShipSave_clicked();
    void on_StartSimulate_clicked();
    // 槽函数现在接收GridMap作为参数
    void onSimulationFinished(const GridMap& result);
    //单张图返回
    void onSingleGridMapReady(const std::string& shipName, const std::string& equipmentName, const GridMap& lossGrid);
    void onDeviceWidgetRemovalRequested(const QString &id);
};



#endif //MAINWINDOW_H