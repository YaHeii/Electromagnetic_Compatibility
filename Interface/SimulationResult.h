#pragma once

#include <optional>
#include <utility>
#include <vector>

#include <QtGlobal>
#include <QString>

#include "Interface/DataModel.h"

enum class ModelType {
    PE,
    RayModel
};

enum class FormationSource {
    ManualInput,
    PresetFormation
};

enum class SimulationResultStatus {
    Succeeded,
    Failed,
    Cancelled
};

enum class EmitterResultStatus {
    Succeeded,
    Failed,
    Cancelled,
    Skipped
};

enum class ScalarFieldQuantity {
    AggregatedPowerDbm,
    PathLossDb,
    NoiseElevationDb,
    DesenseDb
};

inline QString expectedValueUnit(ScalarFieldQuantity quantity) {
    switch (quantity) {
    case ScalarFieldQuantity::AggregatedPowerDbm:
        return QStringLiteral("dBm");
    case ScalarFieldQuantity::PathLossDb:
    case ScalarFieldQuantity::NoiseElevationDb:
    case ScalarFieldQuantity::DesenseDb:
        return QStringLiteral("dB");
    }
    return QString();
}

struct ScalarField2D {
    QString fieldId;
    QString displayName;
    ScalarFieldQuantity quantity{ScalarFieldQuantity::AggregatedPowerDbm};
    QString valueUnit;
    QString axisXUnit;
    QString axisYUnit;
    int rows{0};
    int cols{0};
    double originX{0.0};
    double originY{0.0};
    double stepX{0.0};
    double stepY{0.0};
    std::optional<double> noDataValue;
    std::vector<double> values;

    std::pair<bool, QString> validate() const {
        if (fieldId.trimmed().isEmpty()) {
            return {false, QStringLiteral("fieldId 不能为空")};
        }
        if (displayName.trimmed().isEmpty()) {
            return {false, QStringLiteral("displayName 不能为空")};
        }
        if (rows <= 0 || cols <= 0) {
            return {false, QStringLiteral("二维标量场尺寸必须为正整数")};
        }
        if (stepX <= 0.0 || stepY <= 0.0) {
            return {false, QStringLiteral("二维标量场步长必须大于 0")};
        }
        if (axisXUnit.trimmed().isEmpty() || axisYUnit.trimmed().isEmpty()) {
            return {false, QStringLiteral("二维标量场坐标单位不能为空")};
        }
        if (values.size() != static_cast<std::size_t>(rows * cols)) {
            return {false, QStringLiteral("二维标量场 values 长度必须等于 rows * cols")};
        }
        if (valueUnit != expectedValueUnit(quantity)) {
            return {false, QStringLiteral("二维标量场数值单位与 quantity 不匹配")};
        }
        return {true, QString()};
    }
};

struct Series1D {
    QString seriesId;
    QString displayName;
    QString xUnit;
    QString yUnit;
    std::vector<double> xValues;
    std::vector<double> yValues;

    std::pair<bool, QString> validate() const {
        if (seriesId.trimmed().isEmpty()) {
            return {false, QStringLiteral("seriesId 不能为空")};
        }
        if (displayName.trimmed().isEmpty()) {
            return {false, QStringLiteral("displayName 不能为空")};
        }
        if (xUnit.trimmed().isEmpty() || yUnit.trimmed().isEmpty()) {
            return {false, QStringLiteral("Series1D 的坐标单位不能为空")};
        }
        if (xValues.empty() || yValues.empty()) {
            return {false, QStringLiteral("Series1D 的数据不能为空")};
        }
        if (xValues.size() != yValues.size()) {
            return {false, QStringLiteral("Series1D 的 xValues 和 yValues 长度必须一致")};
        }
        return {true, QString()};
    }
};

struct LabeledMatrix2D {
    QString matrixId;
    QString displayName;
    QString valueUnit;
    int rows{0};
    int cols{0};
    std::vector<QString> rowLabels;
    std::vector<QString> colLabels;
    std::vector<double> values;

    std::pair<bool, QString> validate() const {
        if (matrixId.trimmed().isEmpty()) {
            return {false, QStringLiteral("matrixId 不能为空")};
        }
        if (displayName.trimmed().isEmpty()) {
            return {false, QStringLiteral("displayName 不能为空")};
        }
        if (valueUnit.trimmed().isEmpty()) {
            return {false, QStringLiteral("矩阵的 valueUnit 不能为空")};
        }
        if (rows <= 0 || cols <= 0) {
            return {false, QStringLiteral("矩阵的行列数必须为正整数")};
        }
        if (rowLabels.size() != static_cast<std::size_t>(rows) ||
            colLabels.size() != static_cast<std::size_t>(cols)) {
                return {false, QStringLiteral("矩阵标签数量必须与行列数一致")};
        }
        if (values.size() != static_cast<std::size_t>(rows * cols)) {
            return {false, QStringLiteral("矩阵 values 长度必须等于 rows * cols")};
        }
        return {true, QString()};
    }
};

