#include "shipwidget.h"
#include <QLayout>
#include "deviceonship.h"
#include "spdlog/spdlog.h"

ShipWidget::ShipWidget(QWidget *parent) 
    : BasePage(parent)
{
    // X coordinate
    ElaText* xText = new ElaText("X坐标", this);
    xText->setTextPixelSize(15);
    _X_offset = new ElaLineEdit(this);
    
    // Y coordinate
    ElaText* yText = new ElaText("Y坐标", this);
    yText->setTextPixelSize(15);
    _Y_offset = new ElaLineEdit(this);

    // Z coordinate
    ElaText* zText = new ElaText("Z坐标", this);
    zText->setTextPixelSize(15);
    _Z_offset = new ElaLineEdit(this);

    QHBoxLayout* firstLine = new QHBoxLayout();
    firstLine->addWidget(xText);
    firstLine->addSpacing(15);
    firstLine->addWidget(_X_offset);
    firstLine->addSpacing(15);
    firstLine->addWidget(yText);
    firstLine->addSpacing(15);
    firstLine->addWidget(_Y_offset);
    firstLine->addSpacing(15);
    firstLine->addWidget(zText);
    firstLine->addSpacing(15);
    firstLine->addWidget(_Z_offset);
    firstLine->addSpacing(15);

    //ShipID
    ElaText* IDText = new ElaText("名称", this);
    IDText->setTextPixelSize(15);
    _ship_ID = new ElaLineEdit(this);

    // Speed
    ElaText* speedText = new ElaText("船速", this);
    speedText->setTextPixelSize(53);
    _ship_Speed = new ElaLineEdit(this);
    
    // Orientation
    ElaText* orientationText = new ElaText("朝向", this);
    orientationText->setTextPixelSize(15);
    _ship_Orienteation = new ElaLineEdit(this);

    QHBoxLayout* secondLine = new QHBoxLayout();
    secondLine->addWidget(IDText);
    secondLine->addSpacing(15);
    secondLine->addWidget(_ship_ID);
    secondLine->addSpacing(15);
    secondLine->addWidget(speedText);
    secondLine->addSpacing(15);
    secondLine->addWidget(_ship_Speed);
    secondLine->addSpacing(15);
    secondLine->addWidget(orientationText);
    secondLine->addSpacing(15);
    secondLine->addWidget(_ship_Orienteation);
    secondLine->addSpacing(15);

    // Delete button
    ElaPushButton* deleteShip = new ElaPushButton("删除该船", this);
    deleteShip->setFixedSize(60, 32);
    connect(deleteShip, &ElaPushButton::clicked,
        this, &ShipWidget::on_deleteShip_clicked);

    _leftPannel = new QVBoxLayout();
    _leftPannel->addLayout(firstLine);
    _leftPannel->addLayout(secondLine);
    _leftPannel->addWidget(deleteShip);

    ElaPushButton* shipEquipmentPlus = new ElaPushButton("添加设备", this);
    shipEquipmentPlus->setFixedSize(60, 32);
    connect(shipEquipmentPlus, &ElaPushButton::clicked, 
        this, &ShipWidget::on_shipEquipmentPlus_clicked);

    DeviceonShip* deviceList = new DeviceonShip;
    _rightPannel = new QVBoxLayout();
    _rightPannel->addWidget(deviceList);
    _rightPannel->addWidget(shipEquipmentPlus);

    QWidget* centralWidget = new QWidget(this);
    centralWidget->setWindowTitle("船");
    QHBoxLayout* centerHLayout = new QHBoxLayout(centralWidget);
    centerHLayout->setContentsMargins(0, 0, 0, 0);
    centerHLayout->addLayout(_leftPannel);
    centerHLayout->addLayout(_rightPannel);
    centerHLayout->addStretch();

    //addCentralWidget(centralWidget);
}

ShipWidget::~ShipWidget()
{
}


void ShipWidget::setData(const ShipData& data)
{
    _currentShipId = data.shipID;

    // 填充舰船的基本信息
    _ship_ID->setText(QString(QString::fromStdString(data.shipID)));
    _X_offset->setText(QString::number(data.X_offset));
    _Y_offset->setText(QString::number(data.Y_offset));
    _Z_offset->setText(QString::number(data.Z_offset));
    _ship_Orienteation->setText(QString::number(data.ship_Orienteation));
    _ship_Speed->setText(QString::number(data.ship_Speed));

    // 根据数据刷新舰船上的设备列表
    syncDeviceListWithModel();
}

void ShipWidget::updateShipModelData()
{
    // 遍历数据模型，找到与自己ID匹配的ShipData
    for (ShipData &ship : DataModel::instance()->allShips) {
        if (ship.shipID == _currentShipId) {
            // 用UI的值更新模型数据
             ship.shipID = _ship_ID->text().toStdString();
            
            // 使用toDouble函数的bool*参数检查转换是否成功
            bool ok = false;
            double value = 0.0;
            
            // 更新ship_X
            value = _X_offset->text().toDouble(&ok);
            if (ok) {
                ship.X_offset = value;
            }
           
            // 更新ship_Y
            value = _Y_offset->text().toDouble(&ok);
            if (ok) {
                ship.Y_offset = value;
            }
           
            value = _Z_offset->text().toDouble(&ok);
            if (ok) {
                ship.Z_offset = value;
            }

            // 更新ship_Orienteation
            value = _ship_Orienteation->text().toDouble(&ok);
            if (ok) {
                ship.ship_Orienteation = value;
            }
           
            // 更新ship_Speed，确保速度非负
            value = _ship_Speed->text().toDouble(&ok);
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
            value = _X_offset->text().toDouble(&ok);
            if (ok) {
                ship.X_offset = value;
            }
            
            // 更新ship_Y
            value = _Y_offset->text().toDouble(&ok);
            if (ok) {
                ship.Y_offset = value;
            }
            
            value = _Z_offset->text().toDouble(&ok);
            if (ok) {
                ship.Z_offset = value;
            }

            // 更新ship_Orienteation
            value = _ship_Orienteation->text().toDouble(&ok);
            if (ok) {
                ship.ship_Orienteation = value;
            }
            
            // 更新ship_Speed，确保速度非负
            value = _ship_Speed->text().toDouble(&ok);
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
    while (QLayoutItem* item = _rightPannel->takeAt(0)) {
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

                _rightPannel->addWidget(deviceEntryUi);
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
    int index = _rightPannel->indexOf(deviceWidget);
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
                _rightPannel->removeWidget(deviceWidget);
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
    _leftPannel->addWidget(widget);
    //_treeView->syncViewWithModel();
}

void ShipWidget::on_ShipSave_clicked()
{
    if (updateShipModelFromView()) {
        //QMessageBox::information(this, "成功", "舰船信息已保存并校验通过。");
        spdlog::info("Ship data saved and validated.");
    }
}

bool ShipWidget::updateShipModelFromView()
{
    // 1. 从 View 同步到 Model
    for (int i = 0; i < _rightPannel->count(); ++i) {
        QLayoutItem* item = _rightPannel->itemAt(i);
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
