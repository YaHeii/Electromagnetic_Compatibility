#include <catch2/catch_test_macros.hpp>

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>

#include "Interface/ReportFlowContract.h"
#include "Interface/SimulationResult.h"
#include "Utils/Reportflow/ReportContextBuilder.h"
#include "Utils/Reportflow/ReportJobExporter.h"

namespace {

QApplication& ensureApplication() {
    if (auto* existing = qobject_cast<QApplication*>(QCoreApplication::instance())) {
        return *existing;
    }

    qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));

    static int argc = 1;
    static char appName[] = "ReportflowTests";
    static char* argv[] = {appName, nullptr};
    static QApplication app(argc, argv);
    return app;
}

EnvironmentData makeEnvironment() {
    EnvironmentData environment;
    environment.maxRange = 40.0;
    environment.ductHeight = 10.0;
    environment.windSpeed = 3.0;
    environment.dx = 1.0;
    environment.dz = 0.5;
    environment.nz = 64;
    environment.angleStepDeg = 30;
    return environment;
}

EquipmentData makeTransmitter(const QString& equipmentId) {
    EquipmentData equipment;
    equipment.equipmentId = equipmentId;
    equipment.equipmentType = DataModelSchemaValues::transmitterType();
    equipment.gainDbi = 12.0;
    equipment.offsetX = 0.0;
    equipment.offsetY = 0.0;
    equipment.offsetZ = 2.0;
    equipment.transmitterCenterFrequencyGHz = 1.0;
    equipment.transmitterBandwidthMHz = 20.0;
    equipment.transmitterPowerDbm = 30.0;
    equipment.transmitterAntennaPhiDeg = 0.0;
    equipment.transmitterBeamWidthDeg = 30.0;
    equipment.transmitterPolarization = DataModelSchemaValues::verticalPolarization();
    equipment.transmitterAntennaType = QString::fromLatin1(SchemaValues::Horn);
    return equipment;
}

EquipmentData makeReceiver() {
    EquipmentData equipment;
    equipment.equipmentId = "RX-1";
    equipment.equipmentType = DataModelSchemaValues::receiverType();
    equipment.gainDbi = 6.0;
    equipment.offsetX = 0.0;
    equipment.offsetY = 0.0;
    equipment.offsetZ = 2.0;
    equipment.receiverCenterFrequencyGHz = 1.0;
    equipment.receiverBandwidthMHz = 20.0;
    equipment.receiverSensitivityDbm = -85.0;
    equipment.receiverInterferenceMarginDb = -10.0;
    equipment.receiverSinrMarginDb = 3.0;
    equipment.receiverNoiseFigureDb = 3.0;
    return equipment;
}

ShipData makeShip(
    const std::string& shipId,
    double worldX,
    const QString& equipmentId) {
    ShipData ship;
    ship.shipId = shipId;
    ship.worldX = worldX;
    ship.worldY = 0.0;
    ship.worldZ = 0.0;
    ship.shipOrientationDeg = 0.0;
    ship.shipSpeedMps = 4.0;
    ship.equipmentRefs.push_back(EquipmentOnShip{equipmentId, true});
    return ship;
}

DataModel::DataSnapshot makeSnapshot(const QString& referenceTransmitterId) {
    DataModel::DataSnapshot snapshot;
    snapshot.environmentConfig = makeEnvironment();
    snapshot.emcAnalysisConfig.fieldPlaneHeightM = 8.0;
    snapshot.emcAnalysisConfig.referenceTransmitterId = referenceTransmitterId;
    snapshot.emcAnalysisConfig.referenceReceiverId = "RX-1";
    snapshot.emcAnalysisConfig.s3iBaselineWindSpeedMps = 0.5;
    snapshot.allEquipments = {
        makeTransmitter("TX-1"),
        makeTransmitter("TX-2"),
        makeReceiver(),
    };
    snapshot.allShips = {
        makeShip("USV-TX-1", 0.0, "TX-1"),
        makeShip("USV-TX-2", 8.0, "TX-2"),
        makeShip("USV-RX", 16.0, "RX-1"),
    };
    return snapshot;
}

ScalarField2D makeField(
    ScalarFieldQuantity quantity,
    const QString& fieldId,
    const QString& displayName) {
    ScalarField2D field;
    field.fieldId = fieldId;
    field.displayName = displayName;
    field.quantity = quantity;
    field.valueUnit = expectedValueUnit(quantity);
    field.axisXUnit = "m";
    field.axisYUnit = "m";
    field.rows = 2;
    field.cols = 2;
    field.originX = 0.0;
    field.originY = 0.0;
    field.stepX = 1.0;
    field.stepY = 1.0;
    field.values = {-42.0, -41.0, -40.0, -39.0};
    return field;
}

