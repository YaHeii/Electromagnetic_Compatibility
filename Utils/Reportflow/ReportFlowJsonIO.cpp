#include "Utils/Reportflow/ReportFlowJsonIO.h"

#include <QDateTime>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>

namespace {

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

QJsonArray toStringListArray(const QStringList& values) {
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

}  // namespace

QJsonObject ReportFlowJsonIO::serializeSimulationTaskResult(const SimulationTaskResult& result) {
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

QJsonObject ReportFlowJsonIO::buildRequestObject(const ReportFlow::Request& request) {
    QJsonObject inputFiles;
    inputFiles.insert(
        QString::fromLatin1(ReportFlow::Keys::SimulationResult),
        request.inputFiles.simulationResultFileName);
    inputFiles.insert(
        QString::fromLatin1(ReportFlow::Keys::ReportContext),
        request.inputFiles.reportContextFileName);
    inputFiles.insert(
        QString::fromLatin1(ReportFlow::Keys::BaselineInput),
        request.inputFiles.baselineInputFileName);

    QJsonObject requestObject;
    requestObject.insert(
        QString::fromLatin1(ReportFlow::Keys::ReportBundleVersion),
        request.reportBundleVersion);
    requestObject.insert(QString::fromLatin1(ReportFlow::Keys::TaskId), request.taskId);
    requestObject.insert(
        QString::fromLatin1(ReportFlow::Keys::Mode),
        QString::fromLatin1(ReportFlow::toString(request.mode)));
    requestObject.insert(
        QString::fromLatin1(ReportFlow::Keys::Language),
        request.language);
    requestObject.insert(
        QString::fromLatin1(ReportFlow::Keys::TemplateId),
        request.templateId);
    requestObject.insert(
        QString::fromLatin1(ReportFlow::Keys::OutputFormats),
        toStringListArray(request.outputFormats));
    requestObject.insert(
        QString::fromLatin1(ReportFlow::Keys::InputFiles),
        inputFiles);

    if (request.assetFiles.has_value()) {
        QJsonObject assetFiles;
        const auto& assets = request.assetFiles.value();
        assetFiles.insert(QString::fromLatin1(ReportFlow::Keys::AggregatedField), assets.aggregatedFieldFile);
        assetFiles.insert(QString::fromLatin1(ReportFlow::Keys::ReferenceEmitter), assets.referenceEmitterFile);
        assetFiles.insert(QString::fromLatin1(ReportFlow::Keys::Scf), assets.scfFile);
        assetFiles.insert(QString::fromLatin1(ReportFlow::Keys::S3i), assets.s3iFile);
        assetFiles.insert(QString::fromLatin1(ReportFlow::Keys::TElev), assets.tElevFile);
        assetFiles.insert(QString::fromLatin1(ReportFlow::Keys::DDesense), assets.dDesenseFile);
        requestObject.insert(QString::fromLatin1(ReportFlow::Keys::AssetFiles), assetFiles);
    }

    if (request.agent.has_value()) {
        const auto& agent = request.agent.value();
        QJsonObject agentObject;
        agentObject.insert(QString::fromLatin1(ReportFlow::Keys::GoalMode), agent.goalMode);
        agentObject.insert(QString::fromLatin1(ReportFlow::Keys::MaxExperimentCount), agent.maxExperimentCount);
        agentObject.insert(QString::fromLatin1(ReportFlow::Keys::MutationScopes), toStringListArray(agent.mutationScopes));
        agentObject.insert(QString::fromLatin1(ReportFlow::Keys::RankingPolicy), agent.rankingPolicy);
        agentObject.insert(QString::fromLatin1(ReportFlow::Keys::ProviderProfile), agent.providerProfile);
        requestObject.insert(QString::fromLatin1(ReportFlow::Keys::Agent), agentObject);
    }

    return requestObject;
}

QJsonObject ReportFlowJsonIO::buildStatusObject(const ReportFlow::Status& status) {
    QJsonObject object;
    object.insert(QString::fromLatin1(ReportFlow::Keys::TaskId), status.taskId);
    object.insert(
        QString::fromLatin1(ReportFlow::Keys::State),
        QString::fromLatin1(ReportFlow::toString(status.state)));
    object.insert(
        QString::fromLatin1(ReportFlow::Keys::Stage),
        QString::fromLatin1(ReportFlow::toString(status.stage)));
    object.insert(
        QString::fromLatin1(ReportFlow::Keys::UpdatedAtUtcMs),
        status.updatedAtUtcMs > 0 ? status.updatedAtUtcMs : QDateTime::currentMSecsSinceEpoch());

    if (status.startedAtUtcMs.has_value()) {
        object.insert(QString::fromLatin1(ReportFlow::Keys::StartedAtUtcMs), status.startedAtUtcMs.value());
    }
    if (status.finishedAtUtcMs.has_value()) {
        object.insert(QString::fromLatin1(ReportFlow::Keys::FinishedAtUtcMs), status.finishedAtUtcMs.value());
    }
    object.insert(QString::fromLatin1(ReportFlow::Keys::ErrorMessage), status.errorMessage);
    object.insert(QString::fromLatin1(ReportFlow::Keys::Errors), toStringListArray(status.errors));
    return object;
}

bool ReportFlowJsonIO::writeJsonFile(
    const QString& filePath,
    const QJsonObject& object,
    QString* errorMessage) {
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

bool ReportFlowJsonIO::ensureDirectory(
    const QString& path,
    QString* errorMessage) {
    QDir dir;
    if (dir.mkpath(path)) {
        return true;
    }
    if (errorMessage) {
        *errorMessage = QStringLiteral("无法创建目录：%1").arg(path);
    }
    return false;
}
