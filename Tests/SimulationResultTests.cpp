#include <catch2/catch_test_macros.hpp>

#include "Interface/SimulationResult.h"

namespace {

DataModel::DataSnapshot makeValidSnapshot() {
    DataModel::DataSnapshot snapshot;
    snapshot.environmentConfig.maxRange = 2000.0;
    snapshot.environmentConfig.ductHeight = 20.0;
    snapshot.environmentConfig.windSpeed = 7.0;
    snapshot.environmentConfig.dx = 5.0;
    snapshot.environmentConfig.dz = 0.1;
    snapshot.environmentConfig.nz = 256;
    snapshot.environmentConfig.angleStepDeg = 5;

    ShipData ship;
    ship.shipId = "USV1";
    snapshot.allShips.push_back(ship);
    return snapshot;
}

ScalarField2D makeValidField() {
    ScalarField2D field;
    field.fieldId = "agg-field";
    field.displayName = QStringLiteral("Aggregated Power Field");
    field.quantity = ScalarFieldQuantity::AggregatedPowerDbm;
    field.valueUnit = "dBm";
    field.axisXUnit = "m";
    field.axisYUnit = "m";
    field.rows = 2;
    field.cols = 3;
    field.originX = 0.0;
    field.originY = 0.0;
    field.stepX = 5.0;
    field.stepY = 5.0;
    field.values = {-42.0, -41.5, -41.0, -40.5, -40.0, -39.5};
    return field;
}

EmitterResult makeValidEmitterResult() {
    EmitterResult emitter;
    emitter.emitterId = "TX-1";
    emitter.shipId = "USV1";
    emitter.status = EmitterResultStatus::Succeeded;
    emitter.centerFrequencyGHz = 1.0;
    emitter.transmitPowerDbm = 30.0;
    emitter.worldX = 0.0;
    emitter.worldY = 0.0;
    emitter.worldZ = 2.0;
    emitter.field2D = makeValidField();
    return emitter;
}

}  // namespace

TEST_CASE("ScalarField2D validates row-major dimensions", "[result][field]") {
    ScalarField2D field = makeValidField();
    field.values.pop_back();

    const auto validation = field.validate();
    REQUIRE_FALSE(validation.first);
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

TEST_CASE("SimulationTaskResult accepts a successful task result with consistent payload", "[result][task]") {
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
    result.aggregatedField = makeValidField();
    result.emitterResults.push_back(makeValidEmitterResult());
    result.derivedMetrics.available = false;

    const auto validation = result.validate();
    INFO(validation.second.toStdString());
    REQUIRE(validation.first);
}
