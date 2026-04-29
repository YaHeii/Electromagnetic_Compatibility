#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryFile>

#include "Interface/DataModel.h"
#include "Interface/SchemaConstants.h"
#include "Simulation/EMC_Engine.h"
#include "Utils/JsonLoader.hpp"

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

QString schemaKey(const char* key) {
    return QString::fromLatin1(key);
}

QString schemaValue(const char* value) {
    return QString::fromLatin1(value);
}

void requireValid(const std::pair<bool, QString>& validationResult) {
    INFO(validationResult.second.toStdString());
    REQUIRE(validationResult.first);
}

void requireInvalid(const std::pair<bool, QString>& validationResult) {
    INFO(validationResult.second.toStdString());
    REQUIRE_FALSE(validationResult.first);
}

void requireApprox(double actual, double expected) {
    REQUIRE(actual == Catch::Approx(expected));
}

QJsonArray makeVector3(double x, double y, double z) {
    QJsonArray array;
    array.append(x);
    array.append(y);
    array.append(z);
    return array;
}

QJsonObject makePoint3D(double x, double y, double z) {
    QJsonObject object;
    object.insert(schemaKey(SchemaKeys::Type), schemaValue(SchemaValues::Point3D));
    object.insert(schemaKey(SchemaKeys::Coordinates), makeVector3(x, y, z));
    return object;
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

QJsonObject makeEnvironmentJson(const EnvironmentData& environment) {
    QJsonObject object;
    object.insert(schemaKey(SchemaKeys::MaxRange), environment.maxRange);
    object.insert(schemaKey(SchemaKeys::DuctHeight), environment.ductHeight);
    object.insert(schemaKey(SchemaKeys::WindSpeed), environment.windSpeed);
    object.insert(schemaKey(SchemaKeys::Dx), environment.dx);
    object.insert(schemaKey(SchemaKeys::Dz), environment.dz);
    object.insert(schemaKey(SchemaKeys::Nz), environment.nz);
    object.insert(schemaKey(SchemaKeys::AngleStepDeg), environment.angleStepDeg);
    return object;
}

QJsonObject makeTransmitterJson(const QString& id) {
    QJsonObject object;
    object.insert(schemaKey(SchemaKeys::ID), id);
    object.insert(schemaKey(SchemaKeys::Type), schemaValue(SchemaValues::Transmitter));
    object.insert(schemaKey(SchemaKeys::GainDbi), 9.5);
    object.insert(schemaKey(SchemaKeys::LocationOffset), makeVector3(1.0, 2.0, 3.0));
    object.insert(schemaKey(SchemaKeys::CenterFrequencyGHz), 1.25);
    object.insert(schemaKey(SchemaKeys::BandwidthMHz), 25.0);
    object.insert(schemaKey(SchemaKeys::PowerDbm), 18.0);
    object.insert(schemaKey(SchemaKeys::AntennaPhiDeg), 40.0);
    object.insert(schemaKey(SchemaKeys::BeamWidthDeg), 35.0);
    object.insert(schemaKey(SchemaKeys::Polarization), schemaValue(SchemaValues::Horizontal));
    object.insert(schemaKey(SchemaKeys::AntennaType), schemaValue(SchemaValues::Reflector));
    return object;
}

QJsonObject makeReceiverJson(const QString& id) {
    QJsonObject object;
    object.insert(schemaKey(SchemaKeys::ID), id);
    object.insert(schemaKey(SchemaKeys::Type), schemaValue(SchemaValues::Receiver));
    object.insert(schemaKey(SchemaKeys::GainDbi), -1.5);
    object.insert(schemaKey(SchemaKeys::LocationOffset), makeVector3(-1.0, 0.5, 4.0));
    object.insert(schemaKey(SchemaKeys::CenterFrequencyGHz), 2.45);
    object.insert(schemaKey(SchemaKeys::BandwidthMHz), 12.5);
    object.insert(schemaKey(SchemaKeys::SensitivityDbm), -88.0);
    object.insert(schemaKey(SchemaKeys::InterferenceMarginDb), 1.5);
    object.insert(schemaKey(SchemaKeys::SinrMarginDb), 4.5);
    object.insert(schemaKey(SchemaKeys::NoiseFigureDb), 2.0);
    return object;
}

QJsonObject makeTransceiverJson(const QString& id) {
    QJsonObject transmitter;
    transmitter.insert(schemaKey(SchemaKeys::CenterFrequencyGHz), 5.8);
    transmitter.insert(schemaKey(SchemaKeys::BandwidthMHz), 40.0);
    transmitter.insert(schemaKey(SchemaKeys::PowerDbm), 22.0);
    transmitter.insert(schemaKey(SchemaKeys::AntennaPhiDeg), 12.0);
    transmitter.insert(schemaKey(SchemaKeys::BeamWidthDeg), 60.0);
    transmitter.insert(schemaKey(SchemaKeys::Polarization), schemaValue(SchemaValues::Vertical));
    transmitter.insert(schemaKey(SchemaKeys::AntennaType), schemaValue(SchemaValues::Directional));

    QJsonObject receiver;
    receiver.insert(schemaKey(SchemaKeys::CenterFrequencyGHz), 5.8);
    receiver.insert(schemaKey(SchemaKeys::BandwidthMHz), 20.0);
    receiver.insert(schemaKey(SchemaKeys::SensitivityDbm), -81.0);
    receiver.insert(schemaKey(SchemaKeys::InterferenceMarginDb), 2.0);
    receiver.insert(schemaKey(SchemaKeys::SinrMarginDb), 5.5);
    receiver.insert(schemaKey(SchemaKeys::NoiseFigureDb), 2.2);

    QJsonObject object;
    object.insert(schemaKey(SchemaKeys::ID), id);
    object.insert(schemaKey(SchemaKeys::Type), schemaValue(SchemaValues::Transceiver));
    object.insert(schemaKey(SchemaKeys::GainDbi), 6.5);
    object.insert(schemaKey(SchemaKeys::LocationOffset), makeVector3(0.0, 0.0, 6.0));
    object.insert(schemaKey(SchemaKeys::TransmitterConfig), transmitter);
    object.insert(schemaKey(SchemaKeys::ReceiverConfig), receiver);
    return object;
}

QJsonObject makeSchemaDrivenRootJson() {
    const EnvironmentData environment = makeSchemaDrivenEnvironment();

    QJsonArray transmitters;
    transmitters.append(makeTransmitterJson(QStringLiteral("TX_SCHEMA_1")));

    QJsonArray receivers;
    receivers.append(makeReceiverJson(QStringLiteral("RX_SCHEMA_1")));

    QJsonArray transceivers;
    transceivers.append(makeTransceiverJson(QStringLiteral("TRX_SCHEMA_1")));

    QJsonObject ship;
    ship.insert(schemaKey(SchemaKeys::ID), QStringLiteral("USV_SCHEMA_1"));
    ship.insert(schemaKey(SchemaKeys::Location), makePoint3D(120.0, 45.0, 3.0));
    ship.insert(schemaKey(SchemaKeys::Speed), 9.0);
    ship.insert(schemaKey(SchemaKeys::ShipOrientationDeg), 135.0);
    ship.insert(schemaKey(SchemaKeys::Transmitters), transmitters);
    ship.insert(schemaKey(SchemaKeys::Receivers), receivers);
    ship.insert(schemaKey(SchemaKeys::Transceivers), transceivers);

    QJsonArray usvs;
    usvs.append(ship);

    QJsonObject root;
    root.insert(schemaKey(SchemaKeys::SchemaVersion), schemaValue(SchemaValues::SchemaVersion_1_0_0));
    root.insert(schemaKey(SchemaKeys::Environment), makeEnvironmentJson(environment));
    root.insert(schemaKey(SchemaKeys::Usvs), usvs);
    return root;
}

bool writeJsonToTempFile(const QJsonObject& rootObject, QTemporaryFile& tempFile) {
    if (!tempFile.open()) {
        return false;
    }

    const QByteArray content = QJsonDocument(rootObject).toJson(QJsonDocument::Indented);
    const qint64 written = tempFile.write(content);
    tempFile.flush();
    tempFile.close();
    return written == content.size();
}

void requireEquipmentBaseFieldsEqual(const EquipmentData& actual, const EquipmentData& expected) {
    REQUIRE(actual.equipmentId == expected.equipmentId);
    REQUIRE(actual.equipmentType == expected.equipmentType);
    requireApprox(actual.gainDbi, expected.gainDbi);
    requireApprox(actual.offsetX, expected.offsetX);
    requireApprox(actual.offsetY, expected.offsetY);
    requireApprox(actual.offsetZ, expected.offsetZ);
}

}  // namespace

