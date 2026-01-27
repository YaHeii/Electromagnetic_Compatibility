#include "FleetInput.h"
#include "TreeViewManager.h"
#include "DeviceWidget.h"
#include "shipwidget.h"
#include <QMessageBox>
#include <QLayoutItem> // Added for QLayoutItem
#include <algorithm> // for std::remove_if
#include <QRect>
#include <Qt>

FleetInput::FleetInput(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    ////设置 TreeView 本体透明
    //treeView->setStyleSheet("background-color: transparent; border: none;");
    ////设置设备界面透明
    //deviceScrollArea->setStyleSheet("background-color: transparent; border: none;");
    //deviceScrollArea->viewport()->setStyleSheet("background-color: transparent;");
    //deviceContentsWidget->setAttribute(Qt::WA_TranslucentBackground);
    ////设置舰船界面透明
    //shipScrollArea->setStyleSheet("background-color: transparent; border: none;");
    //shipScrollArea->viewport()->setStyleSheet("background-color: transparent;");
    //shipsContentsWidget->setAttribute(Qt::WA_TranslucentBackground);
    //
    _treeView = new TreeViewManager(treeView, this);
    connectTreeViewSignals();
}

FleetInput::~FleetInput() {
}

void FleetInput::connectTreeViewSignals()
{
    // 可以在这里连接树视图的信号，比如选中项变化等
    connect(_treeView, &TreeViewManager::syncViewWithModel, this, [this]() {
        // 当树视图同步时，可以执行一些操作
    });
}

void FleetInput::setupUI()
{
	// Main layout
	mainLayout = new QVBoxLayout(this);
	
	// Content layout (horizontal split)
	contentLayout = new QHBoxLayout();
	contentLayout->setStretch(0, 1);
	contentLayout->setStretch(1, 4);
	
	// Left side - Tree view manager
	treeViewManager = new ElaTabWidget(this);
	treeViewManager->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	treeViewManager->setTabPosition(QTabWidget::TabPosition::North);
	treeViewManager->setCurrentIndex(0);
	treeViewManager->setElideMode(Qt::TextElideMode::ElideLeft);
	treeViewManager->setDocumentMode(true);
	treeViewManager->setMovable(false);
	
	// Structure tree tab
	structureTab = new QWidget();
	structureTab->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	structureTab->setObjectName("structureTab");
	structureTab->setStyleSheet("QWidget#structureTab { background-color: transparent; }");
	
	structureLayout = new QVBoxLayout(structureTab);
	structureLayout->setSpacing(0);
	structureLayout->setContentsMargins(0, 0, 0, 0);
	
	treeView = new QTreeView(structureTab);
	structureLayout->addWidget(treeView);
	
	treeViewManager->addTab(structureTab, "结构树");
	
	// Right side - Input tabs
	rightLayout = new QVBoxLayout();
	
	inputTabWidget = new ElaTabWidget(this);
	inputTabWidget->setCurrentIndex(0);
	
	// Device tab
	deviceTab = new QWidget();
	deviceTabLayout = new QVBoxLayout(deviceTab);
	deviceTabLayout->setStretch(0, 0);
	deviceTabLayout->setStretch(1, 0);
	deviceTabLayout->setStretch(2, 0);
	
	deviceScrollArea = new QScrollArea(deviceTab);
	deviceScrollArea->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	deviceScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	deviceScrollArea->setWidgetResizable(true);
	
	deviceContentsWidget = new QWidget();
	deviceContentsWidget->setGeometry(0, 0, 414, 337);
	deviceContentsWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	deviceContentsWidget->setObjectName("deviceContentsWidget");
	deviceContentsWidget->setStyleSheet("QWidget#deviceContentsWidget { background-color: transparent; }");
	
	deviceLayout = new QVBoxLayout(deviceContentsWidget);
	
	deviceScrollArea->setWidget(deviceContentsWidget);
	deviceTabLayout->addWidget(deviceScrollArea);
	
	addDeviceButton = new ElaPushButton("添加设备", deviceTab);
	deviceTabLayout->addWidget(addDeviceButton);
	
	DeviceSave = new ElaPushButton("保存", deviceTab);
	deviceTabLayout->addWidget(DeviceSave);
	
	inputTabWidget->addTab(deviceTab, "创建设备");
	
	// Ship tab
	shipTab = new QWidget();
	shipTabLayout = new QVBoxLayout(shipTab);
	
	shipScrollArea = new QScrollArea(shipTab);
	shipScrollArea->setWidgetResizable(true);
	
	shipsContentsWidget = new QWidget();
	shipsContentsWidget->setGeometry(0, 0, 426, 337);
	
	shipsLayout = new QVBoxLayout(shipsContentsWidget);
	shipsLayout->setSizeConstraint(QLayout::SetDefaultConstraint);
	
	shipScrollArea->setWidget(shipsContentsWidget);
	shipTabLayout->addWidget(shipScrollArea);
	
	addShipButton = new ElaPushButton("添加舰船", shipTab);
	shipTabLayout->addWidget(addShipButton);
	
	ShipSave = new ElaPushButton("保存", shipTab);
	shipTabLayout->addWidget(ShipSave);
	
	inputTabWidget->addTab(shipTab, "创建舰队");
	
	rightLayout->addWidget(inputTabWidget);
	
	// Add to content layout
	contentLayout->addWidget(treeViewManager);
	contentLayout->addLayout(rightLayout);
	
	// Add to main layout
	mainLayout->addLayout(contentLayout);
	
	// Connect signals
	connect(addShipButton, &ElaPushButton::clicked, this, &FleetInput::on_addShipButton_clicked);
	connect(addDeviceButton, &ElaPushButton::clicked, this, &FleetInput::on_addDeviceButton_clicked);
	connect(DeviceSave, &ElaPushButton::clicked, this, &FleetInput::on_DeviceSave_clicked);
	connect(ShipSave, &ElaPushButton::clicked, this, &FleetInput::on_ShipSave_clicked);
}

