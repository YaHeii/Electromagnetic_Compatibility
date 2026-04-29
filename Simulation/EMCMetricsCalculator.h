#pragma once

#include "Interface/SimulationResult.h"

struct EMCComputationResult;

class EMCMetricsCalculator {
public:
    struct ComputeResult {
        bool success{false};
        QString errorMessage;
        DerivedMetrics metrics;
    };

    static ComputeResult compute(
        ModelType modelType,
        const DataModel::DataSnapshot& inputSnapshot,
        const EMCComputationResult& computationResult);
};
