#include "ui_mainwindow.h"
#include "ui_devicewidget.h"
#include "../include/mainwindow.h"
#include "../resource/ui/shipwidget.h"
#include "../resource/ui/DeviceWidget.h"
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow) {
    ui->setupUi(this);
    // syncDeviceViewWithModel(); //调用使加载初始数据
}
MainWindow::~MainWindow()
{
    delete ui;
}
void MainWindow::on_addDeviceButton_clicked()
{
    //在添加新控件前，先将UI上所有未保存的修改更新到数据模型中
    updateDeviceModelFromView();

    DeviceData newDevice;
    newDevice.equipmentID = QString("NewDevice_%1").arg(DataModel::instance()->allDevices.count() + 1);
    DataModel::instance()->allDevices.append(newDevice);

    DeviceWidget *widget = new DeviceWidget();
    widget->setData(newDevice); // 用新的空数据填充它
    ui->deviceLayout->addWidget(widget);
}

void MainWindow::on_addShipButton_clicked()
{
    updateShipModelFromView();

    ShipData newShip;
    newShip.shipID = DataModel::instance()->allShips.count() + 1;
    newShip.shipName = QString("NewShip_%1").arg(newShip.shipID);
    DataModel::instance()->allShips.append(newShip);

    ShipWidget *widget = new ShipWidget();
    widget->setData(newShip);
    ui->shipsLayout->addWidget(widget);

}

void MainWindow::on_DeviceSave_clicked()
{
    updateDeviceModelFromView();
    QMessageBox::information(this, "成功", "所有设备更改已应用到数据模型。");
}

void MainWindow::on_ShipSave_clicked()
{
    updateShipModelFromView();
    QMessageBox::information(this, "成功", "所有舰船更改已应用到数据模型。");
}

void MainWindow::updateDeviceModelFromView()
{
    for (int i = 0; i < ui->deviceLayout->count(); ++i) {
        DeviceWidget* widget = qobject_cast<DeviceWidget*>(ui->deviceLayout->itemAt(i)->widget());
        if (widget) {
            // 让每个Widget用自己UI上的当前值去更新数据模型
            widget->updateModelData();
        }
    }
}
void MainWindow::updateShipModelFromView()
{
    for (int i = 0; i < ui->shipsLayout->count(); ++i) {
        ShipWidget *widget = qobject_cast<ShipWidget*>(ui->shipsLayout->itemAt(i)->widget());
        if (widget) {
            // 让每个Widget用自己UI上的当前值去更新数据模型
            widget->updateShipModelData();
        }
    }
}

