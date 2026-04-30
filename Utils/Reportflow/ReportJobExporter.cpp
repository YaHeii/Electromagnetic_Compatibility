#include "Utils/Reportflow/ReportJobExporter.h"

#include <array>

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

#include "Interface/ReportFlowContract.h"
#include "Resource/ui/SimulationResultCatalog.h"
#include "Utils/Reportflow/ReportContextBuilder.h"
#include "Utils/SimulationChartRenderer.h"

namespace {

struct ChartExportDefinition {
    SimulationChartKey key;
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

QString emitterStatusToString(EmitterResultStatus status) {
    switch (status) {
    case EmitterResultStatus::Succeeded:
        return QStringLiteral("Succeeded");
    case EmitterResultStatus::Failed:
        return QStringLiteral("Failed");
    case EmitterResultStatus::Cancelled:
        return QStringLiteral("Cancelled");
    case EmitterResultStatus::Skipped:
        return QStringLiteral("Skipped");
    }
    return QStringLiteral("Succeeded");
}

QString scalarQuantityToString(ScalarFieldQuantity quantity) {
    switch (quantity) {
    case ScalarFieldQuantity::AggregatedPowerDbm:
        return QStringLiteral("AggregatedPowerDbm");
    case ScalarFieldQuantity::PathLossDb:
        return QStringLiteral("PathLossDb");
    case ScalarFieldQuantity::NoiseElevationDb:
        return QStringLiteral("NoiseElevationDb");
    case ScalarFieldQuantity::DesenseDb:
        return QStringLiteral("DesenseDb");
    }
    return QStringLiteral("AggregatedPowerDbm");
}

QJsonArray toDoubleArray(const std::vector<double>& values) {
    QJsonArray array;
    for (double value : values) {
        array.append(value);
    }
    return array;
}

QJsonArray toStringArray(const std::vector<QString>& values) {
    QJsonArray array;
    for (const QString& value : values) {
        array.append(value);
    }
    return array;
}

QJsonObject serializeScalarField(const ScalarField2D& field) {
    QJsonObject object;
    object.insert(QStringLiteral("fieldId"), field.fieldId);
    object.insert(QStringLiteral("displayName"), field.displayName);
    object.insert(QStringLiteral("quantity"), scalarQuantityToString(field.quantity));
    object.insert(QStringLiteral("valueUnit"), field.valueUnit);
    object.insert(QStringLiteral("axisXUnit"), field.axisXUnit);
    object.insert(QStringLiteral("axisYUnit"), field.axisYUnit);
    object.insert(QStringLiteral("rows"), field.rows);
    object.insert(QStringLiteral("cols"), field.cols);
    object.insert(QStringLiteral("originX"), field.originX);
    object.insert(QStringLiteral("originY"), field.originY);
    object.insert(QStringLiteral("stepX"), field.stepX);
    object.insert(QStringLiteral("stepY"), field.stepY);
    object.insert(
        QStringLiteral("noDataValue"),
        field.noDataValue.has_value()
            ? QJsonValue(field.noDataValue.value())
            : QJsonValue(QJsonValue::Null));
    object.insert(QStringLiteral("values"), toDoubleArray(field.values));
    return object;
}

QJsonObject serializeSeries(const Series1D& series) {
    QJsonObject object;
    object.insert(QStringLiteral("seriesId"), series.seriesId);
    object.insert(QStringLiteral("displayName"), series.displayName);
    object.insert(QStringLiteral("xUnit"), series.xUnit);
    object.insert(QStringLiteral("yUnit"), series.yUnit);
    object.insert(QStringLiteral("xValues"), toDoubleArray(series.xValues));
    object.insert(QStringLiteral("yValues"), toDoubleArray(series.yValues));
    return object;
}

QJsonObject serializeMatrix(const LabeledMatrix2D& matrix) {
    QJsonObject object;
    object.insert(QStringLiteral("matrixId"), matrix.matrixId);
    object.insert(QStringLiteral("displayName"), matrix.displayName);
    object.insert(QStringLiteral("valueUnit"), matrix.valueUnit);
    object.insert(QStringLiteral("rows"), matrix.rows);
    object.insert(QStringLiteral("cols"), matrix.cols);
    object.insert(QStringLiteral("rowLabels"), toStringArray(matrix.rowLabels));
    object.insert(QStringLiteral("colLabels"), toStringArray(matrix.colLabels));
    object.insert(QStringLiteral("values"), toDoubleArray(matrix.values));
    return object;
}

QJsonObject serializeEmitterResult(const EmitterResult& emitterResult) {
    QJsonObject object;
    object.insert(QStringLiteral("emitterId"), emitterResult.emitterId);
    object.insert(QStringLiteral("shipId"), emitterResult.shipId);
    object.insert(QStringLiteral("status"), emitterStatusToString(emitterResult.status));
    object.insert(QStringLiteral("centerFrequencyGHz"), emitterResult.centerFrequencyGHz);
    object.insert(QStringLiteral("transmitPowerDbm"), emitterResult.transmitPowerDbm);
    object.insert(QStringLiteral("worldX"), emitterResult.worldX);
    object.insert(QStringLiteral("worldY"), emitterResult.worldY);
    object.insert(QStringLiteral("worldZ"), emitterResult.worldZ);
    object.insert(QStringLiteral("errorMessage"), emitterResult.errorMessage);
    if (emitterResult.status == EmitterResultStatus::Succeeded) {
        object.insert(QStringLiteral("field2D"), serializeScalarField(emitterResult.field2D));
    }
    return object;
}

QJsonObject serializeScf(const SCFMetric& scf) {
    QJsonObject object;
    object.insert(QStringLiteral("scalarDb"), scf.scalarDb);
    object.insert(QStringLiteral("thermalNoiseFloorDbm"), scf.thermalNoiseFloorDbm);
    object.insert(QStringLiteral("linkCount"), scf.linkCount);
    object.insert(QStringLiteral("couplingMatrix"), serializeMatrix(scf.couplingMatrix));
    return object;
}

QJsonObject serializeS3i(const S3IMetric& s3i) {
    QJsonObject object;
    object.insert(QStringLiteral("scalarDb"), s3i.scalarDb);
    object.insert(QStringLiteral("referenceTransmitterId"), s3i.referenceTransmitterId);
    object.insert(QStringLiteral("referenceReceiverId"), s3i.referenceReceiverId);
    object.insert(QStringLiteral("baselineWindSpeedMps"), s3i.baselineWindSpeedMps);
    object.insert(QStringLiteral("currentWindSpeedMps"), s3i.currentWindSpeedMps);
    object.insert(QStringLiteral("calmCurve"), serializeSeries(s3i.calmCurve));
    object.insert(QStringLiteral("currentCurve"), serializeSeries(s3i.currentCurve));
    return object;
}

QJsonObject serializeTElev(const TElevMetric& tElev) {
    QJsonObject object;
    object.insert(QStringLiteral("field"), serializeScalarField(tElev.field));
    object.insert(QStringLiteral("maxDb"), tElev.maxDb);
    object.insert(QStringLiteral("meanDb"), tElev.meanDb);
    return object;
}

QJsonObject serializeDDesense(const DDesenseMetric& dDesense) {
    QJsonObject object;
    object.insert(QStringLiteral("field"), serializeScalarField(dDesense.field));
    object.insert(QStringLiteral("victimReceiverId"), dDesense.victimReceiverId);
    object.insert(QStringLiteral("peakDb"), dDesense.peakDb);
    object.insert(QStringLiteral("coveragePercent"), dDesense.coveragePercent);
    object.insert(QStringLiteral("adiDbPerSquareMeter"), dDesense.adiDbPerSquareMeter);
    return object;
}

QJsonObject serializeDerivedMetrics(const DerivedMetrics& metrics) {
    QJsonObject object;
    object.insert(QStringLiteral("available"), metrics.available);
    if (metrics.available) {
        object.insert(QStringLiteral("scf"), serializeScf(metrics.scf));
        object.insert(QStringLiteral("s3i"), serializeS3i(metrics.s3i));
        object.insert(QStringLiteral("tElev"), serializeTElev(metrics.tElev));
        object.insert(QStringLiteral("dDesense"), serializeDDesense(metrics.dDesense));
    }
    return object;
}

QJsonObject serializeEquipment(const EquipmentData& equipment) {
    QJsonObject object;
    object.insert(QStringLiteral("equipmentId"), equipment.equipmentId);
    object.insert(QStringLiteral("equipmentType"), equipment.equipmentType);
    object.insert(QStringLiteral("gainDbi"), equipment.gainDbi);
    object.insert(QStringLiteral("offsetX"), equipment.offsetX);
    object.insert(QStringLiteral("offsetY"), equipment.offsetY);
    object.insert(QStringLiteral("offsetZ"), equipment.offsetZ);
    object.insert(QStringLiteral("receiverCenterFrequencyGHz"), equipment.receiverCenterFrequencyGHz);
    object.insert(QStringLiteral("receiverBandwidthMHz"), equipment.receiverBandwidthMHz);
    object.insert(QStringLiteral("receiverSensitivityDbm"), equipment.receiverSensitivityDbm);
    object.insert(QStringLiteral("receiverInterferenceMarginDb"), equipment.receiverInterferenceMarginDb);
    object.insert(QStringLiteral("receiverSinrMarginDb"), equipment.receiverSinrMarginDb);
    object.insert(QStringLiteral("receiverNoiseFigureDb"), equipment.receiverNoiseFigureDb);
    object.insert(QStringLiteral("transmitterCenterFrequencyGHz"), equipment.transmitterCenterFrequencyGHz);
    object.insert(QStringLiteral("transmitterBandwidthMHz"), equipment.transmitterBandwidthMHz);
    object.insert(QStringLiteral("transmitterPowerDbm"), equipment.transmitterPowerDbm);
    object.insert(QStringLiteral("transmitterAntennaPhiDeg"), equipment.transmitterAntennaPhiDeg);
    object.insert(QStringLiteral("transmitterBeamWidthDeg"), equipment.transmitterBeamWidthDeg);
    object.insert(QStringLiteral("transmitterPolarization"), equipment.transmitterPolarization);
    object.insert(QStringLiteral("transmitterAntennaType"), equipment.transmitterAntennaType);
    object.insert(QStringLiteral("antennaCenterFrequencyGHz"), equipment.antennaCenterFrequencyGHz);
    object.insert(QStringLiteral("antennaBandwidthMHz"), equipment.antennaBandwidthMHz);
    object.insert(QStringLiteral("antennaPowerDbm"), equipment.antennaPowerDbm);
    object.insert(QStringLiteral("antennaPhiDeg"), equipment.antennaPhiDeg);
    object.insert(QStringLiteral("antennaBeamWidthDeg"), equipment.antennaBeamWidthDeg);
    object.insert(QStringLiteral("antennaPolarization"), equipment.antennaPolarization);
    object.insert(QStringLiteral("antennaType"), equipment.antennaType);
    return object;
}

QJsonObject serializeEquipmentOnShip(const EquipmentOnShip& equipmentOnShip) {
    QJsonObject object;
    object.insert(QStringLiteral("equipmentId"), equipmentOnShip.equipmentId);
    object.insert(QStringLiteral("isEnabled"), equipmentOnShip.isEnabled);
    return object;
}

QJsonObject serializeShip(const ShipData& ship) {
    QJsonObject object;
    object.insert(QStringLiteral("shipId"), QString::fromStdString(ship.shipId));
    object.insert(QStringLiteral("worldX"), ship.worldX);
    object.insert(QStringLiteral("worldY"), ship.worldY);
    object.insert(QStringLiteral("worldZ"), ship.worldZ);
    object.insert(QStringLiteral("shipOrientationDeg"), ship.shipOrientationDeg);
    object.insert(QStringLiteral("shipSpeedMps"), ship.shipSpeedMps);

    QJsonArray equipmentRefs;
    for (const EquipmentOnShip& equipmentOnShip : ship.equipmentRefs) {
        equipmentRefs.append(serializeEquipmentOnShip(equipmentOnShip));
    }
    object.insert(QStringLiteral("equipmentRefs"), equipmentRefs);
    return object;
}

QJsonObject serializeEnvironment(const EnvironmentData& environment) {
    QJsonObject object;
    object.insert(QStringLiteral("maxRange"), environment.maxRange);
    object.insert(QStringLiteral("ductHeight"), environment.ductHeight);
    object.insert(QStringLiteral("windSpeed"), environment.windSpeed);
    object.insert(QStringLiteral("dx"), environment.dx);
    object.insert(QStringLiteral("dz"), environment.dz);
    object.insert(QStringLiteral("nz"), environment.nz);
    object.insert(QStringLiteral("angleStepDeg"), environment.angleStepDeg);
    return object;
}

QJsonObject serializeAnalysisConfig(const EMCAnalysisConfig& config) {
    QJsonObject object;
    object.insert(QStringLiteral("fieldPlaneHeightM"), config.fieldPlaneHeightM);
    object.insert(QStringLiteral("referenceTransmitterId"), config.referenceTransmitterId);
    object.insert(QStringLiteral("referenceReceiverId"), config.referenceReceiverId);
    object.insert(QStringLiteral("s3iBaselineWindSpeedMps"), config.s3iBaselineWindSpeedMps);
    return object;
}

QJsonObject serializeSnapshot(const DataModel::DataSnapshot& snapshot) {
    QJsonObject object;

    QJsonArray equipments;
    for (const EquipmentData& equipment : snapshot.allEquipments) {
        equipments.append(serializeEquipment(equipment));
    }

    QJsonArray ships;
    for (const ShipData& ship : snapshot.allShips) {
        ships.append(serializeShip(ship));
    }

    object.insert(QStringLiteral("allEquipments"), equipments);
    object.insert(QStringLiteral("allShips"), ships);
    object.insert(QStringLiteral("environmentConfig"), serializeEnvironment(snapshot.environmentConfig));
    object.insert(QStringLiteral("emcAnalysisConfig"), serializeAnalysisConfig(snapshot.emcAnalysisConfig));
    return object;
}

QJsonObject serializeSimulationTaskResult(const SimulationTaskResult& result) {
    QJsonObject object;
    object.insert(QStringLiteral("resultSchemaVersion"), result.resultSchemaVersion);
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
    object.insert(QStringLiteral("summaryText"), result.summaryText);
    object.insert(QStringLiteral("inputSnapshot"), serializeSnapshot(result.inputSnapshot));

    if (!result.aggregatedField.fieldId.trimmed().isEmpty()) {
        object.insert(QStringLiteral("aggregatedField"), serializeScalarField(result.aggregatedField));
    }

    QJsonArray emitterResults;
    for (const EmitterResult& emitterResult : result.emitterResults) {
        emitterResults.append(serializeEmitterResult(emitterResult));
    }
    object.insert(QStringLiteral("emitterResults"), emitterResults);
    object.insert(QStringLiteral("derivedMetrics"), serializeDerivedMetrics(result.derivedMetrics));
    return object;
}

QJsonObject buildRequestObject(const QString& taskId) {
    QJsonObject inputFiles;
    inputFiles.insert(QStringLiteral("simulationResult"), QString::fromLatin1(ReportFlow::kSimulationResultFileName));
    inputFiles.insert(QStringLiteral("reportContext"), QString::fromLatin1(ReportFlow::kReportContextFileName));

    QJsonObject assetFiles;
    assetFiles.insert(QStringLiteral("aggregatedField"), QString::fromLatin1(ReportFlow::kAggregatedFieldAssetFile));
    assetFiles.insert(QStringLiteral("referenceEmitter"), QString::fromLatin1(ReportFlow::kReferenceEmitterAssetFile));
    assetFiles.insert(QStringLiteral("scf"), QString::fromLatin1(ReportFlow::kScfAssetFile));
    assetFiles.insert(QStringLiteral("s3i"), QString::fromLatin1(ReportFlow::kS3iAssetFile));
    assetFiles.insert(QStringLiteral("tElev"), QString::fromLatin1(ReportFlow::kTElevAssetFile));
    assetFiles.insert(QStringLiteral("dDesense"), QString::fromLatin1(ReportFlow::kDDesenseAssetFile));

    QJsonArray outputFormats;
    outputFormats.append(QString::fromLatin1(ReportFlow::kMarkdownFormat));
    outputFormats.append(QString::fromLatin1(ReportFlow::kHtmlFormat));

    QJsonObject request;
    request.insert(QStringLiteral("reportBundleVersion"), QString::fromLatin1(ReportFlow::kBundleVersion));
    request.insert(QStringLiteral("taskId"), taskId);
    request.insert(QStringLiteral("mode"), QString::fromLatin1(ReportFlow::kTemplateOnlyMode));
    request.insert(QStringLiteral("language"), QString::fromLatin1(ReportFlow::kDefaultLanguage));
    request.insert(QStringLiteral("templateId"), QString::fromLatin1(ReportFlow::kDefaultTemplateId));
    request.insert(QStringLiteral("outputFormats"), outputFormats);
    request.insert(QStringLiteral("inputFiles"), inputFiles);
    request.insert(QStringLiteral("assetFiles"), assetFiles);
    return request;
}

QJsonObject buildStatusObject(
    const QString& taskId,
    ReportFlow::JobState state,
    ReportFlow::JobStage stage,
    const QJsonArray& errors = QJsonArray()) {
    QJsonObject object;
    object.insert(QStringLiteral("taskId"), taskId);
    object.insert(QStringLiteral("state"), QString::fromLatin1(ReportFlow::toString(state)));
    object.insert(QStringLiteral("stage"), QString::fromLatin1(ReportFlow::toString(stage)));
    object.insert(QStringLiteral("updatedAtUtcMs"), QDateTime::currentMSecsSinceEpoch());
    object.insert(QStringLiteral("errors"), errors);
    return object;
}

bool writeJsonFile(const QString& filePath, const QJsonObject& object, QString* errorMessage) {
    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("无法写入文件：%1").arg(filePath);
        }
        return false;
    }