struct EmitterResult {
    QString emitterId;
    QString shipId;
    EmitterResultStatus status{EmitterResultStatus::Succeeded};
    double centerFrequencyGHz{0.0};
    double transmitPowerDbm{0.0};
    double worldX{0.0};
    double worldY{0.0};
    double worldZ{0.0};
    QString errorMessage;
    ScalarField2D field2D;

    std::pair<bool, QString> validate() const {
        if (emitterId.trimmed().isEmpty()) {
            return {false, QStringLiteral("emitterId 不能为空")};
        }
        if (shipId.trimmed().isEmpty()) {
            return {false, QStringLiteral("shipId 不能为空")};
        }
        if (status == EmitterResultStatus::Succeeded) {
            const auto fieldValidation = field2D.validate();
            if (!fieldValidation.first) {
                return fieldValidation;
            }
            if (field2D.quantity != ScalarFieldQuantity::PathLossDb) {
                return {false, QStringLiteral("成功态 emitterResult.field2D 的 quantity 必须为 PathLossDb")};
            }
            return {true, QString()};
        }
        if (errorMessage.trimmed().isEmpty()) {
            return {false, QStringLiteral("非成功发射机结果必须包含 errorMessage")};
        }
        return {true, QString()};
    }
};

struct SCFMetric {
    double scalarDb{0.0};
    double thermalNoiseFloorDbm{0.0};
    int linkCount{0};
    LabeledMatrix2D couplingMatrix;

    std::pair<bool, QString> validate() const {
        if (linkCount <= 0) {
            return {false, QStringLiteral("SCF 的 linkCount 必须大于 0")};
        }
        const auto matrixValidation = couplingMatrix.validate();
        if (!matrixValidation.first) {
            return matrixValidation;
        }
        if (linkCount > couplingMatrix.rows * couplingMatrix.cols) {
            return {false, QStringLiteral("SCF 的 linkCount 超出耦合矩阵容量")};
        }
        return {true, QString()};
    }
};

struct S3IMetric {
    double scalarDb{0.0};
    QString referenceTransmitterId;
    QString referenceReceiverId;
    double baselineWindSpeedMps{0.0};
    double currentWindSpeedMps{0.0};
    Series1D calmCurve;
    Series1D currentCurve;

    std::pair<bool, QString> validate() const {
        if (referenceTransmitterId.trimmed().isEmpty()) {
            return {false, QStringLiteral("S3I referenceTransmitterId 不能为空")};
        }
        if (referenceReceiverId.trimmed().isEmpty()) {
            return {false, QStringLiteral("S3I referenceReceiverId 不能为空")};
        }
        if (baselineWindSpeedMps < 0.0 || currentWindSpeedMps < 0.0) {
            return {false, QStringLiteral("S3I 的风速必须大于等于 0")};
        }
        const auto calmValidation = calmCurve.validate();
        if (!calmValidation.first) {
            return calmValidation;
        }
        const auto currentValidation = currentCurve.validate();
        if (!currentValidation.first) {
            return currentValidation;
        }
        if (calmCurve.xValues.size() != currentCurve.xValues.size()) {
            return {false, QStringLiteral("S3I 两条曲线的采样点数量必须一致")};
        }
        return {true, QString()};
    }
};

struct TElevMetric {
    ScalarField2D field;
    double maxDb{0.0};
    double meanDb{0.0};

    std::pair<bool, QString> validate() const {
        const auto fieldValidation = field.validate();
        if (!fieldValidation.first) {
            return fieldValidation;
        }
        if (field.quantity != ScalarFieldQuantity::NoiseElevationDb) {
            return {false, QStringLiteral("T_elev 的 field.quantity 必须为 NoiseElevationDb")};
        }
        return {true, QString()};
    }
};

struct DDesenseMetric {
    ScalarField2D field;
    QString victimReceiverId;
    double peakDb{0.0};
    double coveragePercent{0.0};
    double adiDbPerSquareMeter{0.0};

