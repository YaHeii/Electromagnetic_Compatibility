#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <QApplication>
#include <QComboBox>
#include <QStringList>

#include "Interface/DataModel.h"
#include "Interface/SchemaConstants.h"
#include "Resource/ui/DeviceWidget.h"
#include "Resource/ui/EnvironmentWidget.h"
#include "Resource/ui/deviceonship.h"
#include "Resource/ui/shipwidget.h"

namespace {

struct ScopedDataModelState {
    DataModel::DataSnapshot snapshot;

    ScopedDataModelState()
        : snapshot(DataModel::instance()->createSnapshot()) {}

    ~ScopedDataModelState() {
        restore(snapshot);
    }

    static void restore(const DataModel::DataSnapshot& snapshotToRestore) {
        DataModel* model = DataModel::instance();
        model->allEquipments = snapshotToRestore.allEquipments;
        model->allShips = snapshotToRestore.allShips;
        model->environmentConfig = snapshotToRestore.environmentConfig;
    }
};

QString schemaValue(const char* value) {
    return QString::fromLatin1(value);
}

QApplication& ensureApplication() {
    if (auto* existing = qobject_cast<QApplication*>(QCoreApplication::instance())) {
        return *existing;
    }

    qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));

    static int argc = 1;
    static char appName[] = "UiMainlineTests";
    static char* argv[] = {appName, nullptr};
    static QApplication app(argc, argv);
    return app;
}

void requireValid(const std::pair<bool, QString>& validationResult) {
    INFO(validationResult.second.toStdString());
    REQUIRE(validationResult.first);
}

void requireApprox(double actual, double expected) {
    REQUIRE(actual == Catch::Approx(expected));
}

EquipmentData makeReceiver() {
    EquipmentData equipment;
    equipment.equipmentId = "EQ_RX_1";
    equipment.equipmentType = schemaValue(SchemaValues::Receiver);
    equipment.gainDbi = -3.0;
    equipment.offsetX = 0.0;
    equipment.offsetY = 0.0;
    equipment.offsetZ = 1.0;
    equipment.receiverCenterFrequencyGHz = 1.0;
    equipment.receiverBandwidthMHz = 100.0;
    equipment.receiverSensitivityDbm = -75.0;
    equipment.receiverInterferenceMarginDb = 0.0;
    equipment.receiverSinrMarginDb = 0.0;
    equipment.receiverNoiseFigureDb = 3.0;
    return equipment;
}

EquipmentData makeTransmitter() {
    EquipmentData equipment;
    equipment.equipmentId = "EQ_TX_1";
    equipment.equipmentType = schemaValue(SchemaValues::Transmitter);
    equipment.gainDbi = 8.0;
    equipment.offsetX = 0.0;
    equipment.offsetY = 0.0;
    equipment.offsetZ = 1.0;
    equipment.transmitterCenterFrequencyGHz = 1.0;
    equipment.transmitterBandwidthMHz = 100.0;
    equipment.transmitterPowerDbm = -10.0;
    equipment.transmitterAntennaPhiDeg = 30.0;
    equipment.transmitterBeamWidthDeg = 20.0;
    equipment.transmitterPolarization = schemaValue(SchemaValues::Vertical);
    equipment.transmitterAntennaType = schemaValue(SchemaValues::Horn);
    return equipment;
}

EquipmentData makeTransceiver() {
    EquipmentData equipment;
    equipment.equipmentId = "EQ_TRX_1";
    equipment.equipmentType = schemaValue(SchemaValues::Transceiver);
    equipment.gainDbi = 5.0;
    equipment.offsetX = 0.0;
    equipment.offsetY = 0.0;
    equipment.offsetZ = 2.0;

    equipment.transmitterCenterFrequencyGHz = 5.8;
    equipment.transmitterBandwidthMHz = 40.0;
    equipment.transmitterPowerDbm = 20.0;
    equipment.transmitterAntennaPhiDeg = 15.0;
    equipment.transmitterBeamWidthDeg = 45.0;
    equipment.transmitterPolarization = schemaValue(SchemaValues::Vertical);
    equipment.transmitterAntennaType = schemaValue(SchemaValues::Directional);

    equipment.receiverCenterFrequencyGHz = 5.8;
    equipment.receiverBandwidthMHz = 40.0;
    equipment.receiverSensitivityDbm = -82.0;
    equipment.receiverInterferenceMarginDb = 3.0;
    equipment.receiverSinrMarginDb = 6.0;
    equipment.receiverNoiseFigureDb = 2.5;
    return equipment;
}

