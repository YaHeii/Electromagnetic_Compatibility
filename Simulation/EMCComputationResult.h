#pragma once

#include "Interface/SimulationResult.h"

struct EMCComputationResult {
    SimulationResultStatus status{SimulationResultStatus::Failed};
    QString errorMessage;
    ScalarField2D aggregatedField;
    std::vector<EmitterResult> emitterResults;
};
