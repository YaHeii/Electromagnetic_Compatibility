#include "T_TreeViewModel.h"

#include "T_TreeItem.h"

namespace {

QString formatTriple(double x, double y, double z) {
    return QStringLiteral("(%1, %2, %3)")
        .arg(QString::number(x))
        .arg(QString::number(y))
        .arg(QString::number(z));
}

bool supportsTransmitterFields(const QString& type) {
    return type == DataModelSchemaValues::transmitterType() || type == DataModelSchemaValues::transceiverType();
}

bool supportsReceiverFields(const QString& type) {
    return type == DataModelSchemaValues::receiverType() || type == DataModelSchemaValues::transceiverType();
}

}  // namespace

T_TreeViewModel::T_TreeViewModel(QObject* parent)
    : QAbstractItemModel(parent),
      _rootItem(new T_TreeItem(QStringLiteral("root"), T_TreeItem::Root)) {}

T_TreeViewModel::~T_TreeViewModel()
{
    delete _rootItem;
}

QModelIndex T_TreeViewModel::parent(const QModelIndex& child) const
{
    if (!child.isValid()) {
        return QModelIndex();
    }

    T_TreeItem* childItem = getItemFromIndex(child);
    T_TreeItem* parentItem = childItem ? static_cast<T_TreeItem*>(childItem->getParentItem()) : nullptr;
    if (!parentItem || parentItem == _rootItem) {
        return QModelIndex();
    }

    return createIndex(parentItem->getRow(), 0, parentItem);
}

QModelIndex T_TreeViewModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent)) {
        return QModelIndex();
    }

    T_TreeItem* parentItem = parent.isValid() ? getItemFromIndex(parent) : _rootItem;
    if (!parentItem || row < 0 || row >= parentItem->getChildrenItems().count()) {
        return QModelIndex();
    }

    T_TreeItem* childItem = parentItem->getChildrenItems().at(row);
    return childItem ? createIndex(row, column, childItem) : QModelIndex();
}

int T_TreeViewModel::rowCount(const QModelIndex& parent) const
{
    if (parent.column() > 0) {
        return 0;
    }

    T_TreeItem* parentItem = parent.isValid() ? getItemFromIndex(parent) : _rootItem;
    return parentItem ? parentItem->getChildrenItems().count() : 0;
}

int T_TreeViewModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return 1;
}

QVariant T_TreeViewModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    T_TreeItem* item = getItemFromIndex(index);
    if (!item) {
        return QVariant();
    }

    if (role == Qt::DisplayRole || role == Qt::ToolTipRole) {
        return item->getItemTitle();
    }

    return QVariant();
}

Qt::ItemFlags T_TreeViewModel::flags(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant T_TreeViewModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (section == 0 && orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        return QStringLiteral("当前模型总览");
    }
    return QAbstractItemModel::headerData(section, orientation, role);
}

int T_TreeViewModel::getItemCount() const
{
    return countNodes(_rootItem);
}

void T_TreeViewModel::clear()
{
    beginResetModel();
    delete _rootItem;
    _rootItem = new T_TreeItem(QStringLiteral("root"), T_TreeItem::Root);
    endResetModel();
}

void T_TreeViewModel::reloadFromDataModel()
{
    beginResetModel();
    delete _rootItem;
    _rootItem = new T_TreeItem(QStringLiteral("root"), T_TreeItem::Root);

    const DataModel::DataSnapshot snapshot = DataModel::instance()->createSnapshot();
    populateEnvironment(appendChild(_rootItem, QStringLiteral("环境参数"), T_TreeItem::Fleet), snapshot.environmentConfig);
    populateShips(appendChild(_rootItem, QStringLiteral("船只列表"), T_TreeItem::Fleet), snapshot.allShips);
    populateEquipments(appendChild(_rootItem, QStringLiteral("设备库"), T_TreeItem::Fleet), snapshot.allEquipments);
    endResetModel();
}