ShipData makeShip(const QString& equipmentId) {
    ShipData ship;
    ship.shipId = "USV1";
    ship.worldX = 100.0;
    ship.worldY = 200.0;
    ship.worldZ = 0.0;
    ship.shipSpeedMps = 6.0;
    ship.shipOrientationDeg = 90.0;
    ship.equipmentRefs.push_back(EquipmentOnShip{equipmentId, true});
    return ship;
}

ShipData makeWidgetShip() {
    ShipData ship;
    ship.shipId = "USV_WIDGET";
    ship.worldX = 12.0;
    ship.worldY = 34.0;
    ship.worldZ = 5.0;
    ship.shipSpeedMps = 7.5;
    ship.shipOrientationDeg = 123.0;
    ship.equipmentRefs.push_back(EquipmentOnShip{QStringLiteral("EQ_TX_WIDGET"), true});
    ship.equipmentRefs.push_back(EquipmentOnShip{QStringLiteral("EQ_TRX_WIDGET"), false});
    return ship;
}

EnvironmentData makeEnvironment() {
    EnvironmentData environment;
    environment.maxRange = 2000.0;
    environment.ductHeight = 20.0;
    environment.windSpeed = 7.0;
    environment.dx = 5.0;
    environment.dz = 0.1;
    environment.nz = 2048;
    environment.angleStepDeg = 5;
    return environment;
}

EnvironmentData makeSchemaDrivenEnvironment() {
    EnvironmentData environment;
    environment.maxRange = 4096.0;
    environment.ductHeight = 32.0;
    environment.windSpeed = 11.0;
    environment.dx = 2.5;
    environment.dz = 0.25;
    environment.nz = 1024;
    environment.angleStepDeg = 15;
    return environment;
}

void requireEquipmentBaseFieldsEqual(const EquipmentData& actual, const EquipmentData& expected) {
    REQUIRE(actual.equipmentId == expected.equipmentId);
    REQUIRE(actual.equipmentType == expected.equipmentType);
    requireApprox(actual.gainDbi, expected.gainDbi);
    requireApprox(actual.offsetX, expected.offsetX);
    requireApprox(actual.offsetY, expected.offsetY);
    requireApprox(actual.offsetZ, expected.offsetZ);
}

void requireShipBaseFieldsEqual(const ShipData& actual, const ShipData& expected) {
    REQUIRE(actual.shipId == expected.shipId);
    requireApprox(actual.worldX, expected.worldX);
    requireApprox(actual.worldY, expected.worldY);
    requireApprox(actual.worldZ, expected.worldZ);
    requireApprox(actual.shipSpeedMps, expected.shipSpeedMps);
    requireApprox(actual.shipOrientationDeg, expected.shipOrientationDeg);
}

QStringList comboItems(const QComboBox* comboBox) {
    QStringList items;
    for (int i = 0; i < comboBox->count(); ++i) {
        items.append(comboBox->itemText(i));
    }
    return items;
}

}  // namespace

