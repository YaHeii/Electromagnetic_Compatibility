#include "shipwidget.h"
#include <QLayout>
#include "deviceonship.h"
#include "spdlog/spdlog.h"

ShipWidget::ShipWidget(QWidget *parent) :
    QWidget(parent)
{
    setupUI();
}

ShipWidget::~ShipWidget()
{
}

void ShipWidget::setupUI()
{
    // Main layout
    mainLayout = new QVBoxLayout(this);
    
    // Coordinates widget with minimum height
    coordinatesWidget = new QWidget(this);
    coordinatesWidget->setMinimumHeight(200);
    coordinatesLayout = new QHBoxLayout(coordinatesWidget);
    
    // Left side - Coordinate fields
    leftCoordinatesLayout = new QHBoxLayout();
    coordinateFieldsLayout = new QVBoxLayout();
    
    // X coordinate
    xLayout = new QHBoxLayout();
    xLabel = new ElaText("X坐标", this);
    xLabel->setTextPixelSize(13);
    X_offset = new ElaLineEdit(this);
    xLayout->addWidget(xLabel);
    xLayout->addWidget(X_offset);
    coordinateFieldsLayout->addLayout(xLayout);
    
    // Y coordinate
    yLayout = new QHBoxLayout();
    yLabel = new ElaText("Y坐标", this);
    yLabel->setTextPixelSize(13);
    Y_offset = new ElaLineEdit(this);
    yLayout->addWidget(yLabel);
    yLayout->addWidget(Y_offset);
    coordinateFieldsLayout->addLayout(yLayout);
    
    // Z coordinate
    zLayout = new QHBoxLayout();
    zLabel = new ElaText("Z坐标", this);
    zLabel->setTextPixelSize(13);
    Z_offset = new ElaLineEdit(this);
    zLayout->addWidget(zLabel);
    zLayout->addWidget(Z_offset);
    coordinateFieldsLayout->addLayout(zLayout);
    
    leftCoordinatesLayout->addLayout(coordinateFieldsLayout);
    
    // Right side - Ship properties
    shipPropertiesLayout = new QVBoxLayout();
    
    //ShipID
    IDLayout = new QHBoxLayout();
    IDLabel = new ElaText("名称", this);
    IDLabel->setTextPixelSize(13);
    ship_ID = new ElaLineEdit(this);
    IDLayout->addWidget(IDLabel);
    IDLayout->addWidget(ship_ID);
    shipPropertiesLayout->addLayout(IDLayout);

    // Speed
    speedLayout = new QHBoxLayout();
    speedLabel = new ElaText("船速", this);
    speedLabel->setTextPixelSize(13);
    ship_Speed = new ElaLineEdit(this);
    speedLayout->addWidget(speedLabel);
    speedLayout->addWidget(ship_Speed);
    shipPropertiesLayout->addLayout(speedLayout);
    
    // Orientation
    orientationLayout = new QHBoxLayout();
    orientationLabel = new ElaText("朝向", this);
    orientationLabel->setTextPixelSize(13);
    ship_Orienteation = new ElaLineEdit(this);
    orientationLayout->addWidget(orientationLabel);
    orientationLayout->addWidget(ship_Orienteation);
    shipPropertiesLayout->addLayout(orientationLayout);
    
    leftCoordinatesLayout->addLayout(shipPropertiesLayout);
    coordinatesLayout->addLayout(leftCoordinatesLayout);
    
    // Device management section
    deviceManagementLayout = new QVBoxLayout();
    
    scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    
    scrollAreaWidgetContents = new QWidget();
    scrollAreaWidgetContents->setGeometry(0, 0, 77, 200);
    
    scrollAreaContentsLayout = new QVBoxLayout(scrollAreaWidgetContents);
    DeviceonShipLayout = new QVBoxLayout();
    scrollAreaContentsLayout->addLayout(DeviceonShipLayout);
    
    scrollArea->setWidget(scrollAreaWidgetContents);
    deviceManagementLayout->addWidget(scrollArea);
    
    shipEquipmentPlus = new ElaPushButton("添加设备", this);
    deviceManagementLayout->addWidget(shipEquipmentPlus);
    
    // Add both sections to coordinates layout
    coordinatesLayout->addLayout(deviceManagementLayout);
    
    mainLayout->addWidget(coordinatesWidget);
    
    // Delete button
    deleteShip = new ElaPushButton("-", this);
    mainLayout->addWidget(deleteShip);
    
    // Connect signals
    connect(shipEquipmentPlus, &ElaPushButton::clicked, this, &ShipWidget::on_shipEquipmentPlus_clicked);
    connect(deleteShip, &ElaPushButton::clicked, this, &ShipWidget::on_deleteShip_clicked);
}

void ShipWidget::setData(const ShipData& data)
{
    _currentShipId = data.shipID;

    // 填充舰船的基本信息
    ship_ID->setText(QString(QString::fromStdString(data.shipID)));
    X_offset->setText(QString::number(data.X_offset));
    Y_offset->setText(QString::number(data.Y_offset));
    Z_offset->setText(QString::number(data.Z_offset));
    ship_Orienteation->setText(QString::number(data.ship_Orienteation));
    ship_Speed->setText(QString::number(data.ship_Speed));

    // 根据数据刷新舰船上的设备列表
    syncDeviceListWithModel();
}