QModelIndex T_TreeViewModel::findItemIndex(const QString& keyword) const
{
    const QString trimmedKeyword = keyword.trimmed();
    if (trimmedKeyword.isEmpty()) {
        return QModelIndex();
    }

    T_TreeItem* item = findFirstMatch(_rootItem, trimmedKeyword);
    if (!item || item == _rootItem) {
        return QModelIndex();
    }

    return createIndex(item->getRow(), 0, item);
}

T_TreeItem* T_TreeViewModel::getItemFromIndex(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return _rootItem;
    }
    return static_cast<T_TreeItem*>(index.internalPointer());
}

T_TreeItem* T_TreeViewModel::appendChild(T_TreeItem* parent, const QString& title, T_TreeItem::ItemType type)
{
    auto* item = new T_TreeItem(title, type, parent);
    parent->appendChildItem(item);
    return item;
}

void T_TreeViewModel::populateEnvironment(T_TreeItem* parent, const EnvironmentData& environment)
{
    appendChild(parent, QStringLiteral("maxRange: %1 m").arg(QString::number(environment.maxRange)), T_TreeItem::Device);
    appendChild(parent, QStringLiteral("ductHeight: %1 m").arg(QString::number(environment.ductHeight)), T_TreeItem::Device);
    appendChild(parent, QStringLiteral("windSpeed: %1 m/s").arg(QString::number(environment.windSpeed)), T_TreeItem::Device);
    appendChild(parent, QStringLiteral("dx: %1 m").arg(QString::number(environment.dx)), T_TreeItem::Device);
    appendChild(parent, QStringLiteral("dz: %1 m").arg(QString::number(environment.dz)), T_TreeItem::Device);
    appendChild(parent, QStringLiteral("nz: %1").arg(environment.nz), T_TreeItem::Device);
    appendChild(parent, QStringLiteral("angleStepDeg: %1 deg").arg(environment.angleStepDeg), T_TreeItem::Device);
}

void T_TreeViewModel::populateShips(T_TreeItem* parent, const std::vector<ShipData>& ships)
{
    for (const auto& ship : ships) {
        auto* shipItem = appendChild(parent, QString::fromStdString(ship.shipId), T_TreeItem::Ship);
        appendChild(shipItem, QStringLiteral("位置: %1").arg(formatTriple(ship.worldX, ship.worldY, ship.worldZ)), T_TreeItem::Device);
        appendChild(shipItem, QStringLiteral("速度: %1 m/s").arg(QString::number(ship.shipSpeedMps)), T_TreeItem::Device);
        appendChild(shipItem, QStringLiteral("朝向: %1 deg").arg(QString::number(ship.shipOrientationDeg)), T_TreeItem::Device);

        auto* refsItem = appendChild(shipItem, QStringLiteral("挂载设备"), T_TreeItem::Device);
        for (const auto& equipmentRef : ship.equipmentRefs) {
            const QString suffix = equipmentRef.isEnabled ? QString() : QStringLiteral(" [禁用]");
            appendChild(refsItem, QStringLiteral("%1%2").arg(equipmentRef.equipmentId, suffix), T_TreeItem::Device);
        }
    }
}

