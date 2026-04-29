#pragma once

#include <optional>
#include <utility>
#include <vector>

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
    PathLossDb
};

inline QString expectedValueUnit(ScalarFieldQuantity quantity) {
    switch (quantity) {
    case ScalarFieldQuantity::AggregatedPowerDbm:
        return QStringLiteral("dBm");
    case ScalarFieldQuantity::PathLossDb:
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
            return field2D.validate();
        }
        if (errorMessage.trimmed().isEmpty()) {
            return {false, QStringLiteral("非成功发射机结果必须包含 errorMessage")};
        }
        return {true, QString()};
    }
};

struct DerivedMetrics {
    bool available{false};
    QString metricsSchemaVersion;
    QString note;

    std::pair<bool, QString> validate() const {
        if (!available) {
            return {true, QString()};
        }
        if (metricsSchemaVersion.trimmed().isEmpty()) {
            return {false, QStringLiteral("available=true 时 metricsSchemaVersion 不能为空")};
        }
        return {true, QString()};
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
            const auto aggregatedValidation = aggregatedField.validate();
            if (!aggregatedValidation.first) {
                return aggregatedValidation;
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
            break;
        }

        return {true, QString()};
    }
};
