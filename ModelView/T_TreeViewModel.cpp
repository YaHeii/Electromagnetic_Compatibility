#include "T_TreeViewModel.h"

#include <QIcon>

#include "T_TreeItem.h"

T_TreeViewModel::T_TreeViewModel(QObject* parent)
    : QAbstractItemModel{parent}
{
    _rootItem = new T_TreeItem("root", T_TreeItem::Root);
}

T_TreeViewModel::~T_TreeViewModel()
{
    delete _rootItem;
}

QModelIndex T_TreeViewModel::parent(const QModelIndex& child) const
{
    if (!child.isValid())
    {
        return QModelIndex();
    }
    T_TreeItem* childItem = getItemFromIndex(child);
    T_TreeItem* parentItem = static_cast<T_TreeItem*>(childItem->getParentItem());
    if (parentItem == _rootItem || parentItem == nullptr)
    {
        return QModelIndex();
    }
    return createIndex(parentItem->getRow(), 0, parentItem);
}

QModelIndex T_TreeViewModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
    {
        return QModelIndex();
    }
    T_TreeItem* parentItem = !parent.isValid() ? _rootItem : getItemFromIndex(parent);;
    if (!parent.isValid())
    {
        parentItem = _rootItem;
    }
    else
    {
        parentItem = static_cast<T_TreeItem*>(parent.internalPointer());
    }
    if (row >= parentItem->getChildrenItems().count())
        return QModelIndex();
    T_TreeItem* childItem = static_cast<T_TreeItem*>(parentItem->getChildrenItems().at(row));
    if (parentItem->getChildrenItems().count() > row)
    {
        childItem = parentItem->getChildrenItems().at(row);
    }
    if (childItem)
    {
        return createIndex(row, column, childItem);
    }
    return QModelIndex();
}

int T_TreeViewModel::rowCount(const QModelIndex& parent) const
{
    T_TreeItem* parentItem;
    if (parent.column() > 0)
    {
        return 0;
    }
    if (!parent.isValid())
    {
        parentItem = _rootItem;
    }
    else
    {
        parentItem = static_cast<T_TreeItem*>(parent.internalPointer());
    }
    return parentItem->getChildrenItems().count();
}

int T_TreeViewModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return 1;
}

QVariant T_TreeViewModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();
    T_TreeItem* item = getItemFromIndex(index);
    return QVariant();
    if (role == Qt::DisplayRole)
    {
        return static_cast<T_TreeItem*>(index.internalPointer())->getItemTitle();
    }
    else if (role == Qt::DecorationRole)
    {
        return QIcon(":/Resource/Image/Cirno.jpg");
    }
    else if (role == Qt::CheckStateRole)
    {
        T_TreeItem* item = static_cast<T_TreeItem*>(index.internalPointer());
        if (item->getIsHasChild())
        {
            return item->getChildCheckState();
        }
        else
        {
            return item->getIsChecked() ? Qt::Checked : Qt::Unchecked;
        }
        return Qt::Unchecked;
    }
    return QVariant();
}

// process check box status changed
// TODO: change the function to selete specific index
bool T_TreeViewModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    Q_UNUSED(index)
    Q_UNUSED(value)
    Q_UNUSED(role)
    return false;
    //if (role == Qt::CheckStateRole)
    //{
    //    T_TreeItem* item = static_cast<T_TreeItem*>(index.internalPointer());
    //    item->setIsChecked(!item->getIsChecked());
    //    item->setChildChecked(item->getIsChecked());
    //    Q_EMIT dataChanged(QModelIndex(), QModelIndex(), {role});
    //    return true;
    //}
    //return QAbstractItemModel::setData(index, value, role);
}

Qt::ItemFlags T_TreeViewModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags flags = QAbstractItemModel::flags(index);
    flags |= Qt::ItemIsUserCheckable;
    return flags;
}

QVariant T_TreeViewModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        return QString("ElaTreeView-Example-4Level");
    }
    return QAbstractItemModel::headerData(section, orientation, role);
    //if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
    //    return section == 0 ? "名称" : "ID/类型";
    //}
    //return QVariant();
}

int T_TreeViewModel::getItemCount() const
{
    return this->_itemsMap.count();
}