TEST_CASE("Equipment validation follows schema-compatible rules", "[schema][datamodel][equipment]") {
    SECTION("receiver accepts negative gain and rejects non-negative sensitivity") {
        EquipmentData receiver = makeReceiver();
        requireValid(receiver.validate());

        receiver.receiverSensitivityDbm = 0.0;
        requireInvalid(receiver.validate());
    }

    SECTION("transmitter accepts schema enum values and negative power") {
        EquipmentData transmitter = makeTransmitter();
        requireValid(transmitter.validate());
    }

    SECTION("transceiver accepts schema enum values and nested tx-rx DTO fields") {
        EquipmentData transceiver = makeTransceiver();
        requireValid(transceiver.validate());
    }
}

TEST_CASE("Environment validation enforces schema ranges", "[schema][datamodel][environment]") {
    SECTION("schema defaults remain valid") {
        requireValid(makeEnvironment().validateEnvironmentConfig());
    }

    SECTION("maxRange must be positive") {
        EnvironmentData environment = makeEnvironment();
        environment.maxRange = 0.0;
        requireInvalid(environment.validateEnvironmentConfig());
    }

    SECTION("angleStepDeg must stay within schema bounds") {
        EnvironmentData environment = makeEnvironment();
        environment.angleStepDeg = 361;
        requireInvalid(environment.validateEnvironmentConfig());
    }
}

