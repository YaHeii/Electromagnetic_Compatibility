#include "TreeViewManager.h"
#include <QTreeView>
#include <QStandardItem>

TreeViewManager::TreeViewManager(QTreeView *treeView, QObject *parent)
    : QObject(parent), _treeView(treeView)
{
    // 创建一个标准的item模型
    m_model = new QStandardItemModel(this);
    // 将模型设置给TreeView
    _treeView->setModel(m_model);
}
void TreeViewManager::syncViewWithModel()
{
    // 1. 清空旧模型的所有内容
    m_model->clear();

    // 2. 设置列头
    m_model->setHorizontalHeaderLabels({"名称", "ID / 类型"});

    // 3. 创建顶层根节点（不可见）
    QStandardItem *rootItem = m_model->invisibleRootItem();

    // 4. 创建并填充 "舰船" 顶层分类
    QStandardItem *shipsRoot = new QStandardItem("舰船编队");
    rootItem->appendRow(shipsRoot);
    populateShips(shipsRoot);

    // 5. 创建并填充 "设备库" 顶层分类
    QStandardItem *devicesRoot = new QStandardItem("设备库");
    rootItem->appendRow(devicesRoot);
    populateDevices(devicesRoot);

    // 6. （可选）默认展开顶层节点
    _treeView->expand(shipsRoot->index());
    _treeView->expand(devicesRoot->index());
}

// 填充所有舰船及其配置的设备
void TreeViewManager::populateShips(QStandardItem *shipsRoot)
{
    const std::vector<ShipData>& ships = DataModel::instance()->allShips;

    for (const ShipData &ship : ships) {
        // 创建代表一艘舰船的节点
        QStandardItem *shipItem = new QStandardItem(ship.shipName);
        QStandardItem *shipIdItem = new QStandardItem(QString("ID: %1").arg(ship.shipID));
        shipsRoot->appendRow({shipItem, shipIdItem});

        // 如果舰船上有配置设备，则为其创建一个子分类
        if (!ship.Equipments.empty()) {
            QStandardItem *configuredDevicesCategory = new QStandardItem("配置的设备");
            shipItem->appendRow(configuredDevicesCategory);

            for (const EquipmentOnShip &config : ship.Equipments) {
                // 创建代表一个已配置设备的子节点
                QStandardItem *deviceOnShipItem = new QStandardItem(config.equipmentID);
                configuredDevicesCategory->appendRow(deviceOnShipItem);
            }
        }
    }
}

// 填充设备库中的所有独立设备
void TreeViewManager::populateDevices(QStandardItem *devicesRoot)
{
    const std::vector<EquipmentData>& devices = DataModel::instance()->allEquipments;
    for (const EquipmentData &device : devices) {
        // 创建代表一个独立设备的节点
        QStandardItem *deviceItem = new QStandardItem(device.equipmentID);
        QStandardItem *deviceTypeItem = new QStandardItem(device.equipmentType);
        devicesRoot->appendRow({deviceItem, deviceTypeItem});
    }
}
