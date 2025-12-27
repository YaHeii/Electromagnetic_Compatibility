#include <QMainWindow>
#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <future>   // 包含 std::future 和 std::async
#include <thread>   // 包含 std::thread
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

signals:
    // 用于从工作线程安全地将结果传递到UI线程
    void simulationDone(const GridMap& result);

private:
    static QRect pos;
    bool updateDeviceModelFromView();
    bool updateShipModelFromView();
    Ui::MainWindow *ui;
    TreeViewManager *m_treeView;
    Propagation_Engine *m_engine;
    LogEmitter* m_logEmitter; // 日志发射器

    // 在后台线程中等待仿真结果
    void simulationWaiter(std::future<GridMap> future);

private slots:
    void onLogReceived(const QString& message, int level);
    void on_addShipButton_clicked();
    void on_addDeviceButton_clicked();
    void on_DeviceSave_clicked();
    void on_ShipSave_clicked();
    void on_StartSimulate_clicked();
    // 槽函数现在接收GridMap作为参数
    void onSimulationFinished(const GridMap& result);
};



#endif //MAINWINDOW_H