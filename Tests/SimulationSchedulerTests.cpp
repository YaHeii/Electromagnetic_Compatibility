#include <catch2/catch_test_macros.hpp>

#include "Simulation/EMCComputationResult.h"
#include "Simulation/simSchedulerCtx.h"

namespace {

DataModel::DataSnapshot makeSnapshot() {
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

ScalarField2D makeField() {
    ScalarField2D field;
    field.fieldId = "agg-field";
    field.displayName = QStringLiteral("Aggregated Power Field");
    field.quantity = ScalarFieldQuantity::AggregatedPowerDbm;
    field.valueUnit = "dBm";
    field.axisXUnit = "m";
    field.axisYUnit = "m";
    field.rows = 1;
    field.cols = 1;
    field.originX = 0.0;
    field.originY = 0.0;
    field.stepX = 5.0;
    field.stepY = 5.0;
    field.values = {-42.0};
    return field;
}

EmitterResult makeEmitterResult() {
    EmitterResult emitter;
    emitter.emitterId = "USV1_TX1";
    emitter.shipId = "USV1";
    emitter.status = EmitterResultStatus::Succeeded;
    emitter.centerFrequencyGHz = 1.0;
    emitter.transmitPowerDbm = 30.0;
    emitter.worldX = 10.0;
    emitter.worldY = 20.0;
    emitter.worldZ = 2.0;
    emitter.field2D = makeField();
    return emitter;
}

}  // namespace

TEST_CASE("simSchedulerCtx assembles successful task result with preset formation metadata", "[result][scheduler]") {
    const DataModel::DataSnapshot snapshot = makeSnapshot();

    EMCComputationResult computation;
    computation.status = SimulationResultStatus::Succeeded;
    computation.aggregatedField = makeField();
    computation.emitterResults.push_back(makeEmitterResult());

    const SimulationTaskResult result = simSchedulerCtx::assembleResult(
        snapshot,
        FormationSource::PresetFormation,
        3,
        ModelType::PE,
        1000,
        1600,
        computation);

    REQUIRE(result.formationSource == FormationSource::PresetFormation);
    REQUIRE(result.presetFormationId.has_value());
    REQUIRE(result.presetFormationId.value() == 3);
    REQUIRE(result.status == SimulationResultStatus::Succeeded);
    REQUIRE(result.aggregatedField.values.size() == 1);

    const auto validation = result.validate();
    INFO(validation.second.toStdString());
    REQUIRE(validation.first);
}