void T_TreeViewModel::populateEquipments(T_TreeItem* parent, const std::vector<EquipmentData>& equipments)
{
    for (const auto& equipment : equipments) {
        T_TreeItem::ItemType itemType = T_TreeItem::Device;
        if (equipment.equipmentType == DataModelSchemaValues::transmitterType()) {
            itemType = T_TreeItem::Tranmitter;
        } else if (equipment.equipmentType == DataModelSchemaValues::receiverType()) {
            itemType = T_TreeItem::Receiver;
        } else if (equipment.equipmentType == DataModelSchemaValues::transceiverType()) {
            itemType = T_TreeItem::Transceiver;
        }

        auto* equipmentItem = appendChild(parent, equipment.equipmentId, itemType);
        appendChild(equipmentItem, QStringLiteral("类型: %1").arg(equipment.equipmentType), T_TreeItem::Device);
        appendChild(equipmentItem, QStringLiteral("增益: %1 dBi").arg(QString::number(equipment.gainDbi)), T_TreeItem::Device);
        appendChild(
            equipmentItem,
            QStringLiteral("相对坐标: %1").arg(formatTriple(equipment.offsetX, equipment.offsetY, equipment.offsetZ)),
            T_TreeItem::Device);

        if (supportsTransmitterFields(equipment.equipmentType)) {
            appendChild(
                equipmentItem,
                QStringLiteral("发射中心频率: %1 GHz").arg(QString::number(equipment.transmitterCenterFrequencyGHz)),
                T_TreeItem::Device);
            appendChild(
                equipmentItem,
                QStringLiteral("发射带宽: %1 MHz").arg(QString::number(equipment.transmitterBandwidthMHz)),
                T_TreeItem::Device);
            appendChild(
                equipmentItem,
                QStringLiteral("发射功率: %1 dBm").arg(QString::number(equipment.transmitterPowerDbm)),
                T_TreeItem::Device);
            appendChild(
                equipmentItem,
                QStringLiteral("天线下倾角: %1 deg").arg(QString::number(equipment.transmitterAntennaPhiDeg)),
                T_TreeItem::Device);
            appendChild(
                equipmentItem,
                QStringLiteral("波束宽度: %1 deg").arg(QString::number(equipment.transmitterBeamWidthDeg)),
                T_TreeItem::Device);
            appendChild(equipmentItem, QStringLiteral("极化: %1").arg(equipment.transmitterPolarization), T_TreeItem::Device);
            appendChild(equipmentItem, QStringLiteral("天线类型: %1").arg(equipment.transmitterAntennaType), T_TreeItem::Device);
        }

        if (supportsReceiverFields(equipment.equipmentType)) {
            appendChild(
                equipmentItem,
                QStringLiteral("接收中心频率: %1 GHz").arg(QString::number(equipment.receiverCenterFrequencyGHz)),
                T_TreeItem::Device);
            appendChild(
                equipmentItem,
                QStringLiteral("接收带宽: %1 MHz").arg(QString::number(equipment.receiverBandwidthMHz)),
                T_TreeItem::Device);
            appendChild(
                equipmentItem,
                QStringLiteral("灵敏度: %1 dBm").arg(QString::number(equipment.receiverSensitivityDbm)),
                T_TreeItem::Device);
            appendChild(
                equipmentItem,
                QStringLiteral("干扰门限: %1 dB").arg(QString::number(equipment.receiverInterferenceMarginDb)),
                T_TreeItem::Device);
            appendChild(
                equipmentItem,
                QStringLiteral("SINR 裕量: %1 dB").arg(QString::number(equipment.receiverSinrMarginDb)),
                T_TreeItem::Device);
            appendChild(
                equipmentItem,
                QStringLiteral("噪声系数: %1 dB").arg(QString::number(equipment.receiverNoiseFigureDb)),
                T_TreeItem::Device);
        }
    }
}

int T_TreeViewModel::countNodes(const T_TreeItem* item) const
{
    if (!item) {
        return 0;
    }

    int total = 0;
    for (auto* child : item->getChildrenItems()) {
        total += 1;
        total += countNodes(child);
    }
    return total;
}

T_TreeItem* T_TreeViewModel::findFirstMatch(T_TreeItem* item, const QString& keyword) const
{
    if (!item) {
        return nullptr;
    }

    if (item != _rootItem && item->getItemTitle().contains(keyword, Qt::CaseInsensitive)) {
        return item;
    }

    for (auto* child : item->getChildrenItems()) {
        if (T_TreeItem* found = findFirstMatch(child, keyword)) {
            return found;
        }
    }

    return nullptr;
}
