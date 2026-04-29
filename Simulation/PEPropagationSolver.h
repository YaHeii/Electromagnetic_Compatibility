#pragma once

#include <vector>

#include <Eigen/Dense>

#include "Interface/DataModel.h"
#include "Interface/SimulationResult.h"
#include "Interface/TransferToPEdata.hpp"
#include "Models/fleet.h"

using GridMap = std::vector<std::vector<double>>;
using Matrix = std::vector<std::vector<double>>;
using LineMap = std::vector<double>;
using GridMatrix = Eigen::MatrixXd;

class PEPropagationSolver {
public:
    PEPropagationSolver(ModelType modelType, const Fleet* fleet);
    // XXX:compute1D is only to use for experiment
    LineMap compute1D(Transmitter_PE_data peData, EnvironmentData env, double receiverAntennaHeight);
    GridMatrix compute2D(Transmitter_PE_data peData, EnvironmentData env, double receiverAntennaHeight);

private:
    const Fleet* _fleet{nullptr};
    ModelType _modelType{ModelType::PE};
};
