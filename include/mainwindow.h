#include <QMainWindow>
#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include "../include/core/ship.h"
#include "../include/core/equipment.h"
#include "../include/utils/point_2D.h"
#include "../include/models/Computing_distance.h"
#include "../include/utils/conversions.h"
#include "../include/models/RayModel.h"
#include "../include/core/fleet.h"
#include "../include/models/EMC_Engine.h"
#include "models/Path.h"
#include "models/move.h"
#include "models/datamodel.h"
#include "../resource/ui/TreeViewManager.h"
#include "utils/TransferToEngin.h"
#include "utils/PaintImage.hpp"
#include "models/PEModel.h"
#include "utils/QtSpdlogSink.h"


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
    void updateDeviceModelFromView();
    void updateShipModelFromView();
    // void syncDeviceViewWithModel();
    // void syncShipViewWithModel();
    // QList<DeviceWidget*> m_deviceList;
    // QList<ShipWidget*> m_shipList;
    Ui::MainWindow *ui;
    TreeViewManager *m_treeView;
    LogEmitter* m_logEmitter;// 日志发射器
private  slots:
    // 槽函数：用来接收日志并分发到不同的 TextEdit
    void onLogReceived(const QString& message, int level);
    void on_addShipButton_clicked();
    void on_addDeviceButton_clicked();
    void on_DeviceSave_clicked();
    void on_ShipSave_clicked();
    void on_StartSimulate_clicked(); // 添加仿真开始函数声明
};



#endif //MAINWINDOW_H