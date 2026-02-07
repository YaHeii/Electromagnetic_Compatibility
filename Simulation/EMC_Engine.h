#pragma once
#include <vector>
#include <string>
#include "PEModel.h"
#include <omp.h>
#include "Models/Equipment.h"
#include "Models/fleet.h"
#include "Interface/DataModel.h"
#include <stdexcept>
#include <spdlog/spdlog.h>
#include <fstream>
#include "Utils/PaintImage.hpp"
#include "Interface/TransferToPEdata.hpp"
#include "Interface/TransferToFile.hpp"

enum class ModelType {
    PE,
    RayModel
};
using GridMap = std::vector<std::vector<double>>;
using Matrix = std::vector<std::vector<double>>;
using LineMap = std::vector<double>;
using GridMatrix = Eigen::MatrixXd;
class Propagation_Engine;

class EMC_Engine : public QObject {
    Q_OBJECT
public:
    EMC_Engine(ModelType modelType, std::unique_ptr<Fleet> fleet)
        : _modelType(modelType),
          _fleet(std::move(fleet)),
          _dataSnapshot(DataModel::instance()->createSnapshot()),
          _propagationEngine(nullptr) 
    {
        if (!_fleet) {
            spdlog::error("EMC_Engine initialization failed: fleet is null");
        }
		_env = _dataSnapshot.environmentConfig;
    }
	void InitPropagationEngine();
    void do_PE_computing();
    GridMap do_PE_test();
    void do_Validation_TwoRay();
    void do_Validation_Roughness();
    void do_Validation_DuctLeakage();
    void stop() {
        isStopRequested = true;
    }

private:
    std::atomic<bool> isStopRequested{false}; 
    GridMap _LossGrid;
    std::vector<Transmitter_PE_data> _peDataList;
    using DataSnapshot = DataModel::DataSnapshot; 
	std::unique_ptr<Fleet> _fleet;
	DataSnapshot _dataSnapshot;
    ModelType _modelType;
    Propagation_Engine* _propagationEngine;
    EnvironmentData _env;
signals:
    void peComputationFinished(const GridMap& lossGrid);
};


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