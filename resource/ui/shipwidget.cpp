#include "shipwidget.h"
#include "ui_shipwidget.h"
#include <QLayout>
#include "deviceonship.h"
#include "spdlog/spdlog.h"

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
    ui->X_offset->setText(QString::number(data.X_offset));
    ui->Y_offset->setText(QString::number(data.Y_offset));
	ui->Z_offset->setText(QString::number(data.Z_offset));
    ui->ship_Orienteation->setText(QString::number(data.ship_Orienteation));
    ui->ship_Speed->setText(QString::number(data.ship_Speed));

    // 根据数据刷新舰船上的设备列表
    syncDeviceListWithModel();
}

void ShipWidget::updateShipModelData()
{
    // 检查UI指针有效性
    if (!ui) {
        return;
    }

    // 遍历数据模型，找到与自己ID匹配的ShipData
    for (ShipData &ship : DataModel::instance()->allShips) {
        if (ship.shipID == m_currentShipId) {
            // 用UI的值更新模型数据
            // ship.shipName = ui->ship_Name->text();
            
            // 使用toDouble函数的bool*参数检查转换是否成功
            bool ok = false;
            double value = 0.0;
            
            // 更新ship_X
            value = ui->X_offset->text().toDouble(&ok);
            if (ok) {
                ship.X_offset = value;
            }
            
            // 更新ship_Y
            value = ui->Y_offset->text().toDouble(&ok);
            if (ok) {
                ship.Y_offset = value;
            }
            
            value = ui->Z_offset->text().toDouble(&ok);
            if (ok) {
                ship.Z_offset = value;
            }

            // 更新ship_Orienteation
            value = ui->ship_Orienteation->text().toDouble(&ok);
            if (ok) {
                ship.ship_Orienteation = value;
            }
            
            // 更新ship_Speed，确保速度非负
            value = ui->ship_Speed->text().toDouble(&ok);
            if (ok && value >= 0) {
                ship.ship_Speed = value;
            }

            // 注意：舰船上配置的设备列表是通过“+”按钮直接修改模型的，
            // 这里通常不需要再单独更新，除非有删除或修改设备的操作。

            return; // 找到并更新后即可退出
        }
    }
}

void ShipWidget::on_deleteShip_clicked() {
    delete this;
	spdlog::debug("无人船 {} 已删除", this->m_currentShipId);
}


void ShipWidget::on_shipEquipmentPlus_clicked()
{
    // 在数据模型中为当前舰船添加一个空的设备配置
    for (ShipData &ship : DataModel::instance()->allShips) {
        if (ship.shipID == m_currentShipId) {
            EquipmentOnShip newConfig;
            // 默认可以不选择任何设备，或者选择第一个可用设备
            if (!DataModel::instance()->allEquipments.empty()) {
                newConfig.equipmentID = DataModel::instance()->allEquipments.front().equipmentID;
            }
            ship.Equipments.push_back(newConfig);
            break; // 修改后退出循环
        }
    }

    // 刷新UI来显示这个新的设备配置条目
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
            for(const EquipmentData &device : DataModel::instance()->allEquipments) {
                availableDeviceIDs.append(device.equipmentID);
            }

            // 遍历这艘船上配置的每一个设备
            for (const EquipmentOnShip &config : ship.Equipments) {
                DeviceonShip *deviceEntryUi = new DeviceonShip();

                // 填充下拉框，并设置当前选中的项
                if (!availableDeviceIDs.isEmpty()) {
                    QComboBox* equipmentComboBox = deviceEntryUi->findChild<QComboBox*>("EquipmentID");
                    if (equipmentComboBox) {
                        equipmentComboBox->addItems(availableDeviceIDs);
                        equipmentComboBox->setCurrentText(config.equipmentID);
                    }
                }

                ui->DeviceonShipLayout->addWidget(deviceEntryUi);
                connect(deviceEntryUi, &DeviceonShip::removalRequested, this, &ShipWidget::onDeviceOnShipRemovalRequested);
            }
 
            break; // 找到舰船后即可退出
        }
    }
}
 
void ShipWidget::onDeviceOnShipRemovalRequested()
{
    // 获取发出信号的DeviceonShip小部件
    DeviceonShip* deviceWidget = qobject_cast<DeviceonShip*>(sender());
    if (!deviceWidget) {
        return;
    }
 
    // 在布局中找到该小部件的索引
    int index = ui->DeviceonShipLayout->indexOf(deviceWidget);
    if (index == -1) {
        return;
    }
 
    // 从数据模型中移除对应的设备配置
    for (ShipData &ship : DataModel::instance()->allShips) {
        if (ship.shipID == m_currentShipId) {
            if (index < ship.Equipments.size()) {
                ship.Equipments.erase(ship.Equipments.begin() + index);
                spdlog::debug("从舰船 {} 中删除了索引为 {} 的设备", m_currentShipId, index);
 
                // 从布局中移除并删除小部件
                ui->DeviceonShipLayout->removeWidget(deviceWidget);
                deviceWidget->deleteLater();
            }
            break;
        }
    }
}