TEST_CASE("DeviceItemWidget round-trips schema DTO values", "[schema][ui][device]") {
    ensureApplication();

    SECTION("transmitter fields stay aligned with SchemaConstants enums") {
        DeviceItemWidget widget;
        EquipmentData expected = makeTransmitter();
        expected.equipmentId = QStringLiteral("EQ_TX_WIDGET");
        expected.gainDbi = 13.5;
        expected.offsetX = 3.0;
        expected.offsetY = 4.0;
        expected.offsetZ = 5.0;
        expected.transmitterCenterFrequencyGHz = 9.4;
        expected.transmitterBandwidthMHz = 12.0;
        expected.transmitterPowerDbm = 28.0;
        expected.transmitterAntennaPhiDeg = 22.0;
        expected.transmitterBeamWidthDeg = 44.0;
        expected.transmitterPolarization = schemaValue(SchemaValues::Horizontal);
        expected.transmitterAntennaType = schemaValue(SchemaValues::Reflector);

        widget.setData(expected);
        const EquipmentData actual = widget.getData();

        requireEquipmentBaseFieldsEqual(actual, expected);
        requireApprox(actual.transmitterCenterFrequencyGHz, expected.transmitterCenterFrequencyGHz);
        requireApprox(actual.transmitterBandwidthMHz, expected.transmitterBandwidthMHz);
        requireApprox(actual.transmitterPowerDbm, expected.transmitterPowerDbm);
        requireApprox(actual.transmitterAntennaPhiDeg, expected.transmitterAntennaPhiDeg);
        requireApprox(actual.transmitterBeamWidthDeg, expected.transmitterBeamWidthDeg);
        REQUIRE(actual.transmitterPolarization == schemaValue(SchemaValues::Horizontal));
        REQUIRE(actual.transmitterAntennaType == schemaValue(SchemaValues::Reflector));
        requireValid(actual.validate());
    }

    SECTION("receiver fields stay aligned with schema DTO names") {
        DeviceItemWidget widget;
        EquipmentData expected = makeReceiver();
        expected.equipmentId = QStringLiteral("EQ_RX_WIDGET");
        expected.gainDbi = -2.0;
        expected.offsetX = -3.0;
        expected.offsetY = 6.0;
        expected.offsetZ = 7.0;
        expected.receiverCenterFrequencyGHz = 3.6;
        expected.receiverBandwidthMHz = 18.0;
        expected.receiverSensitivityDbm = -91.0;
        expected.receiverInterferenceMarginDb = 2.5;
        expected.receiverSinrMarginDb = 7.0;
        expected.receiverNoiseFigureDb = 1.8;

        widget.setData(expected);
        const EquipmentData actual = widget.getData();

        requireEquipmentBaseFieldsEqual(actual, expected);
        requireApprox(actual.receiverCenterFrequencyGHz, expected.receiverCenterFrequencyGHz);
        requireApprox(actual.receiverBandwidthMHz, expected.receiverBandwidthMHz);
        requireApprox(actual.receiverSensitivityDbm, expected.receiverSensitivityDbm);
        requireApprox(actual.receiverInterferenceMarginDb, expected.receiverInterferenceMarginDb);
        requireApprox(actual.receiverSinrMarginDb, expected.receiverSinrMarginDb);
        requireApprox(actual.receiverNoiseFigureDb, expected.receiverNoiseFigureDb);
        requireValid(actual.validate());
    }

    SECTION("transceiver preserves nested transmitter and receiver dto fields") {
        DeviceItemWidget widget;
        EquipmentData expected = makeTransceiver();
        expected.equipmentId = QStringLiteral("EQ_TRX_WIDGET");
        expected.gainDbi = 4.5;
        expected.offsetX = 8.0;
        expected.offsetY = 9.0;
        expected.offsetZ = 10.0;
        expected.transmitterPolarization = schemaValue(SchemaValues::Horizontal);
        expected.transmitterAntennaType = schemaValue(SchemaValues::Horn);
        expected.receiverSensitivityDbm = -79.5;
        expected.receiverInterferenceMarginDb = 1.25;

        widget.setData(expected);
        const EquipmentData actual = widget.getData();

        requireEquipmentBaseFieldsEqual(actual, expected);
        requireApprox(actual.transmitterCenterFrequencyGHz, expected.transmitterCenterFrequencyGHz);
        requireApprox(actual.transmitterBandwidthMHz, expected.transmitterBandwidthMHz);
        requireApprox(actual.transmitterPowerDbm, expected.transmitterPowerDbm);
        requireApprox(actual.transmitterAntennaPhiDeg, expected.transmitterAntennaPhiDeg);
        requireApprox(actual.transmitterBeamWidthDeg, expected.transmitterBeamWidthDeg);
        REQUIRE(actual.transmitterPolarization == schemaValue(SchemaValues::Horizontal));
        REQUIRE(actual.transmitterAntennaType == schemaValue(SchemaValues::Horn));
        requireApprox(actual.receiverCenterFrequencyGHz, expected.receiverCenterFrequencyGHz);
        requireApprox(actual.receiverBandwidthMHz, expected.receiverBandwidthMHz);
        requireApprox(actual.receiverSensitivityDbm, expected.receiverSensitivityDbm);
        requireApprox(actual.receiverInterferenceMarginDb, expected.receiverInterferenceMarginDb);
        requireApprox(actual.receiverSinrMarginDb, expected.receiverSinrMarginDb);
        requireApprox(actual.receiverNoiseFigureDb, expected.receiverNoiseFigureDb);
        requireValid(actual.validate());
    }
}