EmitterResult makeEmitterResult(const QString& emitterId, const QString& shipId) {
    EmitterResult emitter;
    emitter.emitterId = emitterId;
    emitter.shipId = shipId;
    emitter.status = EmitterResultStatus::Succeeded;
    emitter.centerFrequencyGHz = 1.0;
    emitter.transmitPowerDbm = 30.0;
    emitter.worldX = 0.0;
    emitter.worldY = 0.0;
    emitter.worldZ = 2.0;
    emitter.field2D = makeField(
        ScalarFieldQuantity::PathLossDb,
        QStringLiteral("path-loss-%1").arg(emitterId),
        QStringLiteral("路径损耗场"));
    return emitter;
}

Series1D makeSeries(const QString& id, const QString& name) {
    Series1D series;
    series.seriesId = id;
    series.displayName = name;
    series.xUnit = "m";
    series.yUnit = "dB";
    series.xValues = {1.0, 2.0, 3.0};
    series.yValues = {-70.0, -72.0, -73.5};
    return series;
}

LabeledMatrix2D makeMatrix() {
    LabeledMatrix2D matrix;
    matrix.matrixId = "scf-coupling";
    matrix.displayName = QStringLiteral("SCF 耦合矩阵");
    matrix.valueUnit = "dB";
    matrix.rows = 1;
    matrix.cols = 2;
    matrix.rowLabels = {QStringLiteral("RX-1")};
    matrix.colLabels = {QStringLiteral("TX-1"), QStringLiteral("TX-2")};
    matrix.values = {12.0, 7.0};
    return matrix;
}

DerivedMetrics makeDerivedMetrics() {
    DerivedMetrics metrics;
    metrics.available = true;
    metrics.scf.scalarDb = 12.0;
    metrics.scf.thermalNoiseFloorDbm = -98.0;
    metrics.scf.linkCount = 2;
    metrics.scf.couplingMatrix = makeMatrix();

    metrics.s3i.scalarDb = 0.8;
    metrics.s3i.referenceTransmitterId = "TX-1";
    metrics.s3i.referenceReceiverId = "RX-1";
    metrics.s3i.baselineWindSpeedMps = 0.5;
    metrics.s3i.currentWindSpeedMps = 3.0;
    metrics.s3i.calmCurve = makeSeries("s3i-calm", QStringLiteral("平静海况曲线"));
    metrics.s3i.currentCurve = makeSeries("s3i-current", QStringLiteral("当前海况曲线"));

    metrics.tElev.field = makeField(
        ScalarFieldQuantity::NoiseElevationDb,
        "t-elev",
        QStringLiteral("噪声抬升场"));
    metrics.tElev.maxDb = -39.0;
    metrics.tElev.meanDb = -40.5;

    metrics.dDesense.field = makeField(
        ScalarFieldQuantity::DesenseDb,
        "d-desense",
        QStringLiteral("灵敏度恶化场"));
    metrics.dDesense.victimReceiverId = "RX-1";
    metrics.dDesense.peakDb = 18.0;
    metrics.dDesense.coveragePercent = 75.0;
    metrics.dDesense.adiDbPerSquareMeter = 6.0;
    return metrics;
}

SimulationTaskResult makeSuccessfulResultForReportflow() {
    SimulationTaskResult result;
    result.resultSchemaVersion = "1.0.0";
    result.taskId = "task-reportflow-001";
    result.modelType = ModelType::PE;
    result.status = SimulationResultStatus::Succeeded;
    result.formationSource = FormationSource::ManualInput;
    result.startedAtUtcMs = 100;
    result.finishedAtUtcMs = 300;
    result.durationMs = 200;
    result.summaryText = QStringLiteral("仿真完成");
    result.inputSnapshot = makeSnapshot("TX-1");
    result.aggregatedField = makeField(
        ScalarFieldQuantity::AggregatedPowerDbm,
        "aggregated-power",
        QStringLiteral("总功率场"));
    result.emitterResults.push_back(makeEmitterResult("TX-1", "USV-TX-1"));
    result.emitterResults.push_back(makeEmitterResult("TX-2", "USV-TX-2"));
    result.derivedMetrics = makeDerivedMetrics();
    return result;
}

QJsonObject readJsonObject(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    return QJsonDocument::fromJson(file.readAll()).object();
}

}  // namespace

TEST_CASE("ReportFlowContract exposes stable bundle file names", "[reportflow][contract]") {
    REQUIRE(QString::fromLatin1(ReportFlow::kRequestFileName) == QStringLiteral("request.json"));
    REQUIRE(QString::fromLatin1(ReportFlow::kSimulationResultFileName) == QStringLiteral("simulation-result.json"));
    REQUIRE(QString::fromLatin1(ReportFlow::kReportContextFileName) == QStringLiteral("report-context.json"));
    REQUIRE(QString::fromLatin1(ReportFlow::kStatusFileName) == QStringLiteral("status.json"));
}