void FleetInput::on_addDeviceButton_clicked()
{
    //在添加新控件前，先将UI上所有未保存的修改更新到数据模型中
    updateDeviceModelFromView();

    EquipmentData newDevice;
    newDevice.equipmentID = QString("NewDevice%1").arg(DataModel::instance()->allEquipments.size() + 1);
    // 首先在DataModel中占位
    DataModel::instance()->allEquipments.push_back(newDevice);
    // 然后创建新的DeviceWidget并添加到UI
    DeviceWidget* widget = new DeviceWidget();
    widget->setData(newDevice); // 关联UI与数据
    deviceLayout->addWidget(widget);
    connect(widget, &DeviceWidget::removalRequested, this, &FleetInput::onDeviceWidgetRemovalRequested);
    
    //同步treeView
    _treeView->syncViewWithModel();
}

void FleetInput::on_addShipButton_clicked()
{
    updateShipModelFromView();

    ShipData newShip;
    newShip.shipID = DataModel::instance()->allShips.size() + 1;
    DataModel::instance()->allShips.push_back(newShip);
    ShipWidget* widget = new ShipWidget();
    widget->setData(newShip); // 关联UI与数据
    shipsLayout->addWidget(widget);
    _treeView->syncViewWithModel();
}

void FleetInput::on_DeviceSave_clicked()
{
    if (updateDeviceModelFromView()) {
        QMessageBox::information(this, "成功", "设备信息已保存并校验通过。");
        spdlog::info("Device data saved and validated.");
    }
}

void FleetInput::on_ShipSave_clicked()
{
    if (updateShipModelFromView()) {
        QMessageBox::information(this, "成功", "舰船信息已保存并校验通过。");
        spdlog::info("Ship data saved and validated.");
    }
}

bool FleetInput::updateShipModelFromView()
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
    _treeView->syncViewWithModel();
    return true;
}

bool FleetInput::updateDeviceModelFromView()
{
    // 1. 从 View 同步到 Model
    for (int i = 0; i < deviceLayout->count(); ++i) {
        QLayoutItem* item = deviceLayout->itemAt(i);
        if (item && item->widget()) {
            DeviceWidget* widget = qobject_cast<DeviceWidget*>(item->widget());
            if (widget) {
                // 让每个Widget用自己UI上的当前值去更新数据模型
                widget->updateModelData();
            }
        }
    }

    // 2. 执行校验逻辑
    auto& equipments = DataModel::instance()->allEquipments;
    for (int i = 0; i < equipments.size(); ++i) {
        auto result = equipments[i].validate();
        if (!result.first) {
            QString errorMsg = QString("设备数据错误 (ID: %1):%2")
                .arg(equipments[i].equipmentID)
                .arg(result.second);
            QMessageBox::critical(this, "校验失败", errorMsg);
            spdlog::error("Validation failed for equipment {}: {}",
                equipments[i].equipmentID.toStdString(), result.second.toStdString());
            return false;
        }
    }

    _treeView->syncViewWithModel();
    return true;
}

void FleetInput::onDeviceWidgetRemovalRequested(const QString& id)
{
    // 1. 从DataModel中移除对应的数据
    auto& equipments = DataModel::instance()->allEquipments;
    auto it = std::remove_if(equipments.begin(), equipments.end(),
        [&](const EquipmentData& ed) { return ed.equipmentID == id; });

    if (it != equipments.end()) {
        equipments.erase(it, equipments.end());
        spdlog::info("设备 {} 的数据已从模型中删除。", id.toStdString());

        // 2. 遍历布局，找到并删除对应的UI控件
        for (int i = 0; i < deviceLayout->count(); ++i) {
            QLayoutItem* item = deviceLayout->itemAt(i);
            if (item && item->widget()) {
                DeviceWidget* widget = qobject_cast<DeviceWidget*>(item->widget());
                // 假设DeviceWidget有方法可以获取其ID
                if (widget && widget->getID() == id) {
                    deviceLayout->removeWidget(widget);
                    widget->deleteLater();
                    spdlog::info("设备 {} 的UI控件已删除。", id.toStdString());
                    break; // 找到并删除后即可退出循环
                }
            }
        }

        // 3. 更新TreeView
        _treeView->syncViewWithModel();
    }
    else {
        spdlog::warn("请求删除设备 {}，但在数据模型中未找到。", id.toStdString());
    }
}
