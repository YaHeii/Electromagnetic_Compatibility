#include "Utils/Reportflow/ReportJobExporter.h"

#include <array>

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>

#include "Interface/ReportFlowContract.h"
#include "Resource/ui/SimulationResultCatalog.h"
#include "Utils/Reportflow/ReportContextBuilder.h"
#include "Utils/Reportflow/ReportFlowJsonIO.h"
#include "Utils/Reportflow/StandardInputExporter.h"
#include "Utils/SimulationChartRenderer.h"

namespace {

struct ChartExportDefinition {
    SimulationChartKey key;
    const char* assetFile;
};

bool renderChartImage(
    const SimulationChartPayload& payload,
    const QString& filePath,
    QString* errorMessage) {
    if (!payload.available) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("报告图缺少数据：%1").arg(payload.title);
        }
        return false;
    }

    constexpr int kWidth = 1600;
    constexpr int kHeight = 960;
    QCustomPlot plot;
    plot.resize(kWidth, kHeight);

    switch (payload.payloadType) {
    case SimulationChartPayloadType::ScalarField2D:
        if (!payload.scalarField) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("标量场图缺少数据指针：%1").arg(payload.title);
            }
            return false;
        }
        SimulationChartRenderer::renderScalarFieldDetail(
            *payload.scalarField,
            payload.key,
            &plot,
            false);
        break;

    case SimulationChartPayloadType::Series1D:
        if (!payload.primarySeries) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("曲线图缺少主序列：%1").arg(payload.title);
            }
            return false;
        }
        SimulationChartRenderer::renderSeriesDetail(
            *payload.primarySeries,
            payload.secondarySeries,
            payload.key,
            &plot,
            false);
        break;

    case SimulationChartPayloadType::LabeledMatrix2D:
        if (!payload.matrix) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("矩阵图缺少矩阵数据：%1").arg(payload.title);
            }
            return false;
        }
        SimulationChartRenderer::renderMatrixDetail(
            *payload.matrix,
            payload.key,
            &plot,
            false);
        break;
    }

    plot.replot();
    if (!plot.savePng(filePath, kWidth, kHeight, 1.0)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("导出 PNG 失败：%1").arg(filePath);
        }
        return false;
    }
    return true;
}

}  // namespace

