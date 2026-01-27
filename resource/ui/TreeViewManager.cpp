#include "TreeViewManager.h"
#include <QTreeView>
#include <QHeaderView>

// TreeViewManager Implementation
TreeViewManager::TreeViewManager(QTreeView* treeView, QObject* parent)
    : QObject(parent), _treeView(treeView)
{
    setupTreeView();
}

TreeViewManager::~TreeViewManager()
{
}

void TreeViewManager::setupTreeView()
{
    _model = new FleetTreeViewModel(this);
    _treeView->setModel(_model);

    // 设置列宽
    _treeView->header()->setStretchLastSection(true);
    _treeView->header()->resizeSection(0, 200);

    // 连接信号
    connect(_treeView, &QTreeView::clicked, this, &TreeViewManager::onItemClicked);
}

void TreeViewManager::syncViewWithModel()
{
    _model->clear();

    // 重新添加舰队和设备库根节点
    FleetTreeItem* fleetRoot = new FleetTreeItem("舰船编队", FleetTreeItem::Fleet, "", static_cast<FleetTreeItem*>(_model->_rootItem));
    FleetTreeItem* deviceRoot = new FleetTreeItem("设备库", FleetTreeItem::Fleet, "", static_cast<FleetTreeItem*>(_model->_rootItem));

    _model->_rootItem->appendChildItem(fleetRoot);
    _model->_rootItem->appendChildItem(deviceRoot);

    populateShips(fleetRoot);
    populateDevices(deviceRoot);

    // 展开顶层节点
    _treeView->expand(_model->index(0, 0));
    _treeView->expand(_model->index(1, 0));
}

void TreeViewManager::expandAll()
{
    _treeView->expandAll();
}

void TreeViewManager::collapseAll()
{
    _treeView->collapseAll();
}

QString TreeViewManager::getSelectedItemId() const
{
    QModelIndex currentIndex = _treeView->currentIndex();
    if (currentIndex.isValid()) {
        FleetTreeItem* item = _model->getItemFromIndex(currentIndex);
        return item->getDataId();
    }
    return QString();
}

QModelIndex TreeViewManager::findItemIndex(const QString& itemId) const
{
    return _model->findItemIndex(itemId);
}

void TreeViewManager::onItemClicked(const QModelIndex& index)
{
    FleetTreeItem* item = _model->getItemFromIndex(index);
    if (item) {
        // 可以在这里处理点击事件
        qDebug() << "Clicked item:" << item->getItemTitle() << "Type:" << item->getItemType() << "ID:" << item->getDataId();
    }
}

void TreeViewManager::populateShips(FleetTreeItem* shipsRoot)
{
    const std::vector<ShipData>& ships = DataModel::instance()->allShips;

    for (const ShipData& ship : ships) {
        FleetTreeItem* shipItem = new FleetTreeItem(QString::fromStdString(ship.shipID), FleetTreeItem::Ship, QString::fromStdString(ship.shipID), shipsRoot);
        shipsRoot->appendChildItem(shipItem);

        // 添加配置的设备
        for (const EquipmentOnShip& equipment : ship.Equipments) {
            FleetTreeItem* deviceItem = new FleetTreeItem(equipment.equipmentID, FleetTreeItem::Device, equipment.equipmentID, shipItem);
            shipItem->appendChildItem(deviceItem);
        }
    }
}

void TreeViewManager::populateDevices(FleetTreeItem* devicesRoot)
{
    const std::vector<EquipmentData>& devices = DataModel::instance()->allEquipments;

    for (const EquipmentData& device : devices) {
        FleetTreeItem* deviceItem = new FleetTreeItem(device.equipmentID, FleetTreeItem::Device, device.equipmentID, devicesRoot);
        devicesRoot->appendChildItem(deviceItem);
    }
}

// FleetTreeItem Implementation
FleetTreeItem::FleetTreeItem(const QString& title, ItemType type, const QString& dataId, FleetTreeItem* parent)
    : T_TreeItem(title, parent)
{
    _pItemType = type;
    _pDataId = dataId;
}

// FleetTreeViewModel Implementation
FleetTreeViewModel::FleetTreeViewModel(QObject* parent)
    : QAbstractItemModel(parent)
{
    _rootItem = new FleetTreeItem("Root", FleetTreeItem::Root);
}

FleetTreeViewModel::~FleetTreeViewModel()
{
    delete _rootItem;
}

QModelIndex FleetTreeViewModel::parent(const QModelIndex& child) const
{
    if (!child.isValid())
        return QModelIndex();
    
    FleetTreeItem* childItem = getItemFromIndex(child);
    FleetTreeItem* parentItem = static_cast<FleetTreeItem*>(childItem->getParentItem());
    
    if (parentItem == _rootItem || parentItem == nullptr)
        return QModelIndex();
    
    return createIndex(parentItem->getRow(), 0, parentItem);
}

QModelIndex FleetTreeViewModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();
    
    FleetTreeItem* parentItem = !parent.isValid() ? _rootItem : getItemFromIndex(parent);
    
    if (row >= parentItem->getChildrenItems().count())
        return QModelIndex();
    
    FleetTreeItem* childItem = static_cast<FleetTreeItem*>(parentItem->getChildrenItems().at(row));
    return createIndex(row, column, childItem);
}

int FleetTreeViewModel::rowCount(const QModelIndex& parent) const
{
    if (parent.column() > 0)
        return 0;
    
    FleetTreeItem* parentItem = !parent.isValid() ? _rootItem : getItemFromIndex(parent);
    return parentItem->getChildrenItems().count();
}

int FleetTreeViewModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return 2; // 名称和ID/类型
}

QVariant FleetTreeViewModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();
    
    FleetTreeItem* item = getItemFromIndex(index);
    
    if (role == Qt::DisplayRole) {
        if (index.column() == 0) {
            return item->getItemTitle();
        } else if (index.column() == 1) {
            switch (item->getItemType()) {
                case FleetTreeItem::Root:
                    return "";
                case FleetTreeItem::Fleet:
                    return "舰队";
                case FleetTreeItem::Ship:
                    return QString("ID: %1").arg(item->getDataId());
                case FleetTreeItem::Device:
                    return QString("设备: %1").arg(item->getDataId());
            }
        }
    }
    
    return QVariant();
}

bool FleetTreeViewModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    Q_UNUSED(index)
    Q_UNUSED(value)
    Q_UNUSED(role)
    return false;
}

Qt::ItemFlags FleetTreeViewModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;
    
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant FleetTreeViewModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        return section == 0 ? "名称" : "ID/类型";
    }
    return QVariant();
}

void FleetTreeViewModel::clear()
{
    beginResetModel();
    delete _rootItem;
    _rootItem = new FleetTreeItem("Root", FleetTreeItem::Root);
    endResetModel();
}

void FleetTreeViewModel::addShip(const ShipData& ship)
{
    FleetTreeItem* fleetItem = static_cast<FleetTreeItem*>(_rootItem->getChildrenItems().first());
    if (!fleetItem || fleetItem->getItemType() != FleetTreeItem::Fleet) {
        // 创建舰队根节点
        fleetItem = new FleetTreeItem("舰船编队", FleetTreeItem::Fleet, "", _rootItem);
        beginInsertRows(QModelIndex(), _rootItem->getChildrenItems().count(), _rootItem->getChildrenItems().count());
        _rootItem->appendChildItem(fleetItem);
        endInsertRows();
    }
    
    FleetTreeItem* shipItem = new FleetTreeItem(QString::fromStdString(ship.shipID), FleetTreeItem::Ship, QString::fromStdString(ship.shipID), fleetItem);
    
    beginInsertRows(createIndex(fleetItem->getRow(), 0, fleetItem), fleetItem->getChildrenItems().count(), fleetItem->getChildrenItems().count());
    fleetItem->appendChildItem(shipItem);
    
    // 添加配置的设备
    for (const auto& equipment : ship.Equipments) {
        FleetTreeItem* deviceItem = new FleetTreeItem(equipment.equipmentID, FleetTreeItem::Device, equipment.equipmentID, shipItem);
        shipItem->appendChildItem(deviceItem);
    }
    
    endInsertRows();
}

void FleetTreeViewModel::addDevice(const EquipmentData& device)
{
    FleetTreeItem* devicesRoot = nullptr;
    
    // 查找或创建设备库根节点
    for (auto child : _rootItem->getChildrenItems()) {
        FleetTreeItem* item = static_cast<FleetTreeItem*>(child);
        if (item->getItemType() == FleetTreeItem::Device && item->getItemTitle() == "设备库") {
            devicesRoot = item;
            break;
        }
    }
    
    if (!devicesRoot) {
        devicesRoot = new FleetTreeItem("设备库", FleetTreeItem::Fleet, "", _rootItem);
        beginInsertRows(QModelIndex(), _rootItem->getChildrenItems().count(), _rootItem->getChildrenItems().count());
        _rootItem->appendChildItem(devicesRoot);
        endInsertRows();
    }
    
    FleetTreeItem* deviceItem = new FleetTreeItem(device.equipmentID, FleetTreeItem::Device, device.equipmentID, devicesRoot);
    
    beginInsertRows(createIndex(devicesRoot->getRow(), 0, devicesRoot), devicesRoot->getChildrenItems().count(), devicesRoot->getChildrenItems().count());
    devicesRoot->appendChildItem(deviceItem);
    endInsertRows();
}

void FleetTreeViewModel::removeItem(const QString& itemId)
{
    FleetTreeItem* item = findItemById(_rootItem, itemId);
    if (item && item->getParentItem()) {
        FleetTreeItem* parent = static_cast<FleetTreeItem*>(item->getParentItem());
        int row = item->getRow();
        
        beginRemoveRows(createIndex(parent->getRow(), 0, parent), row, row);
        parent->getChildrenItems().removeAt(row);
        delete item;
        endRemoveRows();
    }
}

QModelIndex FleetTreeViewModel::findItemIndex(const QString& itemId) const
{
    FleetTreeItem* item = findItemById(_rootItem, itemId);
    if (item && item->getParentItem()) {
        FleetTreeItem* parent = static_cast<FleetTreeItem*>(item->getParentItem());
        return createIndex(item->getRow(), 0, item);
    }
    return QModelIndex();
}

FleetTreeItem* FleetTreeViewModel::getItemFromIndex(const QModelIndex& index) const
{
    if (!index.isValid())
        return _rootItem;
    
    return static_cast<FleetTreeItem*>(index.internalPointer());
}

FleetTreeItem* FleetTreeViewModel::findItemById(FleetTreeItem* item, const QString& itemId) const
{
    if (item->getDataId() == itemId)
        return item;
    
    for (auto child : item->getChildrenItems()) {
        FleetTreeItem* found = findItemById(static_cast<FleetTreeItem*>(child), itemId);
        if (found)
            return found;
    }
    
    return nullptr;
}
