#include <catch2/catch_test_macros.hpp>

#include "Interface/SimulationResult.h"
#include "Resource/ui/SimulationResultCatalog.h"

namespace {

EnvironmentData makeEnvironment() {
    EnvironmentData environment;
    environment.maxRange = 40.0;
    environment.ductHeight = 10.0;
    environment.windSpeed = 3.0;
    environment.dx = 1.0;
    environment.dz = 0.5;
    environment.nz = 64;
    environment.angleStepDeg = 30;
    return environment;
}

EquipmentData makeTransmitter(const QString& equipmentId) {
    EquipmentData equipment;
    equipment.equipmentId = equipmentId;
    equipment.equipmentType = DataModelSchemaValues::transmitterType();
    equipment.gainDbi = 12.0;
    equipment.offsetX = 0.0;
    equipment.offsetY = 0.0;
    equipment.offsetZ = 2.0;
    equipment.transmitterCenterFrequencyGHz = 1.0;
    equipment.transmitterBandwidthMHz = 20.0;
    equipment.transmitterPowerDbm = 30.0;
    equipment.transmitterAntennaPhiDeg = 0.0;
    equipment.transmitterBeamWidthDeg = 30.0;
    equipment.transmitterPolarization = DataModelSchemaValues::verticalPolarization();
    equipment.transmitterAntennaType = QString::fromLatin1(SchemaValues::Horn);
    return equipment;
}

EquipmentData makeReceiver() {
    EquipmentData equipment;
    equipment.equipmentId = "RX-1";
    equipment.equipmentType = DataModelSchemaValues::receiverType();
    equipment.gainDbi = 6.0;
    equipment.offsetX = 0.0;
    equipment.offsetY = 0.0;
    equipment.offsetZ = 2.0;
    equipment.receiverCenterFrequencyGHz = 1.0;
    equipment.receiverBandwidthMHz = 20.0;
    equipment.receiverSensitivityDbm = -85.0;
    equipment.receiverInterferenceMarginDb = -10.0;
    equipment.receiverSinrMarginDb = 3.0;
    equipment.receiverNoiseFigureDb = 3.0;
    return equipment;
}

ShipData makeShip(
    const std::string& shipId,
    double worldX,
    const QString& equipmentId) {
    ShipData ship;
    ship.shipId = shipId;
    ship.worldX = worldX;
    ship.worldY = 0.0;
    ship.worldZ = 0.0;
    ship.shipOrientationDeg = 0.0;
    ship.shipSpeedMps = 4.0;
    ship.equipmentRefs.push_back(EquipmentOnShip{equipmentId, true});
    return ship;
}

DataModel::DataSnapshot makeSnapshot(const QString& referenceTransmitterId) {
    DataModel::DataSnapshot snapshot;
    snapshot.environmentConfig = makeEnvironment();
    snapshot.emcAnalysisConfig.fieldPlaneHeightM = 8.0;
    snapshot.emcAnalysisConfig.referenceTransmitterId = referenceTransmitterId;
    snapshot.emcAnalysisConfig.referenceReceiverId = "RX-1";
    snapshot.emcAnalysisConfig.s3iBaselineWindSpeedMps = 0.5;
    snapshot.allEquipments = {
        makeTransmitter("TX-1"),
        makeTransmitter("TX-2"),
        makeReceiver(),
    };
    snapshot.allShips = {
        makeShip("USV-TX-1", 0.0, "TX-1"),
        makeShip("USV-TX-2", 8.0, "TX-2"),
        makeShip("USV-RX", 16.0, "RX-1"),
    };
    return snapshot;
}

ScalarField2D makeField(
    ScalarFieldQuantity quantity,
    const QString& fieldId,
    const QString& displayName) {
    ScalarField2D field;
    field.fieldId = fieldId;
    field.displayName = displayName;
    field.quantity = quantity;
    field.valueUnit = expectedValueUnit(quantity);
    field.axisXUnit = "m";
    field.axisYUnit = "m";
    field.rows = 2;
    field.cols = 2;
    field.originX = 0.0;
    field.originY = 0.0;
    field.stepX = 1.0;
    field.stepY = 1.0;
    field.values = {-42.0, -41.0, -40.0, -39.0};
    return field;
}

EmitterResult makeEmitterResult(const QString& emitterId, const QString& shipId) {
    EmitterResult emitter;
    emitter.emitterId = emitterId;
    emitter.shipId = shipId;
    emitter.status = EmitterResultStatus::Succeeded;
    emitter.centerFrequencyGHz = 1.0;
    emitter.transmitPowerDbm = 30.0;
    emitter.worldX = 0.0;
    emitter.worldY = 0.0;
    emitter.worldZ = 2.0;
    emitter.field2D = makeField(
        ScalarFieldQuantity::PathLossDb,
        QStringLiteral("path-loss-%1").arg(emitterId),
        QStringLiteral("Path Loss Field"));
    return emitter;
}

Series1D makeSeries(const QString& id, const QString& name) {
    Series1D series;
    series.seriesId = id;
    series.displayName = name;
    series.xUnit = "m";
    series.yUnit = "dB";
    series.xValues = {1.0, 2.0, 3.0};
    series.yValues = {-70.0, -72.0, -73.5};
    return series;
}

LabeledMatrix2D makeMatrix() {
    LabeledMatrix2D matrix;
    matrix.matrixId = "scf-coupling";
    matrix.displayName = QStringLiteral("SCF Coupling Matrix");
    matrix.valueUnit = "dB";
    matrix.rows = 1;
    matrix.cols = 2;
    matrix.rowLabels = {QStringLiteral("RX-1")};
    matrix.colLabels = {QStringLiteral("TX-1"), QStringLiteral("TX-2")};
    matrix.values = {12.0, 7.0};
    return matrix;
}

DerivedMetrics makeDerivedMetrics() {
    DerivedMetrics metrics;
    metrics.available = true;
    metrics.scf.scalarDb = 12.0;
    metrics.scf.thermalNoiseFloorDbm = -98.0;
    metrics.scf.linkCount = 2;
    metrics.scf.couplingMatrix = makeMatrix();

    metrics.s3i.scalarDb = 0.8;
    metrics.s3i.referenceTransmitterId = "TX-1";
    metrics.s3i.referenceReceiverId = "RX-1";
    metrics.s3i.baselineWindSpeedMps = 0.5;
    metrics.s3i.currentWindSpeedMps = 3.0;
    metrics.s3i.calmCurve = makeSeries("s3i-calm", QStringLiteral("平静海况曲线"));
    metrics.s3i.currentCurve = makeSeries("s3i-current", QStringLiteral("当前海况曲线"));

    metrics.tElev.field = makeField(
        ScalarFieldQuantity::NoiseElevationDb,
        "t-elev",
        QStringLiteral("Noise Elevation Field"));
    metrics.tElev.maxDb = -39.0;
    metrics.tElev.meanDb = -40.5;

    metrics.dDesense.field = makeField(
        ScalarFieldQuantity::DesenseDb,
        "d-desense",
        QStringLiteral("Receiver Desense Field"));
    metrics.dDesense.victimReceiverId = "RX-1";
    metrics.dDesense.peakDb = 18.0;
    metrics.dDesense.coveragePercent = 75.0;
    metrics.dDesense.adiDbPerSquareMeter = 6.0;
    return metrics;
}

SimulationTaskResult makeSuccessfulResult(const QString& referenceTransmitterId) {
    SimulationTaskResult result;
    result.resultSchemaVersion = "1.0.0";
    result.taskId = "task-success-1";
    result.modelType = ModelType::PE;
    result.status = SimulationResultStatus::Succeeded;
    result.formationSource = FormationSource::ManualInput;
    result.startedAtUtcMs = 100;
    result.finishedAtUtcMs = 300;
    result.durationMs = 200;
    result.summaryText = QStringLiteral("simulation completed");
    result.inputSnapshot = makeSnapshot(referenceTransmitterId);
    result.aggregatedField = makeField(
        ScalarFieldQuantity::AggregatedPowerDbm,
        "aggregated-power",
        QStringLiteral("Aggregated Power Field"));
    result.emitterResults.push_back(makeEmitterResult("TX-1", "USV-TX-1"));
    result.emitterResults.push_back(makeEmitterResult("TX-2", "USV-TX-2"));
    result.derivedMetrics = makeDerivedMetrics();
    return result;
}

}  // namespace