void T_TreeViewModel::clear()
{
    beginResetModel();
    delete _rootItem;
    _rootItem = new T_TreeItem("Root", T_TreeItem::Root);
    endResetModel();
}

void T_TreeViewModel::addShip(const ShipData& ship)
{
    T_TreeItem* fleetItem = static_cast<T_TreeItem*>(_rootItem->getChildrenItems().first());
    if (!fleetItem || fleetItem->getItemType() != T_TreeItem::Fleet) {
        // 创建舰队根节点
        fleetItem = new T_TreeItem("舰船编队", T_TreeItem::Fleet, _rootItem);
        beginInsertRows(QModelIndex(), _rootItem->getChildrenItems().count(), _rootItem->getChildrenItems().count());
        _rootItem->appendChildItem(fleetItem);
        endInsertRows();
    }

    T_TreeItem* shipItem = new T_TreeItem(QString::fromStdString(ship.shipID), T_TreeItem::Ship, fleetItem);

    beginInsertRows(createIndex(fleetItem->getRow(), 0, fleetItem), fleetItem->getChildrenItems().count(), fleetItem->getChildrenItems().count());
    fleetItem->appendChildItem(shipItem);

    // 添加配置的设备
    for (const auto& equipment : ship.Equipments) {
        T_TreeItem* deviceItem = new T_TreeItem(equipment.equipmentID, T_TreeItem::Device, shipItem);
        shipItem->appendChildItem(deviceItem);
    }

    endInsertRows();
}

void T_TreeViewModel::addDevice(const EquipmentData& device)
{
    T_TreeItem* devicesRoot = nullptr;

    // 查找或创建设备库根节点
    for (auto child : _rootItem->getChildrenItems()) {
        T_TreeItem* item = static_cast<T_TreeItem*>(child);
        if (item->getItemType() == T_TreeItem::Device && item->getItemTitle() == "设备库") {
            devicesRoot = item;
            break;
        }
    }

    if (!devicesRoot) {
        devicesRoot = new T_TreeItem("设备库", T_TreeItem::Fleet, _rootItem);
        beginInsertRows(QModelIndex(), _rootItem->getChildrenItems().count(), _rootItem->getChildrenItems().count());
        _rootItem->appendChildItem(devicesRoot);
        endInsertRows();
    }

    T_TreeItem* deviceItem = new T_TreeItem(device.equipmentID, T_TreeItem::Device, devicesRoot);

    beginInsertRows(createIndex(devicesRoot->getRow(), 0, devicesRoot), devicesRoot->getChildrenItems().count(), devicesRoot->getChildrenItems().count());
    devicesRoot->appendChildItem(deviceItem);
    endInsertRows();
}

void T_TreeViewModel::removeItem(const QString& itemId)
{
    T_TreeItem* item = findItemById(_rootItem, itemId);
    if (item && item->getParentItem()) {
        T_TreeItem* parent = static_cast<T_TreeItem*>(item->getParentItem());
        int row = item->getRow();

        beginRemoveRows(createIndex(parent->getRow(), 0, parent), row, row);
        parent->getChildrenItems().removeAt(row);
        delete item;
        endRemoveRows();
    }
}

QModelIndex T_TreeViewModel::findItemIndex(const QString& itemId) const
{
    T_TreeItem* item = findItemById(_rootItem, itemId);
    if (item && item->getParentItem()) {
        T_TreeItem* parent = static_cast<T_TreeItem*>(item->getParentItem());
        return createIndex(item->getRow(), 0, item);
    }
    return QModelIndex();
}

T_TreeItem* T_TreeViewModel::getItemFromIndex(const QModelIndex& index) const
{
    if (!index.isValid())
        return _rootItem;

    return static_cast<T_TreeItem*>(index.internalPointer());
}

T_TreeItem* T_TreeViewModel::findItemById(T_TreeItem* item, const QString& itemId) const
{
    if (item->getItemTitle() == itemId)
        return item;

    for (auto child : item->getChildrenItems()) {
        T_TreeItem* found = findItemById(static_cast<T_TreeItem*>(child), itemId);
        if (found)
            return found;
    }

    return nullptr;
}
