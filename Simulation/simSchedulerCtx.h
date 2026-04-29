#pragma once

#include <atomic>
#include <memory>
#include <optional>

#include "Interface/SimulationResult.h"

class EMC_Engine;

struct EMCComputationResult;

class simSchedulerCtx {
public:
    using DataSnapshot = DataModel::DataSnapshot;

    simSchedulerCtx(
        ModelType modelType,
        DataSnapshot inputSnapshot,
        FormationSource formationSource,
        std::optional<int> presetFormationId);
    ~simSchedulerCtx();

    SimulationTaskResult run();
    void requestStop();
    bool stopRequested() const;

    static SimulationTaskResult assembleResult(
        const DataSnapshot& inputSnapshot,
        FormationSource formationSource,
        std::optional<int> presetFormationId,
        ModelType modelType,
        qint64 startedAtUtcMs,
        qint64 finishedAtUtcMs,
        const EMCComputationResult& computationResult);

private:
    ModelType _modelType{ModelType::PE};
    DataSnapshot _inputSnapshot;
    FormationSource _formationSource{FormationSource::ManualInput};
    std::optional<int> _presetFormationId;
    std::atomic<bool> _stopRequested{false};
    std::unique_ptr<EMC_Engine> _engine;
};
