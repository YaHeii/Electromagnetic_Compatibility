//
// Created by lenovo on 25-10-5.
//

#include "shipwidget.h"
#include "ui_shipwidget.h"
#include <QLayout>
#include "deviceonship.h"

ShipWidget::ShipWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::shipWidget)
{
    ui->setupUi(this);
}

ShipWidget::~ShipWidget()
{
    delete ui;
}

void ShipWidget::setData(const ShipData& data)
{
    m_currentShipId = data.shipID;

    // 填充舰船的基本信息
    // ui->ship_Name->setText(data.shipName); // 假设UI中有个叫ship_Name的QLineEdit
    ui->ship_X->setText(QString::number(data.ship_X));
    ui->ship_Y->setText(QString::number(data.ship_Y));
    ui->ship_Orienteation->setText(QString::number(data.ship_Orienteation));
    ui->ship_Speed->setText(QString::number(data.ship_Speed));

    // 根据数据刷新舰船上的设备列表
    syncDeviceListWithModel();
}

void ShipWidget::updateShipModelData()
{
    // 遍历数据模型，找到与自己ID匹配的ShipData
    for (ShipData &ship : DataModel::instance()->allShips) {
        if (ship.shipID == m_currentShipId) {
            // 用UI的值更新模型数据
            // ship.shipName = ui->ship_Name->text();
            ship.ship_X = ui->ship_X->text().toDouble();
            ship.ship_Y = ui->ship_Y->text().toDouble();
            ship.ship_Orienteation = ui->ship_Orienteation->text().toDouble();
            ship.ship_Speed = ui->ship_Speed->text().toDouble();

            // 注意：舰船上配置的设备列表是通过“+”按钮直接修改模型的，
            // 这里通常不需要再单独更新，除非有删除或修改设备的操作。

            return; // 找到并更新后即可退出
        }
    }
}

void ShipWidget::on_shipEquipmentPlus_clicked()
{
    // 1. 在数据模型中为当前舰船添加一个空的设备配置
    for (ShipData &ship : DataModel::instance()->allShips) {
        if (ship.shipID == m_currentShipId) {
            DeviceOnShipConfig newConfig;
            // 默认可以不选择任何设备，或者选择第一个可用设备
            if (!DataModel::instance()->allDevices.isEmpty()) {
                newConfig.deviceID = DataModel::instance()->allDevices.first().equipmentID;
            }
            ship.configuredDevices.append(newConfig);
            break; // 修改后退出循环
        }
    }

    // 2. 刷新UI来显示这个新的设备配置条目
    syncDeviceListWithModel();
}

void ShipWidget::syncDeviceListWithModel()
{
    // 清空当前的设备列表UI
    while (QLayoutItem* item = ui->DeviceonShipLayout->takeAt(0)) {
        delete item->widget();
        delete item;
    }

    // 找到当前舰船的数据
    for (const ShipData &ship : DataModel::instance()->allShips) {
        if (ship.shipID == m_currentShipId) {
            // 获取所有已定义设备的ID列表，用于下拉框
            QStringList availableDeviceIDs;
            for(const DeviceData &device : DataModel::instance()->allDevices) {
                availableDeviceIDs.append(device.equipmentID);
            }

            // 遍历这艘船上配置的每一个设备
            for (const DeviceOnShipConfig &config : ship.configuredDevices) {
                DeviceonShip *deviceEntryUi = new DeviceonShip();

                // 填充下拉框，并设置当前选中的项
                // deviceEntryUi->ui->comboBox->addItems(availableDeviceIDs);
                // deviceEntryUi->ui->comboBox->setCurrentText(config.deviceID);

                ui->DeviceonShipLayout->addWidget(deviceEntryUi);
            }
            break; // 找到舰船后即可退出
        }
    }
}