void ShipWidget::updateShipModelData()
{
    // 遍历数据模型，找到与自己ID匹配的ShipData
    for (ShipData &ship : DataModel::instance()->allShips) {
        if (ship.shipID == _currentShipId) {
            // 用UI的值更新模型数据
             ship.shipID = ship_ID->text().toStdString();
            
            // 使用toDouble函数的bool*参数检查转换是否成功
            bool ok = false;
            double value = 0.0;
            
            // 更新ship_X
            value = X_offset->text().toDouble(&ok);
            if (ok) {
                ship.X_offset = value;
            }
           
            // 更新ship_Y
            value = Y_offset->text().toDouble(&ok);
            if (ok) {
                ship.Y_offset = value;
            }
           
            value = Z_offset->text().toDouble(&ok);
            if (ok) {
                ship.Z_offset = value;
            }

            // 更新ship_Orienteation
            value = ship_Orienteation->text().toDouble(&ok);
            if (ok) {
                ship.ship_Orienteation = value;
            }
           
            // 更新ship_Speed，确保速度非负
            value = ship_Speed->text().toDouble(&ok);
            if (ok && value >= 0) {
                ship.ship_Speed = value;
            }

            // 注意：舰船上配置的设备列表是通过"+"按钮直接修改模型的，
            // 这里通常不需要再单独更新，除非有删除或修改设备的操作。

            return; // 找到并更新后即可退出
        }
    }
    // 遍历数据模型，找到与自己ID匹配的ShipData
    for (ShipData &ship : DataModel::instance()->allShips) {
        if (ship.shipID == _currentShipId) {
            // 用UI的值更新模型数据
            // ship.shipName = ui->ship_Name->text();
            
            // 使用toDouble函数的bool*参数检查转换是否成功
            bool ok = false;
            double value = 0.0;
            
            // 更新ship_X
            value = X_offset->text().toDouble(&ok);
            if (ok) {
                ship.X_offset = value;
            }
            
            // 更新ship_Y
            value = Y_offset->text().toDouble(&ok);
            if (ok) {
                ship.Y_offset = value;
            }
            
            value = Z_offset->text().toDouble(&ok);
            if (ok) {
                ship.Z_offset = value;
            }

            // 更新ship_Orienteation
            value = ship_Orienteation->text().toDouble(&ok);
            if (ok) {
                ship.ship_Orienteation = value;
            }
            
            // 更新ship_Speed，确保速度非负
            value = ship_Speed->text().toDouble(&ok);
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
	spdlog::debug("无人船 {} 已删除", this->_currentShipId);
}


void ShipWidget::on_shipEquipmentPlus_clicked()
{
    // 在数据模型中为当前舰船添加一个空的设备配置
    for (ShipData &ship : DataModel::instance()->allShips) {
        if (ship.shipID == _currentShipId) {
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
    while (QLayoutItem* item = DeviceonShipLayout->takeAt(0)) {
        delete item->widget();
        delete item;
    }

    // 找到当前舰船的数据
    for (const ShipData &ship : DataModel::instance()->allShips) {
        if (ship.shipID == _currentShipId) {
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

                DeviceonShipLayout->addWidget(deviceEntryUi);
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
    int index = DeviceonShipLayout->indexOf(deviceWidget);
    if (index == -1) {
        return;
    }
 
    // 从数据模型中移除对应的设备配置
    for (ShipData &ship : DataModel::instance()->allShips) {
        if (ship.shipID == _currentShipId) {
            if (index < ship.Equipments.size()) {
                ship.Equipments.erase(ship.Equipments.begin() + index);
                spdlog::debug("从舰船 {} 中删除了索引为 {} 的设备", _currentShipId, index);
 
                // 从布局中移除并删除小部件
                DeviceonShipLayout->removeWidget(deviceWidget);
                deviceWidget->deleteLater();
            }
            break;
        }
    }
}

void ShipWidget::on_addShipButton_clicked()
{
    updateShipModelFromView();

    ShipData newShip;
    newShip.shipID = DataModel::instance()->allShips.size() + 1;
    DataModel::instance()->allShips.push_back(newShip);
    ShipWidget* widget = new ShipWidget();
    widget->setData(newShip); // 关联UI与数据
    shipsLayout->addWidget(widget);
    //_treeView->syncViewWithModel();
}


void ShipWidget::on_ShipSave_clicked()
{
    if (updateShipModelFromView()) {
        QMessageBox::information(this, "成功", "舰船信息已保存并校验通过。");
        spdlog::info("Ship data saved and validated.");
    }
}

bool ShipWidget::updateShipModelFromView()
{
    // 1. 从 View 同步到 Model
    for (int i = 0; i < shipsLayout->count(); ++i) {
        QLayoutItem* item = shipsLayout->itemAt(i);
        if (item && item->widget()) {
             ShipWidget* widget = qobject_cast<ShipWidget*>(item->widget());
             if (widget) {
                widget->updateShipModelData(); // 这是一个 void 函数，只负责赋值
             }
        }
    }

    // 2. 执行校验逻辑
    // 遍历 DataModel 中的所有船只进行检查
    auto& ships = DataModel::instance()->allShips;
    for (int i = 0; i < ships.size(); ++i) {
        auto result = ships[i].validate_Ship(); // 调用 validate
        if (!result.first) {
            // 校验失败，弹出警告
            // QString errorMsg = QString("船只数据错误 (第 %1 个):%2").arg(i + 1).arg(result.second);
            // QMessageBox::critical(this, "校验失败", errorMsg);
            spdlog::error("Validation failed for ship {}: {}", i, result.second.toStdString());
            return false; // 中断
        }
    }

    // 3. 同步 TreeView (如果校验通过)
    //_treeView->syncViewWithModel();
    return true;
}