TEST_CASE("DeviceonShip preserves equipment reference DTO fields", "[schema][ui][deviceonship]") {
    ensureApplication();
    const ScopedDataModelState dataModelStateGuard;

    DataModel* model = DataModel::instance();
    EquipmentData transmitter = makeTransmitter();
    transmitter.equipmentId = QStringLiteral("EQ_TX_WIDGET");
    EquipmentData transceiver = makeTransceiver();
    transceiver.equipmentId = QStringLiteral("EQ_TRX_WIDGET");
    model->allEquipments = {transmitter, transceiver};

    DeviceonShip widget;
    const EquipmentOnShip expected{QStringLiteral("EQ_TRX_WIDGET"), false};
    widget.setData(expected);

    const EquipmentOnShip actual = widget.getData();
    REQUIRE(actual.equipmentId == expected.equipmentId);
    REQUIRE(actual.isEnabled == expected.isEnabled);
}

TEST_CASE("DeviceonShip refresh preserves current draft selection while loading new equipment ids", "[schema][ui][deviceonship][refresh]") {
    ensureApplication();
    const ScopedDataModelState dataModelStateGuard;

    DataModel* model = DataModel::instance();
    EquipmentData transmitter = makeTransmitter();
    transmitter.equipmentId = QStringLiteral("EQ_KEEP");
    model->allEquipments = {transmitter};

    DeviceonShip widget;
    widget.setData(EquipmentOnShip{QStringLiteral("EQ_KEEP"), true});

    EquipmentData receiver = makeReceiver();
    receiver.equipmentId = QStringLiteral("EQ_NEW");
    model->allEquipments = {receiver};
    widget.refreshEquipmentList();

    auto* comboBox = widget.findChild<QComboBox*>();
    REQUIRE(comboBox != nullptr);

    const QStringList items = comboItems(comboBox);
    REQUIRE(items.contains(QStringLiteral("EQ_KEEP")));
    REQUIRE(items.contains(QStringLiteral("EQ_NEW")));
    REQUIRE(comboBox->currentText() == QStringLiteral("EQ_KEEP"));
}

TEST_CASE("ShipItemWidget round-trips ship DTO fields and mounted equipment refs", "[schema][ui][ship]") {
    ensureApplication();
    const ScopedDataModelState dataModelStateGuard;

    DataModel* model = DataModel::instance();
    EquipmentData transmitter = makeTransmitter();
    transmitter.equipmentId = QStringLiteral("EQ_TX_WIDGET");
    EquipmentData transceiver = makeTransceiver();
    transceiver.equipmentId = QStringLiteral("EQ_TRX_WIDGET");
    model->allEquipments = {transmitter, transceiver};

    ShipItemWidget widget;
    const ShipData expected = makeWidgetShip();
    widget.setData(expected);

    const ShipData actual = widget.getData();
    requireShipBaseFieldsEqual(actual, expected);
    REQUIRE(actual.equipmentRefs.size() == expected.equipmentRefs.size());
    REQUIRE(actual.equipmentRefs[0].equipmentId == expected.equipmentRefs[0].equipmentId);
    REQUIRE(actual.equipmentRefs[0].isEnabled == expected.equipmentRefs[0].isEnabled);
    REQUIRE(actual.equipmentRefs[1].equipmentId == expected.equipmentRefs[1].equipmentId);
    REQUIRE(actual.equipmentRefs[1].isEnabled == expected.equipmentRefs[1].isEnabled);
    requireValid(actual.validateShip());
}

