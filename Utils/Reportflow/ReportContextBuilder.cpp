#include "Utils/Reportflow/ReportContextBuilder.h"

#include <array>

#include <QJsonArray>

#include "Interface/ReportFlowContract.h"
#include "Resource/ui/SimulationResultCatalog.h"

namespace {

struct ChartDefinition {
    SimulationChartKey key;
    const char* chartId;
    const char* assetFile;
};

QString modelTypeToString(ModelType modelType) {
    switch (modelType) {
    case ModelType::PE:
        return QStringLiteral("PE");
    case ModelType::RayModel:
        return QStringLiteral("RayModel");
    }
    return QStringLiteral("PE");
}

QString simulationStatusToString(SimulationResultStatus status) {
    switch (status) {
    case SimulationResultStatus::Succeeded:
        return QStringLiteral("Succeeded");
    case SimulationResultStatus::Failed:
        return QStringLiteral("Failed");
    case SimulationResultStatus::Cancelled:
        return QStringLiteral("Cancelled");
    }
    return QStringLiteral("Succeeded");
}

QString formationSourceToString(FormationSource source) {
    switch (source) {
    case FormationSource::ManualInput:
        return QStringLiteral("ManualInput");
    case FormationSource::PresetFormation:
        return QStringLiteral("PresetFormation");
    }
    return QStringLiteral("ManualInput");
}

QString payloadTypeToString(SimulationChartPayloadType payloadType) {
    switch (payloadType) {
    case SimulationChartPayloadType::ScalarField2D:
        return QStringLiteral("ScalarField2D");
    case SimulationChartPayloadType::Series1D:
        return QStringLiteral("Series1D");
    case SimulationChartPayloadType::LabeledMatrix2D:
        return QStringLiteral("LabeledMatrix2D");
    }
    return QStringLiteral("ScalarField2D");
}

QJsonObject buildTaskObject(const SimulationTaskResult& result) {
    QJsonObject object;
    object.insert(QStringLiteral("taskId"), result.taskId);
    object.insert(QStringLiteral("modelType"), modelTypeToString(result.modelType));
    object.insert(QStringLiteral("status"), simulationStatusToString(result.status));
    object.insert(QStringLiteral("formationSource"), formationSourceToString(result.formationSource));
    object.insert(
        QStringLiteral("presetFormationId"),
        result.presetFormationId.has_value()
            ? QJsonValue(result.presetFormationId.value())
            : QJsonValue(QJsonValue::Null));
    object.insert(QStringLiteral("startedAtUtcMs"), result.startedAtUtcMs);
    object.insert(QStringLiteral("finishedAtUtcMs"), result.finishedAtUtcMs);
    object.insert(QStringLiteral("durationMs"), result.durationMs);
    object.insert(QStringLiteral("errorMessage"), result.errorMessage);
    return object;
}

QJsonObject buildAnalysisConfigObject(const SimulationTaskResult& result) {
    const EMCAnalysisConfig& config = result.inputSnapshot.emcAnalysisConfig;
    const EnvironmentData& environment = result.inputSnapshot.environmentConfig;

    QJsonObject object;
    object.insert(QStringLiteral("fieldPlaneHeightM"), config.fieldPlaneHeightM);
    object.insert(QStringLiteral("referenceTransmitterId"), config.referenceTransmitterId);
    object.insert(QStringLiteral("referenceReceiverId"), config.referenceReceiverId);
    object.insert(QStringLiteral("s3iBaselineWindSpeedMps"), config.s3iBaselineWindSpeedMps);
    object.insert(QStringLiteral("currentWindSpeedMps"), environment.windSpeed);
    return object;
}

QJsonObject buildMetricsSummaryObject(const SimulationTaskResult& result) {
    QJsonObject root;
    root.insert(QStringLiteral("available"), result.derivedMetrics.available);

    if (!result.derivedMetrics.available) {
        return root;
    }

    QJsonObject scf;
    scf.insert(QStringLiteral("scalarDb"), result.derivedMetrics.scf.scalarDb);
    scf.insert(QStringLiteral("thermalNoiseFloorDbm"), result.derivedMetrics.scf.thermalNoiseFloorDbm);
    scf.insert(QStringLiteral("linkCount"), result.derivedMetrics.scf.linkCount);
    root.insert(QStringLiteral("scf"), scf);

    QJsonObject s3i;
    s3i.insert(QStringLiteral("scalarDb"), result.derivedMetrics.s3i.scalarDb);
    s3i.insert(QStringLiteral("referenceTransmitterId"), result.derivedMetrics.s3i.referenceTransmitterId);
    s3i.insert(QStringLiteral("referenceReceiverId"), result.derivedMetrics.s3i.referenceReceiverId);
    s3i.insert(QStringLiteral("baselineWindSpeedMps"), result.derivedMetrics.s3i.baselineWindSpeedMps);
    s3i.insert(QStringLiteral("currentWindSpeedMps"), result.derivedMetrics.s3i.currentWindSpeedMps);
    root.insert(QStringLiteral("s3i"), s3i);

    QJsonObject tElev;
    tElev.insert(QStringLiteral("maxDb"), result.derivedMetrics.tElev.maxDb);
    tElev.insert(QStringLiteral("meanDb"), result.derivedMetrics.tElev.meanDb);
    root.insert(QStringLiteral("tElev"), tElev);

    QJsonObject dDesense;
    dDesense.insert(QStringLiteral("victimReceiverId"), result.derivedMetrics.dDesense.victimReceiverId);
    dDesense.insert(QStringLiteral("peakDb"), result.derivedMetrics.dDesense.peakDb);
    dDesense.insert(QStringLiteral("coveragePercent"), result.derivedMetrics.dDesense.coveragePercent);
    dDesense.insert(QStringLiteral("adiDbPerSquareMeter"), result.derivedMetrics.dDesense.adiDbPerSquareMeter);
    root.insert(QStringLiteral("dDesense"), dDesense);

    return root;
}

QJsonArray buildChartsArray(const SimulationTaskResult& result) {
    static constexpr std::array<ChartDefinition, 6> kCharts = {{
        {SimulationChartKey::AggregatedField, ReportFlow::kChartIdAggregatedField, ReportFlow::kAggregatedFieldAssetFile},
        {SimulationChartKey::ReferenceEmitterPathLoss, ReportFlow::kChartIdReferenceEmitter, ReportFlow::kReferenceEmitterAssetFile},
        {SimulationChartKey::ScfMatrix, ReportFlow::kChartIdScf, ReportFlow::kScfAssetFile},
        {SimulationChartKey::S3iCurve, ReportFlow::kChartIdS3i, ReportFlow::kS3iAssetFile},
        {SimulationChartKey::TElevField, ReportFlow::kChartIdTElev, ReportFlow::kTElevAssetFile},
        {SimulationChartKey::DDesenseField, ReportFlow::kChartIdDDesense, ReportFlow::kDDesenseAssetFile},
    }};

    QJsonArray charts;

    for (const ChartDefinition& definition : kCharts) {
        const SimulationChartPayload payload = SimulationResultCatalog::payloadForKey(result, definition.key);
        QJsonObject object;
        object.insert(QStringLiteral("chartId"), QString::fromLatin1(definition.chartId));
        object.insert(QStringLiteral("title"), payload.title);
        object.insert(QStringLiteral("subtitle"), payload.subtitle);
        object.insert(QStringLiteral("detailSummary"), payload.detailSummary);
        object.insert(QStringLiteral("payloadType"), payloadTypeToString(payload.payloadType));
        object.insert(QStringLiteral("available"), payload.available);
        object.insert(QStringLiteral("assetFile"), QString::fromLatin1(definition.assetFile));
        charts.append(object);
    }

    return charts;
}

}  // namespace

QJsonObject ReportContextBuilder::build(const SimulationTaskResult& result) {
    QJsonObject root;
    root.insert(QStringLiteral("reportContextVersion"), QString::fromLatin1(ReportFlow::kReportContextVersion));
    root.insert(QStringLiteral("summaryText"), result.summaryText);
    root.insert(QStringLiteral("task"), buildTaskObject(result));
    root.insert(QStringLiteral("analysisConfig"), buildAnalysisConfigObject(result));
    root.insert(QStringLiteral("metricsSummary"), buildMetricsSummaryObject(result));
    root.insert(QStringLiteral("charts"), buildChartsArray(result));
    return root;
}