TEST_CASE("Snapshot validation enforces ship and equipment consistency", "[schema][datamodel][snapshot]") {
    DataModel::DataSnapshot snapshot;
    snapshot.environmentConfig = makeEnvironment();
    snapshot.allEquipments.push_back(makeReceiver());
    snapshot.allShips.push_back(makeShip(QStringLiteral("EQ_RX_1")));

    SECTION("valid snapshot passes") {
        requireValid(DataModel::validateSnapshot(snapshot));
    }

    SECTION("snapshot requires at least one ship") {
        snapshot.allShips.clear();
        requireInvalid(DataModel::validateSnapshot(snapshot));
    }

    SECTION("ship orientation must remain within [0, 360]") {
        snapshot.allShips.front().shipOrientationDeg = 361.0;
        requireInvalid(DataModel::validateSnapshot(snapshot));
    }

    SECTION("ship equipment references must point to existing equipment") {
        snapshot.allShips.front().equipmentRefs.front().equipmentId = QStringLiteral("MISSING_EQ");
        requireInvalid(DataModel::validateSnapshot(snapshot));
    }
}

TEST_CASE("JsonLoader maps SchemaConstants-based JSON into DataModel DTOs", "[schema][loader][chain]") {
    const ScopedDataModelState dataModelStateGuard;

    QTemporaryFile tempFile(QStringLiteral("SchemaDtoValidation-XXXXXX.json"));
    REQUIRE(writeJsonToTempFile(makeSchemaDrivenRootJson(), tempFile));
    REQUIRE(JsonLoader::LoadFile(tempFile.fileName()));

    DataModel* model = DataModel::instance();
    requireValid(model->validateCurrentModel());

    REQUIRE(model->allShips.size() == 1);
    REQUIRE(model->allEquipments.size() == 3);

    const auto& loadedShip = model->allShips.front();
    REQUIRE(loadedShip.shipId == std::string("USV_SCHEMA_1"));
    requireApprox(loadedShip.worldX, 120.0);
    requireApprox(loadedShip.worldY, 45.0);
    requireApprox(loadedShip.worldZ, 3.0);
    requireApprox(loadedShip.shipSpeedMps, 9.0);
    requireApprox(loadedShip.shipOrientationDeg, 135.0);
    REQUIRE(loadedShip.equipmentRefs.size() == 3);
    REQUIRE(loadedShip.equipmentRefs[0].equipmentId == QStringLiteral("TX_SCHEMA_1"));
    REQUIRE(loadedShip.equipmentRefs[1].equipmentId == QStringLiteral("RX_SCHEMA_1"));
    REQUIRE(loadedShip.equipmentRefs[2].equipmentId == QStringLiteral("TRX_SCHEMA_1"));

    requireApprox(model->environmentConfig.maxRange, 4096.0);
    requireApprox(model->environmentConfig.ductHeight, 32.0);
    requireApprox(model->environmentConfig.windSpeed, 11.0);
    requireApprox(model->environmentConfig.dx, 2.5);
    requireApprox(model->environmentConfig.dz, 0.25);
    REQUIRE(model->environmentConfig.nz == 1024);
    REQUIRE(model->environmentConfig.angleStepDeg == 15);

    const auto* loadedTransmitter = model->findEquipmentByID(QStringLiteral("TX_SCHEMA_1"));
    REQUIRE(loadedTransmitter != nullptr);
    REQUIRE(loadedTransmitter->equipmentType == schemaValue(SchemaValues::Transmitter));
    REQUIRE(loadedTransmitter->transmitterPolarization == schemaValue(SchemaValues::Horizontal));
    REQUIRE(loadedTransmitter->transmitterAntennaType == schemaValue(SchemaValues::Reflector));
    requireApprox(loadedTransmitter->transmitterCenterFrequencyGHz, 1.25);
    requireApprox(loadedTransmitter->transmitterBandwidthMHz, 25.0);

    const auto* loadedReceiver = model->findEquipmentByID(QStringLiteral("RX_SCHEMA_1"));
    REQUIRE(loadedReceiver != nullptr);
    REQUIRE(loadedReceiver->equipmentType == schemaValue(SchemaValues::Receiver));
    requireApprox(loadedReceiver->receiverCenterFrequencyGHz, 2.45);
    requireApprox(loadedReceiver->receiverSensitivityDbm, -88.0);

    const auto* loadedTransceiver = model->findEquipmentByID(QStringLiteral("TRX_SCHEMA_1"));
    REQUIRE(loadedTransceiver != nullptr);
    REQUIRE(loadedTransceiver->equipmentType == schemaValue(SchemaValues::Transceiver));
    requireApprox(loadedTransceiver->transmitterCenterFrequencyGHz, 5.8);
    requireApprox(loadedTransceiver->receiverCenterFrequencyGHz, 5.8);
    REQUIRE(loadedTransceiver->transmitterPolarization == schemaValue(SchemaValues::Vertical));
    REQUIRE(loadedTransceiver->transmitterAntennaType == schemaValue(SchemaValues::Directional));
}