TEST_CASE("EnvironmentWidget round-trips environment data and saves it back into DataModel", "[schema][ui][environment]") {
    ensureApplication();
    const ScopedDataModelState dataModelStateGuard;

    const EnvironmentData expected = makeSchemaDrivenEnvironment();
    DataModel* model = DataModel::instance();
    ShipData shipWithoutEquipment;
    shipWithoutEquipment.shipId = "ENVIRONMENT_ONLY_SHIP";
    model->allShips = {shipWithoutEquipment};
    model->environmentConfig = expected;

    EnvironmentWidget widget;
    widget.loadFromModel();

    const EnvironmentData loaded = widget.getData();
    requireApprox(loaded.maxRange, expected.maxRange);
    requireApprox(loaded.ductHeight, expected.ductHeight);
    requireApprox(loaded.windSpeed, expected.windSpeed);
    requireApprox(loaded.dx, expected.dx);
    requireApprox(loaded.dz, expected.dz);
    REQUIRE(loaded.nz == expected.nz);
    REQUIRE(loaded.angleStepDeg == expected.angleStepDeg);
    REQUIRE_FALSE(widget.isDirty());

    model->environmentConfig = makeEnvironment();
    QString errorMessage;
    REQUIRE(widget.saveToModel(&errorMessage));
    requireApprox(model->environmentConfig.maxRange, expected.maxRange);
    requireApprox(model->environmentConfig.ductHeight, expected.ductHeight);
    requireApprox(model->environmentConfig.windSpeed, expected.windSpeed);
    requireApprox(model->environmentConfig.dx, expected.dx);
    requireApprox(model->environmentConfig.dz, expected.dz);
    REQUIRE(model->environmentConfig.nz == expected.nz);
    REQUIRE(model->environmentConfig.angleStepDeg == expected.angleStepDeg);
    REQUIRE_FALSE(widget.isDirty());
}

TEST_CASE("DeviceWidget can load existing model data and save without silently dropping it", "[schema][ui][devicepage]") {
    ensureApplication();
    const ScopedDataModelState dataModelStateGuard;

    DataModel* model = DataModel::instance();
    const EquipmentData transmitter = makeTransmitter();
    const EquipmentData receiver = makeReceiver();
    model->allEquipments = {transmitter, receiver};
    model->allShips = {makeShip(receiver.equipmentId)};
    model->environmentConfig = makeEnvironment();

    DeviceWidget widget;
    widget.loadFromModel();
    REQUIRE_FALSE(widget.isDirty());

    model->allEquipments.clear();
    QString errorMessage;
    REQUIRE(widget.saveToModel(&errorMessage));
    REQUIRE(model->allEquipments.size() == 2);
    REQUIRE(model->allEquipments[0].equipmentId == transmitter.equipmentId);
    REQUIRE(model->allEquipments[1].equipmentId == receiver.equipmentId);
    REQUIRE_FALSE(widget.isDirty());
}

TEST_CASE("ShipWidget can load existing model data and save without silently dropping it", "[schema][ui][shippage]") {
    ensureApplication();
    const ScopedDataModelState dataModelStateGuard;

    DataModel* model = DataModel::instance();
    EquipmentData transmitter = makeTransmitter();
    transmitter.equipmentId = QStringLiteral("EQ_SHIP_TX");
    model->allEquipments = {transmitter};

    ShipData expectedShip = makeShip(transmitter.equipmentId);
    expectedShip.shipId = "USV_SAVE_ROUNDTRIP";
    expectedShip.worldX = 321.0;
    expectedShip.worldY = 654.0;
    expectedShip.shipOrientationDeg = 210.0;
    model->allShips = {expectedShip};
    model->environmentConfig = makeEnvironment();

    ShipWidget widget;
    widget.loadFromModel();
    REQUIRE_FALSE(widget.isDirty());

    model->allShips.clear();
    QString errorMessage;
    REQUIRE(widget.saveToModel(&errorMessage));
    REQUIRE(model->allShips.size() == 1);
    requireShipBaseFieldsEqual(model->allShips.front(), expectedShip);
    REQUIRE(model->allShips.front().equipmentRefs.size() == 1);
    REQUIRE(model->allShips.front().equipmentRefs.front().equipmentId == transmitter.equipmentId);
    REQUIRE_FALSE(widget.isDirty());
}
