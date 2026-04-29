#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "Interface/TransferToEngin.h"
#include "Simulation/EMCComputationResult.h"
#include "Simulation/EMC_Engine.h"
#include "Simulation/PEPropagationSolver.h"
#include "Simulation/simSchedulerCtx.h"

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
    equipment.equipmentId = "USV1_TX1";
    equipment.equipmentType = DataModelSchemaValues::transmitterType();
    equipment.gainDbi = 10.0;
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
    equipment.equipmentId = "USV2_RX1";
    equipment.equipmentType = DataModelSchemaValues::receiverType();
    equipment.gainDbi = 5.0;
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

DataModel::DataSnapshot makeSnapshot() {
    DataModel::DataSnapshot snapshot;
    snapshot.environmentConfig = makeEnvironment();
    snapshot.emcAnalysisConfig.fieldPlaneHeightM = 8.0;
    snapshot.emcAnalysisConfig.referenceTransmitterId = "USV1_TX1";
    snapshot.emcAnalysisConfig.referenceReceiverId = "USV2_RX1";
    snapshot.emcAnalysisConfig.s3iBaselineWindSpeedMps = 0.5;
    snapshot.allEquipments = {makeTransmitter(), makeReceiver()};
    snapshot.allShips = {
        makeShip("USV1", 0.0, "USV1_TX1"),
        makeShip("USV2", 10.0, "USV2_RX1"),
    };
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
    field.values = {101.0, 100.0, 99.0, 98.0};
    return field;
}

EmitterResult makeEmitterResult() {
    EmitterResult emitter;
    emitter.emitterId = "USV1_TX1";
    emitter.shipId = "USV1";
    emitter.status = EmitterResultStatus::Succeeded;
    emitter.centerFrequencyGHz = 1.0;
    emitter.transmitPowerDbm = 30.0;
    emitter.worldX = 0.0;
    emitter.worldY = 0.0;
    emitter.worldZ = 2.0;
    emitter.field2D = makeField(
        ScalarFieldQuantity::PathLossDb,
        "path-loss-USV1_TX1",
        QStringLiteral("Path Loss Field"));
    return emitter;
}

class FakePropagationSolver final : public PEPropagationSolver {
public:
    FakePropagationSolver()
        : PEPropagationSolver(ModelType::PE, nullptr) {}

    GridMatrix compute2D(
        Transmitter_PE_data peData,
        EnvironmentData env,
        double receiverAntennaHeight) override {
        Q_UNUSED(peData);
        receiverHeights.push_back(receiverAntennaHeight);
        return GridMatrix::Constant(2, 2, receiverAntennaHeight + env.windSpeed);
    }

    std::vector<double> receiverHeights;
};

}  // namespace

TEST_CASE("simSchedulerCtx assembles successful task result with derived metrics", "[result][scheduler]") {
    const DataModel::DataSnapshot snapshot = makeSnapshot();

    EMCComputationResult computation;
    computation.status = SimulationResultStatus::Succeeded;
    computation.aggregatedField = makeField(
        ScalarFieldQuantity::AggregatedPowerDbm,
        "aggregated-power",
        QStringLiteral("Aggregated Power Field"));
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
    REQUIRE(result.aggregatedField.values.size() == 4);
    REQUIRE(result.derivedMetrics.available);
    REQUIRE(result.derivedMetrics.scf.linkCount >= 1);
    REQUIRE(result.derivedMetrics.s3i.referenceTransmitterId == QStringLiteral("USV1_TX1"));
    REQUIRE(result.derivedMetrics.s3i.referenceReceiverId == QStringLiteral("USV2_RX1"));
    REQUIRE(result.derivedMetrics.dDesense.victimReceiverId == QStringLiteral("USV2_RX1"));
    REQUIRE_FALSE(result.derivedMetrics.tElev.field.values.empty());
    REQUIRE_FALSE(result.derivedMetrics.dDesense.field.values.empty());

    const auto validation = result.validate();
    INFO(validation.second.toStdString());
    REQUIRE(validation.first);
}

TEST_CASE("EMC_Engine forwards fieldPlaneHeightM to propagation solver", "[result][scheduler][engine]") {
    DataModel::DataSnapshot snapshot = makeSnapshot();
    snapshot.emcAnalysisConfig.fieldPlaneHeightM = 12.5;

    auto fleet = TransferToEngine::convertDataModelToFleet(snapshot);
    REQUIRE(fleet != nullptr);

    auto fakeSolver = std::make_unique<FakePropagationSolver>();
    auto* fakeSolverPtr = fakeSolver.get();

    EMC_Engine engine(
        ModelType::PE,
        std::move(fleet),
        snapshot,
        std::move(fakeSolver));

    engine.do_PE_computing();

    REQUIRE(engine.completedSuccessfully());
    REQUIRE(fakeSolverPtr->receiverHeights.size() == 2);
    REQUIRE(fakeSolverPtr->receiverHeights[0] == Catch::Approx(12.5));
    REQUIRE(fakeSolverPtr->receiverHeights[1] == Catch::Approx(12.5));

    const auto& result = engine.computationResult();
    REQUIRE(result.status == SimulationResultStatus::Succeeded);
    REQUIRE(result.emitterResults.size() == 1);
    REQUIRE(result.emitterResults.front().field2D.values.front() == Catch::Approx(15.5));
}