TEST_CASE("ReportContextBuilder exposes stable report summary fields", "[reportflow][context]") {
    const SimulationTaskResult result = makeSuccessfulResultForReportflow();
    const QJsonObject context = ReportContextBuilder::build(result);

    REQUIRE(context.value(QStringLiteral("reportContextVersion")).toString() == QStringLiteral("1.0.0"));
    REQUIRE(context.value(QStringLiteral("summaryText")).toString() == QStringLiteral("仿真完成"));

    const QJsonObject analysisConfig = context.value(QStringLiteral("analysisConfig")).toObject();
    REQUIRE(analysisConfig.value(QStringLiteral("referenceTransmitterId")).toString() == QStringLiteral("TX-1"));
    REQUIRE(analysisConfig.value(QStringLiteral("referenceReceiverId")).toString() == QStringLiteral("RX-1"));

    const QJsonArray charts = context.value(QStringLiteral("charts")).toArray();
    REQUIRE(charts.size() == 6);
    REQUIRE(charts[0].toObject().value(QStringLiteral("chartId")).toString() == QStringLiteral("aggregatedField"));
    REQUIRE(charts[0].toObject().value(QStringLiteral("assetFile")).toString() == QStringLiteral("assets/aggregated-field.png"));

    const QJsonObject metricsSummary = context.value(QStringLiteral("metricsSummary")).toObject();
    REQUIRE(metricsSummary.value(QStringLiteral("available")).toBool());
    REQUIRE(metricsSummary.value(QStringLiteral("scf")).toObject().value(QStringLiteral("linkCount")).toInt() == 2);
    REQUIRE(metricsSummary.value(QStringLiteral("dDesense")).toObject().value(QStringLiteral("victimReceiverId")).toString() == QStringLiteral("RX-1"));
}

TEST_CASE("ReportJobExporter writes a complete template-only bundle", "[reportflow][export]") {
    ensureApplication();
    const SimulationTaskResult result = makeSuccessfulResultForReportflow();
    QTemporaryDir tempDir;
    REQUIRE(tempDir.isValid());

    const ReportJobExportResult exportResult = ReportJobExporter::exportBundle(result, tempDir.path());
    INFO(exportResult.errorMessage.toStdString());
    REQUIRE(exportResult.success);

    REQUIRE(QFileInfo::exists(exportResult.jobDirectory));
    REQUIRE(QFileInfo::exists(exportResult.requestFilePath));
    REQUIRE(QFileInfo::exists(exportResult.simulationResultFilePath));
    REQUIRE(QFileInfo::exists(exportResult.reportContextFilePath));
    REQUIRE(QFileInfo::exists(exportResult.statusFilePath));
    REQUIRE(QFileInfo::exists(QDir(exportResult.jobDirectory).filePath(QString::fromLatin1(ReportFlow::kAggregatedFieldAssetFile))));
    REQUIRE(QFileInfo::exists(QDir(exportResult.jobDirectory).filePath(QString::fromLatin1(ReportFlow::kReferenceEmitterAssetFile))));
    REQUIRE(QFileInfo::exists(QDir(exportResult.jobDirectory).filePath(QString::fromLatin1(ReportFlow::kScfAssetFile))));
    REQUIRE(QFileInfo::exists(QDir(exportResult.jobDirectory).filePath(QString::fromLatin1(ReportFlow::kS3iAssetFile))));
    REQUIRE(QFileInfo::exists(QDir(exportResult.jobDirectory).filePath(QString::fromLatin1(ReportFlow::kTElevAssetFile))));
    REQUIRE(QFileInfo::exists(QDir(exportResult.jobDirectory).filePath(QString::fromLatin1(ReportFlow::kDDesenseAssetFile))));

    const QJsonObject requestObject = readJsonObject(exportResult.requestFilePath);
    REQUIRE(requestObject.value(QStringLiteral("mode")).toString() == QStringLiteral("template-only"));
    REQUIRE(requestObject.value(QStringLiteral("language")).toString() == QStringLiteral("zh-CN"));
    REQUIRE(requestObject.value(QStringLiteral("outputFormats")).toArray().size() == 2);

    const QJsonObject resultObject = readJsonObject(exportResult.simulationResultFilePath);
    REQUIRE(resultObject.value(QStringLiteral("taskId")).toString() == result.taskId);
    REQUIRE(resultObject.value(QStringLiteral("derivedMetrics")).toObject().value(QStringLiteral("available")).toBool());

    const QJsonObject statusObject = readJsonObject(exportResult.statusFilePath);
    REQUIRE(statusObject.value(QStringLiteral("state")).toString() == QStringLiteral("pending"));
    REQUIRE(statusObject.value(QStringLiteral("stage")).toString() == QStringLiteral("validate_bundle"));
}

TEST_CASE("ReportJobExporter rejects non-succeeded result", "[reportflow][export]") {
    SimulationTaskResult result = makeSuccessfulResultForReportflow();
    result.status = SimulationResultStatus::Failed;
    result.errorMessage = QStringLiteral("mock failure");

    QTemporaryDir tempDir;
    REQUIRE(tempDir.isValid());

    const ReportJobExportResult exportResult = ReportJobExporter::exportBundle(result, tempDir.path());
    REQUIRE_FALSE(exportResult.success);
    REQUIRE_FALSE(exportResult.errorMessage.trimmed().isEmpty());
}
