#pragma once

#include <atomic>
#include <memory>
#include <vector>

#include <QObject>
#include "Models/fleet.h"
#include "Interface/SimulationResult.h"
#include "Simulation/EMCComputationResult.h"

class Fleet;
class PEPropagationSolver;

using GridMap = std::vector<std::vector<double>>;

class EMC_Engine : public QObject {
    Q_OBJECT
public:
    using DataSnapshot = DataModel::DataSnapshot;

    EMC_Engine(ModelType modelType, std::unique_ptr<Fleet> fleet, DataSnapshot dataSnapshot);
    ~EMC_Engine();
    //TODO: 改造计算接口,依据Validation_test中experiment3 添加计算
    void do_PE_computing();
    // TODO: 目前计算返回的仍然是GridMap,没有与ScalarField2D对齐
    GridMap do_PE_test();
    void do_Validation_TwoRay();
    void do_Validation_Roughness();
    void do_Validation_DuctLeakage();

    void stop() {
        isStopRequested = true;
    }

    bool completedSuccessfully() const {
        return _completedSuccessfully;
    }

    bool wasCancelled() const {
        return _wasCancelled;
    }

    bool stopRequested() const {
        return isStopRequested.load();
    }

    QString lastErrorMessage() const {
        return _lastErrorMessage;
    }

    const EMCComputationResult& computationResult() const {
        return _computationResult;
    }

    const DataSnapshot& inputSnapshot() const {
        return _dataSnapshot;
    }

private:
    void markFailed(const QString& errorMessage);
    void markCancelled();

    std::atomic<bool> isStopRequested{false};
    std::unique_ptr<Fleet> _fleet;
    DataSnapshot _dataSnapshot;
    ModelType _modelType{ModelType::PE};
    std::unique_ptr<PEPropagationSolver> _propagationSolver;
    EnvironmentData _env;
    EMCComputationResult _computationResult;
    bool _completedSuccessfully{false};
    bool _wasCancelled{false};
    QString _lastErrorMessage;

signals:
    void peComputationFinished(const GridMap& lossGrid);
};
