#pragma once

#include <optional>
#include <vector>

#include <QString>

#include "Interface/SimulationResult.h"

enum class SimulationChartKey {
    AggregatedField,
    ReferenceEmitterPathLoss,
    ScfMatrix,
    S3iCurve,
    TElevField,
    DDesenseField
};

enum class SimulationChartPayloadType {
    ScalarField2D,
    Series1D,
    LabeledMatrix2D
};

struct SimulationChartCardDescriptor {
    SimulationChartKey key{SimulationChartKey::AggregatedField};
    SimulationChartPayloadType payloadType{SimulationChartPayloadType::ScalarField2D};
    QString title;
    QString subtitle;
    bool available{false};
};

struct SimulationChartPayload {
    SimulationChartKey key{SimulationChartKey::AggregatedField};
    SimulationChartPayloadType payloadType{SimulationChartPayloadType::ScalarField2D};
    QString title;
    QString subtitle;
    QString detailSummary;
    bool available{false};
    const ScalarField2D* scalarField{nullptr};
    const Series1D* primarySeries{nullptr};
    const Series1D* secondarySeries{nullptr};
    const LabeledMatrix2D* matrix{nullptr};
};

class SimulationResultCatalog {
public:
    static std::vector<SimulationChartCardDescriptor> buildCards(const SimulationTaskResult& result);
    static SimulationChartPayload payloadForKey(
        const SimulationTaskResult& result,
        SimulationChartKey key);

private:
    static const EmitterResult* findReferenceEmitterResult(const SimulationTaskResult& result);
};
