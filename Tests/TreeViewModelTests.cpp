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
        model->emcAnalysisConfig = snapshot.emcAnalysisConfig;
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

EquipmentData makeTreeReceiver(const QString& equipmentId) {
    EquipmentData equipment;
    equipment.equipmentId = equipmentId;
    equipment.equipmentType = schemaValue(SchemaValues::Receiver);
    equipment.gainDbi = 4.0;
    equipment.offsetX = 1.0;
    equipment.offsetY = 0.0;
    equipment.offsetZ = 3.0;
    equipment.receiverCenterFrequencyGHz = 1.5;
    equipment.receiverBandwidthMHz = 20.0;
    equipment.receiverSensitivityDbm = -85.0;
    equipment.receiverInterferenceMarginDb = -10.0;
    equipment.receiverSinrMarginDb = 0.0;
    equipment.receiverNoiseFigureDb = 3.0;
    return equipment;
}

ShipData makeTreeShip(const std::string& shipId, double worldX, const QString& equipmentId) {
    ShipData ship;
    ship.shipId = shipId;
    ship.worldX = worldX;
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
    model->emcAnalysisConfig.fieldPlaneHeightM = 12.0;
    model->emcAnalysisConfig.referenceTransmitterId = QStringLiteral("EQ_TREE_TX_01");
    model->emcAnalysisConfig.referenceReceiverId = QStringLiteral("EQ_TREE_RX_01");
    model->emcAnalysisConfig.s3iBaselineWindSpeedMps = 0.5;

    model->allEquipments = {
        makeTreeTransmitter(QStringLiteral("EQ_TREE_TX_01")),
        makeTreeReceiver(QStringLiteral("EQ_TREE_RX_01")),
    };
    model->allShips = {
        makeTreeShip("USV_TREE_01", 120.0, QStringLiteral("EQ_TREE_TX_01")),
        makeTreeShip("USV_TREE_02", 240.0, QStringLiteral("EQ_TREE_RX_01")),
    };

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

    REQUIRE(treeModel.rowCount(environmentRoot) == 8);
    REQUIRE(treeModel.rowCount(shipsRoot) == 2);
    REQUIRE(treeModel.rowCount(equipmentsRoot) == 2);

    const QModelIndex shipIndex = firstChild(treeModel, shipsRoot);
    const QModelIndex equipmentIndex = firstChild(treeModel, equipmentsRoot);
    REQUIRE(displayText(treeModel, shipIndex) == QStringLiteral("USV_TREE_01"));
    REQUIRE(displayText(treeModel, equipmentIndex) == QStringLiteral("EQ_TREE_TX_01"));

    const QModelIndex analysisGroup = firstChild(treeModel, environmentRoot, 7);
    REQUIRE(displayText(treeModel, analysisGroup) == QStringLiteral("EMC 分析配置"));
    REQUIRE(treeModel.rowCount(analysisGroup) == 4);

    REQUIRE(treeModel.getItemCount() > 3);
}

TEST_CASE("T_TreeViewModel findItemIndex matches environment names ship ids and equipment ids", "[ui][treeview][find]") {
    const ScopedDataModelState dataModelStateGuard;

    DataModel* model = DataModel::instance();
    model->environmentConfig.maxRange = 1234.0;
    model->emcAnalysisConfig.fieldPlaneHeightM = 14.0;
    model->emcAnalysisConfig.referenceTransmitterId = QStringLiteral("EQ_TREE_FIND_01");
    model->emcAnalysisConfig.referenceReceiverId = QStringLiteral("EQ_TREE_FIND_RX");
    model->emcAnalysisConfig.s3iBaselineWindSpeedMps = 0.5;
    model->allEquipments = {
        makeTreeTransmitter(QStringLiteral("EQ_TREE_FIND_01")),
        makeTreeReceiver(QStringLiteral("EQ_TREE_FIND_RX")),
    };
    model->allShips = {
        makeTreeShip("USV_TREE_FIND_01", 120.0, QStringLiteral("EQ_TREE_FIND_01")),
        makeTreeShip("USV_TREE_FIND_02", 180.0, QStringLiteral("EQ_TREE_FIND_RX")),
    };

    T_TreeViewModel treeModel;
    treeModel.reloadFromDataModel();

    REQUIRE(treeModel.findItemIndex(QStringLiteral("maxrange")).isValid());
    REQUIRE(treeModel.findItemIndex(QStringLiteral("tree_find_01")).isValid());
    REQUIRE(treeModel.findItemIndex(QStringLiteral("referenceTransmitterId")).isValid());
    REQUIRE(treeModel.findItemIndex(QStringLiteral("USV_TREE_FIND_01")).isValid());
    REQUIRE_FALSE(treeModel.findItemIndex(QStringLiteral("missing_keyword")).isValid());
}
