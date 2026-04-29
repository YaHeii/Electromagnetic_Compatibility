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
#include "Simulation/PEPropagationSolver.h"
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
        model->emcAnalysisConfig = snapshotToRestore.emcAnalysisConfig;
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

EnvironmentData makeEnvironment() {
    EnvironmentData environment;
    environment.maxRange = 2000.0;
    environment.ductHeight = 20.0;
    environment.windSpeed = 7.0;
    environment.dx = 5.0;
    environment.dz = 0.5;
    environment.nz = 256;
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

EMCAnalysisConfig makeAnalysisConfig() {
    EMCAnalysisConfig config;
    config.fieldPlaneHeightM = 12.0;
    config.referenceTransmitterId = "TX_SCHEMA_1";
    config.referenceReceiverId = "RX_SCHEMA_1";
    config.s3iBaselineWindSpeedMps = 0.5;
    return config;
}

QJsonObject makeAnalysisConfigJson(const EMCAnalysisConfig& config) {
    QJsonObject object;
    object.insert(schemaKey(SchemaKeys::FieldPlaneHeightM), config.fieldPlaneHeightM);
    object.insert(schemaKey(SchemaKeys::ReferenceTransmitterId), config.referenceTransmitterId);
    object.insert(schemaKey(SchemaKeys::ReferenceReceiverId), config.referenceReceiverId);
    object.insert(schemaKey(SchemaKeys::S3IBaselineWindSpeedMps), config.s3iBaselineWindSpeedMps);
    return object;
}

EquipmentData makeReceiver() {
    EquipmentData equipment;
    equipment.equipmentId = "RX_SCHEMA_1";
    equipment.equipmentType = schemaValue(SchemaValues::Receiver);
    equipment.gainDbi = -3.0;
    equipment.offsetX = 0.0;
    equipment.offsetY = 0.0;
    equipment.offsetZ = 2.0;
    equipment.receiverCenterFrequencyGHz = 1.0;
    equipment.receiverBandwidthMHz = 20.0;
    equipment.receiverSensitivityDbm = -85.0;
    equipment.receiverInterferenceMarginDb = -10.0;
    equipment.receiverSinrMarginDb = 0.0;
    equipment.receiverNoiseFigureDb = 3.0;
    return equipment;
}

EquipmentData makeTransmitter() {
    EquipmentData equipment;
    equipment.equipmentId = "TX_SCHEMA_1";
    equipment.equipmentType = schemaValue(SchemaValues::Transmitter);
    equipment.gainDbi = 8.0;
    equipment.offsetX = 0.0;
    equipment.offsetY = 0.0;
    equipment.offsetZ = 2.0;
    equipment.transmitterCenterFrequencyGHz = 1.0;
    equipment.transmitterBandwidthMHz = 25.0;
    equipment.transmitterPowerDbm = 18.0;
    equipment.transmitterAntennaPhiDeg = 40.0;
    equipment.transmitterBeamWidthDeg = 35.0;
    equipment.transmitterPolarization = schemaValue(SchemaValues::Horizontal);
    equipment.transmitterAntennaType = schemaValue(SchemaValues::Reflector);
    return equipment;
}

EquipmentData makeTransceiver() {
    EquipmentData equipment;
    equipment.equipmentId = "TRX_SCHEMA_1";
    equipment.equipmentType = schemaValue(SchemaValues::Transceiver);
    equipment.gainDbi = 5.0;
    equipment.offsetX = 0.0;
    equipment.offsetY = 0.0;
    equipment.offsetZ = 2.0;
    equipment.transmitterCenterFrequencyGHz = 5.8;
    equipment.transmitterBandwidthMHz = 40.0;
    equipment.transmitterPowerDbm = 22.0;
    equipment.transmitterAntennaPhiDeg = 12.0;
    equipment.transmitterBeamWidthDeg = 60.0;
    equipment.transmitterPolarization = schemaValue(SchemaValues::Vertical);
    equipment.transmitterAntennaType = schemaValue(SchemaValues::Directional);
    equipment.receiverCenterFrequencyGHz = 5.8;
    equipment.receiverBandwidthMHz = 20.0;
    equipment.receiverSensitivityDbm = -81.0;
    equipment.receiverInterferenceMarginDb = -5.0;
    equipment.receiverSinrMarginDb = 5.5;
    equipment.receiverNoiseFigureDb = 2.2;
    return equipment;
}

ShipData makeShip(
    const std::string& shipId,
    double worldX,
    const std::vector<EquipmentOnShip>& equipmentRefs) {
    ShipData ship;
    ship.shipId = shipId;
    ship.worldX = worldX;
    ship.worldY = 0.0;
    ship.worldZ = 0.0;
    ship.shipSpeedMps = 6.0;
    ship.shipOrientationDeg = 90.0;
    ship.equipmentRefs = equipmentRefs;
    return ship;
}

QJsonObject makeTransmitterJson(const QString& id) {
    QJsonObject object;
    object.insert(schemaKey(SchemaKeys::ID), id);
    object.insert(schemaKey(SchemaKeys::Type), schemaValue(SchemaValues::Transmitter));
    object.insert(schemaKey(SchemaKeys::GainDbi), 9.5);
    object.insert(schemaKey(SchemaKeys::LocationOffset), makeVector3(1.0, 2.0, 2.0));
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
    object.insert(schemaKey(SchemaKeys::LocationOffset), makeVector3(-1.0, 0.5, 2.0));
    object.insert(schemaKey(SchemaKeys::CenterFrequencyGHz), 2.45);
    object.insert(schemaKey(SchemaKeys::BandwidthMHz), 12.5);
    object.insert(schemaKey(SchemaKeys::SensitivityDbm), -88.0);
    object.insert(schemaKey(SchemaKeys::InterferenceMarginDb), -8.0);
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
    receiver.insert(schemaKey(SchemaKeys::InterferenceMarginDb), -5.0);
    receiver.insert(schemaKey(SchemaKeys::SinrMarginDb), 5.5);
    receiver.insert(schemaKey(SchemaKeys::NoiseFigureDb), 2.2);

    QJsonObject object;
    object.insert(schemaKey(SchemaKeys::ID), id);
    object.insert(schemaKey(SchemaKeys::Type), schemaValue(SchemaValues::Transceiver));
    object.insert(schemaKey(SchemaKeys::GainDbi), 6.5);
    object.insert(schemaKey(SchemaKeys::LocationOffset), makeVector3(0.0, 0.0, 2.0));
    object.insert(schemaKey(SchemaKeys::TransmitterConfig), transmitter);
    object.insert(schemaKey(SchemaKeys::ReceiverConfig), receiver);
    return object;
}

QJsonObject makeSchemaDrivenRootJson() {
    const EnvironmentData environment = makeEnvironment();
    const EMCAnalysisConfig analysisConfig = makeAnalysisConfig();

    QJsonArray ship1Transmitters;
    ship1Transmitters.append(makeTransmitterJson(QStringLiteral("TX_SCHEMA_1")));

    QJsonArray ship1Receivers;

    QJsonArray ship1Transceivers;
    ship1Transceivers.append(makeTransceiverJson(QStringLiteral("TRX_SCHEMA_1")));

    QJsonObject ship1;
    ship1.insert(schemaKey(SchemaKeys::ID), QStringLiteral("USV_SCHEMA_1"));
    ship1.insert(schemaKey(SchemaKeys::Location), makePoint3D(120.0, 45.0, 0.0));
    ship1.insert(schemaKey(SchemaKeys::Speed), 9.0);
    ship1.insert(schemaKey(SchemaKeys::ShipOrientationDeg), 135.0);
    ship1.insert(schemaKey(SchemaKeys::Transmitters), ship1Transmitters);
    ship1.insert(schemaKey(SchemaKeys::Receivers), ship1Receivers);
    ship1.insert(schemaKey(SchemaKeys::Transceivers), ship1Transceivers);

    QJsonArray ship2Transmitters;
    QJsonArray ship2Receivers;
    ship2Receivers.append(makeReceiverJson(QStringLiteral("RX_SCHEMA_1")));
    QJsonArray ship2Transceivers;

    QJsonObject ship2;
    ship2.insert(schemaKey(SchemaKeys::ID), QStringLiteral("USV_SCHEMA_2"));
    ship2.insert(schemaKey(SchemaKeys::Location), makePoint3D(520.0, 45.0, 0.0));
    ship2.insert(schemaKey(SchemaKeys::Speed), 8.0);
    ship2.insert(schemaKey(SchemaKeys::ShipOrientationDeg), 180.0);
    ship2.insert(schemaKey(SchemaKeys::Transmitters), ship2Transmitters);
    ship2.insert(schemaKey(SchemaKeys::Receivers), ship2Receivers);
    ship2.insert(schemaKey(SchemaKeys::Transceivers), ship2Transceivers);

    QJsonArray usvs;
    usvs.append(ship1);
    usvs.append(ship2);

    QJsonObject root;
    root.insert(schemaKey(SchemaKeys::SchemaVersion), schemaValue(SchemaValues::SchemaVersion_1_0_0));
    root.insert(schemaKey(SchemaKeys::Environment), makeEnvironmentJson(environment));
    root.insert(schemaKey(SchemaKeys::EMCAnalysisConfig), makeAnalysisConfigJson(analysisConfig));
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

}  // namespace

TEST_CASE("Equipment validation follows schema-compatible rules", "[schema][datamodel][equipment]") {
    SECTION("receiver accepts negative sensitivity and rejects non-negative sensitivity") {
        EquipmentData receiver = makeReceiver();
        requireValid(receiver.validate());

        receiver.receiverSensitivityDbm = 0.0;
        requireInvalid(receiver.validate());
    }

    SECTION("transmitter accepts schema enum values") {
        EquipmentData transmitter = makeTransmitter();
        requireValid(transmitter.validate());
    }

    SECTION("transceiver accepts nested tx-rx DTO fields") {
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

TEST_CASE("Snapshot validation enforces EMC analysis config and cross-field consistency", "[schema][datamodel][snapshot]") {
    DataModel::DataSnapshot snapshot;
    snapshot.environmentConfig = makeEnvironment();
    snapshot.emcAnalysisConfig = makeAnalysisConfig();
    snapshot.allEquipments = {makeTransmitter(), makeReceiver()};
    snapshot.allShips = {
        makeShip("USV_TX", 0.0, {EquipmentOnShip{"TX_SCHEMA_1", true}}),
        makeShip("USV_RX", 10.0, {EquipmentOnShip{"RX_SCHEMA_1", true}}),
    };

    SECTION("valid snapshot passes") {
        requireValid(DataModel::validateSnapshot(snapshot));
    }

    SECTION("snapshot requires field plane height within vertical grid") {
        snapshot.emcAnalysisConfig.fieldPlaneHeightM = 1000.0;
        requireInvalid(DataModel::validateSnapshot(snapshot));
    }

    SECTION("reference transmitter must resolve to enabled transmitter reference") {
        snapshot.allShips.front().equipmentRefs.front().isEnabled = false;
        requireInvalid(DataModel::validateSnapshot(snapshot));
    }

    SECTION("reference receiver must stay cross-platform") {
        snapshot.allShips.front().equipmentRefs.push_back(EquipmentOnShip{"RX_SCHEMA_1", true});
        snapshot.allShips.back().equipmentRefs.clear();
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

    REQUIRE(model->allShips.size() == 2);
    REQUIRE(model->allEquipments.size() == 3);

    requireApprox(model->environmentConfig.maxRange, 2000.0);
    requireApprox(model->environmentConfig.ductHeight, 20.0);
    requireApprox(model->environmentConfig.windSpeed, 7.0);
    requireApprox(model->environmentConfig.dx, 5.0);
    requireApprox(model->environmentConfig.dz, 0.5);
    REQUIRE(model->environmentConfig.nz == 256);
    REQUIRE(model->environmentConfig.angleStepDeg == 15);

    requireApprox(model->emcAnalysisConfig.fieldPlaneHeightM, 12.0);
    REQUIRE(model->emcAnalysisConfig.referenceTransmitterId == QStringLiteral("TX_SCHEMA_1"));
    REQUIRE(model->emcAnalysisConfig.referenceReceiverId == QStringLiteral("RX_SCHEMA_1"));
    requireApprox(model->emcAnalysisConfig.s3iBaselineWindSpeedMps, 0.5);

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

TEST_CASE("JsonLoader rejects missing or invalid EMC analysis config", "[schema][loader][negative]") {
    const ScopedDataModelState dataModelStateGuard;

    SECTION("missing emcAnalysisConfig is rejected") {
        QJsonObject rootObject = makeSchemaDrivenRootJson();
        rootObject.remove(schemaKey(SchemaKeys::EMCAnalysisConfig));

        QTemporaryFile tempFile(QStringLiteral("SchemaDtoValidation-Missing-XXXXXX.json"));
        REQUIRE(writeJsonToTempFile(rootObject, tempFile));
        REQUIRE_FALSE(JsonLoader::LoadFile(tempFile.fileName()));
    }

    SECTION("same-platform reference link is rejected") {
        QJsonObject rootObject = makeSchemaDrivenRootJson();
        QJsonObject analysisObject = rootObject.value(schemaKey(SchemaKeys::EMCAnalysisConfig)).toObject();
        analysisObject.insert(schemaKey(SchemaKeys::ReferenceReceiverId), QStringLiteral("TRX_SCHEMA_1"));
        rootObject.insert(schemaKey(SchemaKeys::EMCAnalysisConfig), analysisObject);

        QTemporaryFile tempFile(QStringLiteral("SchemaDtoValidation-SamePlatform-XXXXXX.json"));
        REQUIRE(writeJsonToTempFile(rootObject, tempFile));
        REQUIRE_FALSE(JsonLoader::LoadFile(tempFile.fileName()));
    }
}

TEST_CASE("EMC_Engine keeps the launch snapshot instead of re-reading DataModel", "[schema][simulation][snapshot]") {
    const ScopedDataModelState dataModelStateGuard;

    DataModel::DataSnapshot snapshot;
    snapshot.environmentConfig = makeEnvironment();
    snapshot.emcAnalysisConfig = makeAnalysisConfig();

    DataModel::instance()->environmentConfig = makeEnvironment();
    DataModel::instance()->emcAnalysisConfig.fieldPlaneHeightM = 99.0;

    auto fleet = std::make_unique<Fleet>();
    EMC_Engine engine(ModelType::PE, std::move(fleet), snapshot);

    DataModel::instance()->emcAnalysisConfig.fieldPlaneHeightM = 200.0;
    REQUIRE(engine.inputSnapshot().emcAnalysisConfig.fieldPlaneHeightM == Catch::Approx(12.0));
}

TEST_CASE("JsonLoader loads the canonical schema sample", "[schema][loader]") {
    const ScopedDataModelState dataModelStateGuard;

    REQUIRE(JsonLoader::LoadFile(QStringLiteral(EMC_SOURCE_DIR "/Tests/Test.jsonc")));

    REQUIRE(DataModel::instance()->allShips.size() == 7);
    REQUIRE(DataModel::instance()->allEquipments.size() == 14);
    REQUIRE(DataModel::instance()->emcAnalysisConfig.referenceTransmitterId == QStringLiteral("USV1_TX1"));
    REQUIRE(DataModel::instance()->emcAnalysisConfig.referenceReceiverId == QStringLiteral("USV2_RX1"));

    const auto* loadedEquipment = DataModel::instance()->findEquipmentByID(QStringLiteral("USV1_TX1"));
    REQUIRE(loadedEquipment != nullptr);
    REQUIRE(loadedEquipment->transmitterCenterFrequencyGHz == 1.0);
    REQUIRE(loadedEquipment->transmitterBandwidthMHz == 100.0);

    const auto* loadedReceiver = DataModel::instance()->findEquipmentByID(QStringLiteral("USV1_RX1"));
    REQUIRE(loadedReceiver != nullptr);
    REQUIRE(loadedReceiver->equipmentType == schemaValue(SchemaValues::Receiver));
    REQUIRE(loadedReceiver->receiverCenterFrequencyGHz == 1.0);
    REQUIRE(loadedReceiver->receiverBandwidthMHz == 100.0);

    requireValid(DataModel::instance()->validateCurrentModel());
}