    std::pair<bool, QString> validate() const {
        const auto fieldValidation = field.validate();
        if (!fieldValidation.first) {
            return fieldValidation;
        }
        if (field.quantity != ScalarFieldQuantity::DesenseDb) {
            return {false, QStringLiteral("D_desense 的 field.quantity 必须为 DesenseDb")};
        }
        if (victimReceiverId.trimmed().isEmpty()) {
            return {false, QStringLiteral("victimReceiverId 不能为空")};
        }
        if (coveragePercent < 0.0 || coveragePercent > 100.0) {
            return {false, QStringLiteral("coveragePercent 必须位于 [0, 100]")};
        }
        return {true, QString()};
    }
};

struct DerivedMetrics {
    bool available{false};
    SCFMetric scf;
    S3IMetric s3i;
    TElevMetric tElev;
    DDesenseMetric dDesense;

    std::pair<bool, QString> validate() const {
        if (!available) {
            return {true, QString()};
        }

        const auto scfValidation = scf.validate();
        if (!scfValidation.first) {
            return scfValidation;
        }
        const auto s3iValidation = s3i.validate();
        if (!s3iValidation.first) {
            return s3iValidation;
        }
        const auto tElevValidation = tElev.validate();
        if (!tElevValidation.first) {
            return tElevValidation;
        }
        return dDesense.validate();
    }
};

struct SimulationTaskResult {
    QString resultSchemaVersion;
    QString taskId;
    ModelType modelType{ModelType::PE};
    SimulationResultStatus status{SimulationResultStatus::Succeeded};
    FormationSource formationSource{FormationSource::ManualInput};
    std::optional<int> presetFormationId;
    qint64 startedAtUtcMs{0};
    qint64 finishedAtUtcMs{0};
    qint64 durationMs{0};
    QString errorMessage;
    QString summaryText;
    DataModel::DataSnapshot inputSnapshot;
    ScalarField2D aggregatedField;
    std::vector<EmitterResult> emitterResults;
    DerivedMetrics derivedMetrics;

    std::pair<bool, QString> validate() const {
        if (resultSchemaVersion.trimmed().isEmpty()) {
            return {false, QStringLiteral("resultSchemaVersion 不能为空")};
        }
        if (taskId.trimmed().isEmpty()) {
            return {false, QStringLiteral("taskId 不能为空")};
        }
        if (startedAtUtcMs > finishedAtUtcMs) {
            return {false, QStringLiteral("startedAtUtcMs 不能晚于 finishedAtUtcMs")};
        }
        if (durationMs < 0) {
            return {false, QStringLiteral("durationMs 不能为负数")};
        }
        if (durationMs != finishedAtUtcMs - startedAtUtcMs) {
            return {false, QStringLiteral("durationMs 必须等于结束时间减开始时间")};
        }

        const auto snapshotValidation = DataModel::validateSnapshot(inputSnapshot);
        if (!snapshotValidation.first) {
            return {false, QStringLiteral("inputSnapshot 非法: %1").arg(snapshotValidation.second)};
        }

        if (formationSource == FormationSource::ManualInput && presetFormationId.has_value()) {
            return {false, QStringLiteral("ManualInput 不应携带 presetFormationId")};
        }
        if (formationSource == FormationSource::PresetFormation && !presetFormationId.has_value()) {
            return {false, QStringLiteral("PresetFormation 必须携带 presetFormationId")};
        }

        const auto metricsValidation = derivedMetrics.validate();
        if (!metricsValidation.first) {
            return metricsValidation;
        }

        switch (status) {
        case SimulationResultStatus::Succeeded: {
            if (!derivedMetrics.available) {
                return {false, QStringLiteral("成功结果必须包含 available=true 的 DerivedMetrics")};
            }

            const auto aggregatedValidation = aggregatedField.validate();
            if (!aggregatedValidation.first) {
                return aggregatedValidation;
            }
            if (aggregatedField.quantity != ScalarFieldQuantity::AggregatedPowerDbm) {
                return {false, QStringLiteral("aggregatedField 的 quantity 必须为 AggregatedPowerDbm")};
            }
            if (emitterResults.empty()) {
                return {false, QStringLiteral("成功结果必须包含至少一个发射机结果")};
            }
            for (const auto& emitterResult : emitterResults) {
                const auto emitterValidation = emitterResult.validate();
                if (!emitterValidation.first) {
                    return emitterValidation;
                }
            }
            break;
        }
        case SimulationResultStatus::Failed:
            if (errorMessage.trimmed().isEmpty()) {
                return {false, QStringLiteral("失败结果必须包含 errorMessage")};
            }
            break;
        case SimulationResultStatus::Cancelled:
            if (derivedMetrics.available) {
                return {false, QStringLiteral("已取消结果不能暴露 available=true 的 DerivedMetrics")};
            }
            break;
        }

        return {true, QString()};
    }
};
