#include "Simulation/simSchedulerCtx.h"

#include <utility>

#include <QDateTime>
#include <QUuid>

#include <spdlog/spdlog.h>
#include "Interface/TransferToEngin.h"
#include "Simulation/EMCComputationResult.h"
#include "Simulation/EMCMetricsCalculator.h"
#include "Simulation/PEPropagationSolver.h"
#include "Simulation/EMC_Engine.h"

simSchedulerCtx::simSchedulerCtx(
    ModelType modelType,
    DataSnapshot inputSnapshot,
    FormationSource formationSource,
    std::optional<int> presetFormationId)
    : _modelType(modelType),
      _inputSnapshot(std::move(inputSnapshot)),
      _formationSource(formationSource),
      _presetFormationId(std::move(presetFormationId)) {}

simSchedulerCtx::~simSchedulerCtx() = default;

SimulationTaskResult simSchedulerCtx::run() {
    const qint64 startedAtUtcMs = QDateTime::currentMSecsSinceEpoch();

    EMCComputationResult failedComputation;
    failedComputation.status = SimulationResultStatus::Failed;

    if (_stopRequested.load()) {
        EMCComputationResult cancelledComputation;
        cancelledComputation.status = SimulationResultStatus::Cancelled;
        return assembleResult(
            _inputSnapshot,
            _formationSource,
            _presetFormationId,
            _modelType,
            startedAtUtcMs,
            QDateTime::currentMSecsSinceEpoch(),
            cancelledComputation);
    }

    auto fleet = TransferToEngine::convertDataModelToFleet(_inputSnapshot);
    if (!fleet) {
        failedComputation.errorMessage = QStringLiteral("无法从冻结快照构建 Fleet");
        return assembleResult(
            _inputSnapshot,
            _formationSource,
            _presetFormationId,
            _modelType,
            startedAtUtcMs,
            QDateTime::currentMSecsSinceEpoch(),
            failedComputation);
    }

    _engine = std::make_unique<EMC_Engine>(_modelType, std::move(fleet), _inputSnapshot);
    if (_stopRequested.load()) {
        _engine->stop();
    }

    _engine->do_PE_computing();
    const qint64 finishedAtUtcMs = QDateTime::currentMSecsSinceEpoch();
    const SimulationTaskResult result = assembleResult(
        _inputSnapshot,
        _formationSource,
        _presetFormationId,
        _modelType,
        startedAtUtcMs,
        finishedAtUtcMs,
        _engine->computationResult());
    _engine.reset();
    return result;
}

void simSchedulerCtx::requestStop() {
    _stopRequested = true;
    if (_engine) {
        _engine->stop();
    }
}

bool simSchedulerCtx::stopRequested() const {
    return _stopRequested.load();
}

SimulationTaskResult simSchedulerCtx::assembleResult(
    const DataSnapshot& inputSnapshot,
    FormationSource formationSource,
    std::optional<int> presetFormationId,
    ModelType modelType,
    qint64 startedAtUtcMs,
    qint64 finishedAtUtcMs,
    const EMCComputationResult& computationResult) {
    SimulationTaskResult result;
    result.resultSchemaVersion = QStringLiteral("1.0.0");
    result.taskId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    result.modelType = modelType;
    result.status = computationResult.status;
    result.formationSource = formationSource;
    result.presetFormationId = std::move(presetFormationId);
    result.startedAtUtcMs = startedAtUtcMs;
    result.finishedAtUtcMs = finishedAtUtcMs;
    result.durationMs = finishedAtUtcMs - startedAtUtcMs;
    result.errorMessage = computationResult.errorMessage;
    result.inputSnapshot = inputSnapshot;
    result.aggregatedField = computationResult.aggregatedField;
    result.emitterResults = computationResult.emitterResults;
    result.derivedMetrics.available = false;

    if (result.status == SimulationResultStatus::Succeeded) {
        const auto metricsResult = EMCMetricsCalculator::compute(
            modelType,
            inputSnapshot,
            computationResult);
        if (!metricsResult.success) {
            result.status = SimulationResultStatus::Failed;
            result.errorMessage = metricsResult.errorMessage;
        } else {
            result.derivedMetrics = metricsResult.metrics;
        }
    }

    switch (result.status) {
    case SimulationResultStatus::Succeeded:
        result.summaryText = QStringLiteral("仿真完成");
        break;
    case SimulationResultStatus::Failed:
        result.summaryText = QStringLiteral("仿真失败");
        break;
    case SimulationResultStatus::Cancelled:
        result.summaryText = QStringLiteral("仿真已取消");
        break;
    }

    const auto validation = result.validate();
    if (!validation.first) {
        spdlog::error("SimulationTaskResult assembly validation failed: {}", validation.second.toStdString());
    }
    return result;
}