    const QByteArray content = QJsonDocument(object).toJson(QJsonDocument::Indented);
    if (file.write(content) != content.size()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("写入文件失败：%1").arg(filePath);
        }
        return false;
    }

    if (!file.commit()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("提交文件失败：%1").arg(filePath);
        }
        return false;
    }

    return true;
}

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

bool ensureDirectory(const QString& path, QString* errorMessage) {
    QDir dir;
    if (dir.mkpath(path)) {
        return true;
    }
    if (errorMessage) {
        *errorMessage = QStringLiteral("无法创建目录：%1").arg(path);
    }
    return false;
}

}  // namespace

ReportJobExportResult ReportJobExporter::exportBundle(
    const SimulationTaskResult& result,
    const QString& workRootDir) {
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

    if (!ensureDirectory(workRootDir, &exportResult.errorMessage)) {
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

    if (!ensureDirectory(exportResult.assetsDirectory, &exportResult.errorMessage) ||
        !ensureDirectory(exportResult.outputsDirectory, &exportResult.errorMessage) ||
        !ensureDirectory(exportResult.logsDirectory, &exportResult.errorMessage)) {
        return exportResult;
    }

    exportResult.requestFilePath = QDir(exportResult.jobDirectory).filePath(QString::fromLatin1(ReportFlow::kRequestFileName));
    exportResult.simulationResultFilePath = QDir(exportResult.jobDirectory).filePath(QString::fromLatin1(ReportFlow::kSimulationResultFileName));
    exportResult.reportContextFilePath = QDir(exportResult.jobDirectory).filePath(QString::fromLatin1(ReportFlow::kReportContextFileName));
    exportResult.statusFilePath = QDir(exportResult.jobDirectory).filePath(QString::fromLatin1(ReportFlow::kStatusFileName));

    if (!writeJsonFile(exportResult.simulationResultFilePath, serializeSimulationTaskResult(result), &exportResult.errorMessage)) {
        return exportResult;
    }

    if (!writeJsonFile(exportResult.reportContextFilePath, ReportContextBuilder::build(result), &exportResult.errorMessage)) {
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

    if (!writeJsonFile(exportResult.requestFilePath, buildRequestObject(result.taskId), &exportResult.errorMessage)) {
        return exportResult;
    }

    if (!writeJsonFile(
            exportResult.statusFilePath,
            buildStatusObject(
                result.taskId,
                ReportFlow::JobState::Pending,
                ReportFlow::JobStage::ValidateBundle),
            &exportResult.errorMessage)) {
        return exportResult;
    }

    exportResult.success = true;
    return exportResult;
}