ReportJobExportResult ReportJobExporter::exportBundle(
    const SimulationTaskResult& result,
    const QString& workRootDir,
    ReportFlow::JobMode mode) {
    ReportJobExportResult exportResult;

    if (result.status != SimulationResultStatus::Succeeded) {
        exportResult.errorMessage = QStringLiteral("只有成功态仿真结果才能创建报告任务");
        return exportResult;
    }

    const auto validation = result.validate();
    if (!validation.first) {
        exportResult.errorMessage = QStringLiteral("仿真结果校验失败：%1").arg(validation.second);
        return exportResult;
    }

    if (!ReportFlowJsonIO::ensureDirectory(workRootDir, &exportResult.errorMessage)) {
        return exportResult;
    }

    QDir workRoot(workRootDir);
    exportResult.jobDirectory = workRoot.filePath(result.taskId);
    if (QFileInfo::exists(exportResult.jobDirectory)) {
        exportResult.errorMessage = QStringLiteral("报告任务目录已存在：%1").arg(exportResult.jobDirectory);
        return exportResult;
    }

    exportResult.assetsDirectory = QDir(exportResult.jobDirectory).filePath(QString::fromLatin1(ReportFlow::kAssetsDirName));
    exportResult.outputsDirectory = QDir(exportResult.jobDirectory).filePath(QString::fromLatin1(ReportFlow::kOutputsDirName));
    exportResult.logsDirectory = QDir(exportResult.jobDirectory).filePath(QString::fromLatin1(ReportFlow::kLogsDirName));
    exportResult.experimentsDirectory = QDir(exportResult.jobDirectory).filePath(QString::fromLatin1(ReportFlow::kExperimentsDirName));

    if (!ReportFlowJsonIO::ensureDirectory(exportResult.assetsDirectory, &exportResult.errorMessage) ||
        !ReportFlowJsonIO::ensureDirectory(exportResult.outputsDirectory, &exportResult.errorMessage) ||
        !ReportFlowJsonIO::ensureDirectory(exportResult.logsDirectory, &exportResult.errorMessage) ||
        !ReportFlowJsonIO::ensureDirectory(exportResult.experimentsDirectory, &exportResult.errorMessage)) {
        return exportResult;
    }

    exportResult.requestFilePath = QDir(exportResult.jobDirectory).filePath(QString::fromLatin1(ReportFlow::kRequestFileName));
    exportResult.baselineInputFilePath = QDir(exportResult.jobDirectory).filePath(QString::fromLatin1(ReportFlow::kBaselineInputFileName));
    exportResult.simulationResultFilePath = QDir(exportResult.jobDirectory).filePath(QString::fromLatin1(ReportFlow::kSimulationResultFileName));
    exportResult.reportContextFilePath = QDir(exportResult.jobDirectory).filePath(QString::fromLatin1(ReportFlow::kReportContextFileName));
    exportResult.statusFilePath = QDir(exportResult.jobDirectory).filePath(QString::fromLatin1(ReportFlow::kStatusFileName));

    if (!StandardInputExporter::writeJsoncFile(
            exportResult.baselineInputFilePath,
            result.inputSnapshot,
            &exportResult.errorMessage)) {
        return exportResult;
    }

    if (!ReportFlowJsonIO::writeJsonFile(
            exportResult.simulationResultFilePath,
            ReportFlowJsonIO::serializeSimulationTaskResult(result),
            &exportResult.errorMessage)) {
        return exportResult;
    }

    if (!ReportFlowJsonIO::writeJsonFile(
            exportResult.reportContextFilePath,
            ReportContextBuilder::build(result),
            &exportResult.errorMessage)) {
        return exportResult;
    }

    static constexpr std::array<ChartExportDefinition, 6> kCharts = {{
        {SimulationChartKey::AggregatedField, ReportFlow::kAggregatedFieldAssetFile},
        {SimulationChartKey::ReferenceEmitterPathLoss, ReportFlow::kReferenceEmitterAssetFile},
        {SimulationChartKey::ScfMatrix, ReportFlow::kScfAssetFile},
        {SimulationChartKey::S3iCurve, ReportFlow::kS3iAssetFile},
        {SimulationChartKey::TElevField, ReportFlow::kTElevAssetFile},
        {SimulationChartKey::DDesenseField, ReportFlow::kDDesenseAssetFile},
    }};

    exportResult.assetFilePaths.reserve(kCharts.size());
    for (const ChartExportDefinition& chart : kCharts) {
        const SimulationChartPayload payload = SimulationResultCatalog::payloadForKey(result, chart.key);
        const QString assetFilePath = QDir(exportResult.jobDirectory).filePath(QString::fromLatin1(chart.assetFile));
        if (!renderChartImage(payload, assetFilePath, &exportResult.errorMessage)) {
            return exportResult;
        }
        exportResult.assetFilePaths.push_back(assetFilePath);
    }

    ReportFlow::Request request;
    request.taskId = result.taskId;
    request.mode = mode;
    request.inputFiles.baselineInputFileName = QString::fromLatin1(ReportFlow::kBaselineInputFileName);
    if (mode == ReportFlow::JobMode::AgentExperiment) {
        ReportFlow::AgentConfig agent;
        agent.goalMode = QString::fromLatin1(ReportFlow::kGoalModeImprovement);
        agent.maxExperimentCount = 5;
        request.agent = agent;
    }

    if (!ReportFlowJsonIO::writeJsonFile(
            exportResult.requestFilePath,
            ReportFlowJsonIO::buildRequestObject(request),
            &exportResult.errorMessage)) {
        return exportResult;
    }

    ReportFlow::Status status;
    status.taskId = result.taskId;
    status.state = ReportFlow::JobState::Pending;
    status.stage = ReportFlow::JobStage::ValidateBundle;
    status.updatedAtUtcMs = QDateTime::currentMSecsSinceEpoch();

    if (!ReportFlowJsonIO::writeJsonFile(
            exportResult.statusFilePath,
            ReportFlowJsonIO::buildStatusObject(status),
            &exportResult.errorMessage)) {
        return exportResult;
    }

    exportResult.success = true;
    return exportResult;
}