TEST_CASE("SimulationResultCatalog builds six fixed chart cards for a successful task", "[ui][simulation][catalog]") {
    const SimulationTaskResult result = makeSuccessfulResult("TX-1");

    const std::vector<SimulationChartCardDescriptor> cards = SimulationResultCatalog::buildCards(result);
    REQUIRE(cards.size() == 6);

    REQUIRE(cards[0].key == SimulationChartKey::AggregatedField);
    REQUIRE(cards[1].key == SimulationChartKey::ReferenceEmitterPathLoss);
    REQUIRE(cards[2].key == SimulationChartKey::ScfMatrix);
    REQUIRE(cards[3].key == SimulationChartKey::S3iCurve);
    REQUIRE(cards[4].key == SimulationChartKey::TElevField);
    REQUIRE(cards[5].key == SimulationChartKey::DDesenseField);

    for (const SimulationChartCardDescriptor& card : cards) {
        INFO(card.title.toStdString());
        REQUIRE_FALSE(card.title.trimmed().isEmpty());
        REQUIRE(card.available);
    }
}

TEST_CASE("SimulationResultCatalog only accepts the configured reference transmitter", "[ui][simulation][catalog]") {
    const SimulationTaskResult result = makeSuccessfulResult("TX-2");

    const SimulationChartPayload payload = SimulationResultCatalog::payloadForKey(
        result,
        SimulationChartKey::ReferenceEmitterPathLoss);

    REQUIRE(payload.available);
    REQUIRE(payload.scalarField != nullptr);
    REQUIRE(payload.subtitle.contains(QStringLiteral("TX-2")));
    REQUIRE_FALSE(payload.subtitle.contains(QStringLiteral("TX-1")));
}

TEST_CASE("SimulationResultCatalog only exposes the gallery for successful results", "[ui][simulation][catalog]") {
    SimulationTaskResult failedResult = makeSuccessfulResult("TX-1");
    failedResult.status = SimulationResultStatus::Failed;
    failedResult.errorMessage = QStringLiteral("mock failure");

    SimulationTaskResult cancelledResult = makeSuccessfulResult("TX-1");
    cancelledResult.status = SimulationResultStatus::Cancelled;
    cancelledResult.derivedMetrics.available = false;

    REQUIRE(SimulationResultCatalog::buildCards(failedResult).empty());
    REQUIRE(SimulationResultCatalog::buildCards(cancelledResult).empty());
}

TEST_CASE("SimulationResultCatalog never falls back to a different emitter when the reference transmitter is missing", "[ui][simulation][catalog]") {
    const SimulationTaskResult result = makeSuccessfulResult("TX-missing");

    const SimulationChartPayload payload = SimulationResultCatalog::payloadForKey(
        result,
        SimulationChartKey::ReferenceEmitterPathLoss);

    REQUIRE_FALSE(payload.available);
    REQUIRE(payload.scalarField == nullptr);
    REQUIRE(payload.subtitle == QStringLiteral("未找到参考发射机结果"));
}
