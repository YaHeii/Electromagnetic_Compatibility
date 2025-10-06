
#include <QMainWindow>
#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include "../include/utils/data_get.h"
#include "../include/core/ship.h"
#include "../include/core/equipment.h"
#include "../include/utils/point_2D.h"
#include "../include/models/shortlist.h"
#include "../include/utils/data_get.h"
#include "../include/utils/conversions.h"
#include "../include/models/PropagationModle.h"
#include "../include/core/fleet.h"
#include "../include/core/EMC_Engine.h"
#include "../include/models/Path.h"
#include "../include/models/move.h"
#include "../include/models/datamodel.h"

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

private  slots:
    void on_addShipButton_clicked();
    void on_addDeviceButton_clicked();
    void on_DeviceSave_clicked();
    void on_ShipSave_clicked();
};



#endif //MAINWINDOW_H
