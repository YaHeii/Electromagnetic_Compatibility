#include <catch2/catch_test_macros.hpp>

#include "Interface/SimulationResult.h"

namespace {

EnvironmentData makeEnvironment() {
    EnvironmentData environment;
    environment.maxRange = 20.0;
    environment.ductHeight = 10.0;
    environment.windSpeed = 3.0;
    environment.dx = 1.0;
    environment.dz = 0.5;
    environment.nz = 64;
    environment.angleStepDeg = 30;
    return environment;
}

EquipmentData makeTransmitter() {
    EquipmentData equipment;
    equipment.equipmentId = "TX-1";
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

ShipData makeTxShip() {
    ShipData ship;
    ship.shipId = "USV-TX";
    ship.worldX = 0.0;
    ship.worldY = 0.0;
    ship.worldZ = 0.0;
    ship.shipOrientationDeg = 0.0;
    ship.shipSpeedMps = 4.0;
    ship.equipmentRefs.push_back(EquipmentOnShip{"TX-1", true});
    return ship;
}

ShipData makeRxShip() {
    ShipData ship;
    ship.shipId = "USV-RX";
    ship.worldX = 10.0;
    ship.worldY = 0.0;
    ship.worldZ = 0.0;
    ship.shipOrientationDeg = 180.0;
    ship.shipSpeedMps = 4.0;
    ship.equipmentRefs.push_back(EquipmentOnShip{"RX-1", true});
    return ship;
}

EMCAnalysisConfig makeAnalysisConfig() {
    EMCAnalysisConfig config;
    config.fieldPlaneHeightM = 8.0;
    config.referenceTransmitterId = "TX-1";
    config.referenceReceiverId = "RX-1";
    config.s3iBaselineWindSpeedMps = 0.5;
    return config;
}

DataModel::DataSnapshot makeValidSnapshot() {
    DataModel::DataSnapshot snapshot;
    snapshot.environmentConfig = makeEnvironment();
    snapshot.emcAnalysisConfig = makeAnalysisConfig();
    snapshot.allEquipments = {makeTransmitter(), makeReceiver()};
    snapshot.allShips = {makeTxShip(), makeRxShip()};
    return snapshot;
}

ScalarField2D makeField(ScalarFieldQuantity quantity, const QString& fieldId, const QString& displayName) {
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

EmitterResult makeValidEmitterResult() {
    EmitterResult emitter;
    emitter.emitterId = "TX-1";
    emitter.shipId = "USV-TX";
    emitter.status = EmitterResultStatus::Succeeded;
    emitter.centerFrequencyGHz = 1.0;
    emitter.transmitPowerDbm = 30.0;
    emitter.worldX = 0.0;
    emitter.worldY = 0.0;
    emitter.worldZ = 2.0;
    emitter.field2D = makeField(
        ScalarFieldQuantity::PathLossDb,
        "path-loss-TX-1",
        QStringLiteral("Path Loss Field"));
    return emitter;
}

Series1D makeSeries(const QString& id, const QString& name, const QString& yUnit) {
    Series1D series;
    series.seriesId = id;
    series.displayName = name;
    series.xUnit = "m";
    series.yUnit = yUnit;
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
    matrix.cols = 1;
    matrix.rowLabels = {QStringLiteral("RX-1")};
    matrix.colLabels = {QStringLiteral("TX-1")};
    matrix.values = {12.0};
    return matrix;
}

DerivedMetrics makeDerivedMetrics() {
    DerivedMetrics metrics;
    metrics.available = true;

    metrics.scf.scalarDb = 12.0;
    metrics.scf.thermalNoiseFloorDbm = -98.0;
    metrics.scf.linkCount = 1;
    metrics.scf.couplingMatrix = makeMatrix();

    metrics.s3i.scalarDb = 0.8;
    metrics.s3i.referenceTransmitterId = "TX-1";
    metrics.s3i.referenceReceiverId = "RX-1";
    metrics.s3i.baselineWindSpeedMps = 0.5;
    metrics.s3i.currentWindSpeedMps = 3.0;
    metrics.s3i.calmCurve = makeSeries(
        "s3i-calm",
        QStringLiteral("Calm Sea Curve"),
        "dB");
    metrics.s3i.currentCurve = makeSeries(
        "s3i-current",
        QStringLiteral("Current Sea Curve"),
        "dB");

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

}  // namespace

TEST_CASE("ScalarField2D validates row-major dimensions", "[result][field]") {
    ScalarField2D field = makeField(
        ScalarFieldQuantity::AggregatedPowerDbm,
        "agg-field",
        QStringLiteral("Aggregated Power Field"));
    field.values.pop_back();

    const auto validation = field.validate();
    REQUIRE_FALSE(validation.first);
}

TEST_CASE("DerivedMetrics validates chart payload completeness", "[result][metrics]") {
    DerivedMetrics metrics = makeDerivedMetrics();
    INFO(metrics.validate().second.toStdString());
    REQUIRE(metrics.validate().first);

    SECTION("available metrics require aligned coupling matrix labels") {
        metrics.scf.couplingMatrix.rowLabels.clear();
        const auto validation = metrics.validate();
        REQUIRE_FALSE(validation.first);
    }

    SECTION("available metrics require aligned S3I curves") {
        metrics.s3i.currentCurve.yValues.pop_back();
        const auto validation = metrics.validate();
        REQUIRE_FALSE(validation.first);
    }
}

TEST_CASE("SimulationTaskResult validates formation source contract", "[result][task]") {
    SimulationTaskResult result;
    result.resultSchemaVersion = "1.0.0";
    result.taskId = "task-1";
    result.modelType = ModelType::PE;
    result.status = SimulationResultStatus::Failed;
    result.formationSource = FormationSource::ManualInput;
    result.presetFormationId = 7;
    result.startedAtUtcMs = 100;
    result.finishedAtUtcMs = 200;
    result.durationMs = 100;
    result.errorMessage = QStringLiteral("mock error");
    result.inputSnapshot = makeValidSnapshot();
    result.derivedMetrics.available = false;

    const auto validation = result.validate();
    REQUIRE_FALSE(validation.first);
}

TEST_CASE("SimulationTaskResult accepts a successful task result with complete derived metrics", "[result][task]") {
    SimulationTaskResult result;
    result.resultSchemaVersion = "1.0.0";
    result.taskId = "task-success-1";
    result.modelType = ModelType::PE;
    result.status = SimulationResultStatus::Succeeded;
    result.formationSource = FormationSource::PresetFormation;
    result.presetFormationId = 3;
    result.startedAtUtcMs = 100;
    result.finishedAtUtcMs = 300;
    result.durationMs = 200;
    result.summaryText = QStringLiteral("simulation completed");
    result.inputSnapshot = makeValidSnapshot();
    result.aggregatedField = makeField(
        ScalarFieldQuantity::AggregatedPowerDbm,
        "aggregated-power",
        QStringLiteral("Aggregated Power Field"));
    result.emitterResults.push_back(makeValidEmitterResult());
    result.derivedMetrics = makeDerivedMetrics();

    const auto validation = result.validate();
    INFO(validation.second.toStdString());
    REQUIRE(validation.first);
}
