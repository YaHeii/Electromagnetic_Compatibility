#include <catch2/catch_test_macros.hpp>

#include "Interface/DataModel.h"
#include "Interface/SchemaConstants.h"
#include "ModelView/T_TreeViewModel.h"

namespace {

struct ScopedDataModelState {
    DataModel::DataSnapshot snapshot;

    ScopedDataModelState()
        : snapshot(DataModel::instance()->createSnapshot()) {}

    ~ScopedDataModelState() {
        DataModel* model = DataModel::instance();
        model->allEquipments = snapshot.allEquipments;
        model->allShips = snapshot.allShips;
        model->environmentConfig = snapshot.environmentConfig;
    }
};

QString schemaValue(const char* value) {
    return QString::fromLatin1(value);
}

EquipmentData makeTreeTransmitter(const QString& equipmentId) {
    EquipmentData equipment;
    equipment.equipmentId = equipmentId;
    equipment.equipmentType = schemaValue(SchemaValues::Transmitter);
    equipment.gainDbi = 12.5;
    equipment.offsetX = 1.0;
    equipment.offsetY = 2.0;
    equipment.offsetZ = 3.0;
    equipment.transmitterCenterFrequencyGHz = 1.5;
    equipment.transmitterBandwidthMHz = 20.0;
    equipment.transmitterPowerDbm = 30.0;
    equipment.transmitterAntennaPhiDeg = 10.0;
    equipment.transmitterBeamWidthDeg = 35.0;
    equipment.transmitterPolarization = schemaValue(SchemaValues::Vertical);
    equipment.transmitterAntennaType = schemaValue(SchemaValues::Horn);
    return equipment;
}

ShipData makeTreeShip(const QString& equipmentId) {
    ShipData ship;
    ship.shipId = "USV_TREE_01";
    ship.worldX = 120.0;
    ship.worldY = 240.0;
    ship.worldZ = 3.0;
    ship.shipSpeedMps = 8.0;
    ship.shipOrientationDeg = 135.0;
    ship.equipmentRefs.push_back(EquipmentOnShip{equipmentId, true});
    return ship;
}

QModelIndex firstChild(const QAbstractItemModel& model, const QModelIndex& parent, int row = 0) {
    return model.index(row, 0, parent);
}

QString displayText(const QAbstractItemModel& model, const QModelIndex& index) {
    return model.data(index, Qt::DisplayRole).toString();
}

}  // namespace

TEST_CASE("T_TreeViewModel reloads DataModel into three readonly top-level groups", "[ui][treeview][model]") {
    const ScopedDataModelState dataModelStateGuard;

    DataModel* model = DataModel::instance();
    model->environmentConfig.maxRange = 4096.0;
    model->environmentConfig.ductHeight = 18.0;
    model->environmentConfig.windSpeed = 6.5;
    model->environmentConfig.dx = 5.0;
    model->environmentConfig.dz = 0.2;
    model->environmentConfig.nz = 512;
    model->environmentConfig.angleStepDeg = 10;

    const QString equipmentId = QStringLiteral("EQ_TREE_TX_01");
    model->allEquipments = {makeTreeTransmitter(equipmentId)};
    model->allShips = {makeTreeShip(equipmentId)};

    T_TreeViewModel treeModel;
    treeModel.reloadFromDataModel();

    REQUIRE(treeModel.columnCount() == 1);
    REQUIRE(treeModel.rowCount() == 3);

    const QModelIndex environmentRoot = firstChild(treeModel, QModelIndex(), 0);
    const QModelIndex shipsRoot = firstChild(treeModel, QModelIndex(), 1);
    const QModelIndex equipmentsRoot = firstChild(treeModel, QModelIndex(), 2);

    REQUIRE(displayText(treeModel, environmentRoot) == QStringLiteral("环境参数"));
    REQUIRE(displayText(treeModel, shipsRoot) == QStringLiteral("船只列表"));
    REQUIRE(displayText(treeModel, equipmentsRoot) == QStringLiteral("设备库"));

    REQUIRE(treeModel.rowCount(environmentRoot) == 7);
    REQUIRE(treeModel.rowCount(shipsRoot) == 1);
    REQUIRE(treeModel.rowCount(equipmentsRoot) == 1);

    const QModelIndex shipIndex = firstChild(treeModel, shipsRoot);
    const QModelIndex equipmentIndex = firstChild(treeModel, equipmentsRoot);
    REQUIRE(displayText(treeModel, shipIndex) == QStringLiteral("USV_TREE_01"));
    REQUIRE(displayText(treeModel, equipmentIndex) == QStringLiteral("EQ_TREE_TX_01"));

    REQUIRE(treeModel.getItemCount() > 3);
}

TEST_CASE("T_TreeViewModel findItemIndex matches environment names ship ids and equipment ids", "[ui][treeview][find]") {
    const ScopedDataModelState dataModelStateGuard;

    DataModel* model = DataModel::instance();
    model->environmentConfig.maxRange = 1234.0;

    const QString equipmentId = QStringLiteral("EQ_TREE_FIND_01");
    model->allEquipments = {makeTreeTransmitter(equipmentId)};

    ShipData ship = makeTreeShip(equipmentId);
    ship.shipId = "USV_TREE_FIND_01";
    model->allShips = {ship};

    T_TreeViewModel treeModel;
    treeModel.reloadFromDataModel();

    REQUIRE(treeModel.findItemIndex(QStringLiteral("maxrange")).isValid());
    REQUIRE(treeModel.findItemIndex(QStringLiteral("tree_find_01")).isValid());
    REQUIRE(treeModel.findItemIndex(QStringLiteral("USV_TREE_FIND_01")).isValid());
    REQUIRE(treeModel.findItemIndex(QStringLiteral("missing_keyword")).isValid() == false);
}
