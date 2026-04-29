#pragma once

#include <atomic>
#include <memory>
#include <vector>

#include <omp.h>
#include <spdlog/spdlog.h>
#include <Eigen/Dense>
#include "Interface/DataModel.h"
#include "Interface/TransferToPEdata.hpp"
#include "Models/fleet.h"

#include "Utils/PaintImage.hpp"

enum class ModelType {
    PE,
    RayModel
};
using GridMap = std::vector<std::vector<double>>;
using Matrix = std::vector<std::vector<double>>;
using LineMap = std::vector<double>;
using GridMatrix = Eigen::MatrixXd;

class Propagation_Engine {
public:
    Propagation_Engine(ModelType model_type, const Fleet* fleet)
        : _model_type(model_type), _fleet(fleet) {}
    LineMap PEmodel_computing1D(Transmitter_PE_data PEdata, EnvironmentData env, double reciever_antenna_height);
    GridMatrix PEmodel_computing2D(Transmitter_PE_data PEdata, EnvironmentData env, double reciever_antenna_height);

private:
    const Fleet* _fleet;
    ModelType _model_type;
    Transmitter_PE_data _PEdata;
    GridMap _LossGrid;
    LineMap _LossLine;
    EnvironmentData _env;
};


class EMC_Engine : public QObject {
    Q_OBJECT
public:
    using DataSnapshot = DataModel::DataSnapshot;

    EMC_Engine(ModelType modelType, std::unique_ptr<Fleet> fleet, DataSnapshot dataSnapshot)
        : _fleet(std::move(fleet)),
          _dataSnapshot(std::move(dataSnapshot)),
          _modelType(modelType) {
        if (!_fleet) {
            spdlog::error("EMC_Engine initialization failed: fleet is null");
        }
        _env = _dataSnapshot.environmentConfig;
        _propagationEngine = std::make_unique<Propagation_Engine>(_modelType, _fleet.get());
    }

    void do_PE_computing();
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

    const GridMap& lossGrid() const {
        return _LossGrid;
    }

    const DataSnapshot& inputSnapshot() const {
        return _dataSnapshot;
    }

private:
    void markFailed(const QString& errorMessage) {
        _completedSuccessfully = false;
        _wasCancelled = false;
        _lastErrorMessage = errorMessage;
        spdlog::error("EMC_Engine failed: {}", errorMessage.toStdString());
    }

    void markCancelled() {
        _completedSuccessfully = false;
        _wasCancelled = true;
        _lastErrorMessage.clear();
        _LossGrid.clear();
        spdlog::info("EMC_Engine cancellation requested, aborting current task.");
    }

    std::atomic<bool> isStopRequested{ false };
    GridMap _LossGrid;
    std::vector<Transmitter_PE_data> _peDataList;
    std::unique_ptr<Fleet> _fleet;
    DataSnapshot _dataSnapshot;
    ModelType _modelType;
    std::unique_ptr<Propagation_Engine> _propagationEngine;
    EnvironmentData _env;
    bool _completedSuccessfully{false};
    bool _wasCancelled{false};
    QString _lastErrorMessage;
signals:
    void peComputationFinished(const GridMap& lossGrid);
};