TEST_CASE("JsonLoader rejects type values outside SchemaConstants enums", "[schema][loader][negative]") {
    const ScopedDataModelState dataModelStateGuard;

    QJsonObject rootObject = makeSchemaDrivenRootJson();
    QJsonArray usvs = rootObject.value(schemaKey(SchemaKeys::Usvs)).toArray();
    QJsonObject ship = usvs.at(0).toObject();
    QJsonArray transmitters = ship.value(schemaKey(SchemaKeys::Transmitters)).toArray();
    QJsonObject transmitter = transmitters.at(0).toObject();
    transmitter.insert(schemaKey(SchemaKeys::Type), QStringLiteral("transmitter"));
    transmitters.replace(0, transmitter);
    ship.insert(schemaKey(SchemaKeys::Transmitters), transmitters);
    usvs.replace(0, ship);
    rootObject.insert(schemaKey(SchemaKeys::Usvs), usvs);

    QTemporaryFile tempFile(QStringLiteral("SchemaDtoValidation-Invalid-XXXXXX.json"));
    REQUIRE(writeJsonToTempFile(rootObject, tempFile));
    REQUIRE_FALSE(JsonLoader::LoadFile(tempFile.fileName()));
}

TEST_CASE("EMC_Engine keeps the launch snapshot instead of re-reading DataModel", "[schema][simulation][snapshot]") {
    const ScopedDataModelState dataModelStateGuard;

    DataModel::DataSnapshot snapshot;
    snapshot.environmentConfig = makeEnvironment();
    snapshot.environmentConfig.maxRange = 1234.0;

    DataModel::instance()->environmentConfig = makeSchemaDrivenEnvironment();

    auto fleet = std::make_unique<Fleet>();
    EMC_Engine engine(ModelType::PE, std::move(fleet), snapshot);

    DataModel::instance()->environmentConfig.maxRange = 9999.0;
    REQUIRE(engine.inputSnapshot().environmentConfig.maxRange == Catch::Approx(1234.0));
}

TEST_CASE("JsonLoader loads the canonical schema sample", "[schema][loader]") {
    const ScopedDataModelState dataModelStateGuard;

    REQUIRE(JsonLoader::LoadFile(QStringLiteral(EMC_SOURCE_DIR "/Tests/Test.jsonc")));

    REQUIRE(DataModel::instance()->allShips.size() == 7);
    REQUIRE(DataModel::instance()->allEquipments.size() == 14);

    const auto* loadedEquipment = DataModel::instance()->findEquipmentByID(QStringLiteral("USV1_TX1"));
    REQUIRE(loadedEquipment != nullptr);
    REQUIRE(loadedEquipment->transmitterCenterFrequencyGHz == 1.0);
    REQUIRE(loadedEquipment->transmitterBandwidthMHz == 100.0);

    const auto* loadedReceiver = DataModel::instance()->findEquipmentByID(QStringLiteral("USV1_RX1"));
    REQUIRE(loadedReceiver != nullptr);
    REQUIRE(loadedReceiver->equipmentType == schemaValue(SchemaValues::Receiver));
    REQUIRE(loadedReceiver->receiverCenterFrequencyGHz == 1.0);
    REQUIRE(loadedReceiver->receiverBandwidthMHz == 100.0);
    REQUIRE(DataModel::instance()->findEquipmentByID(QStringLiteral("USV1_TRX1")) == nullptr);

    requireValid(DataModel::instance()->validateCurrentModel());
}
