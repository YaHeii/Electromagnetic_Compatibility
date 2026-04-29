#include "Simulation/EMCMetricsCalculator.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>

#include "Interface/TransferToEngin.h"
#include "Simulation/EMCComputationResult.h"
#include "Simulation/PEPropagationSolver.h"
#include "Utils/conversions.h"

namespace {

struct ResolvedEquipmentRef {
    const EquipmentData* equipment{nullptr};
    const ShipData* ship{nullptr};
    double worldX{0.0};
    double worldY{0.0};
    double worldZ{0.0};
};

bool isTransmitterLike(const EquipmentData& equipment) {
    return DataModelSchemaValues::supportsTransmitterRole(equipment.equipmentType);
}

bool isReceiverLike(const EquipmentData& equipment) {
    return DataModelSchemaValues::supportsReceiverRole(equipment.equipmentType);
}

std::optional<ResolvedEquipmentRef> resolveEnabledEquipment(
    const DataModel::DataSnapshot& snapshot,
    const QString& equipmentId) {
    for (const auto& ship : snapshot.allShips) {
        for (const auto& equipmentRef : ship.equipmentRefs) {
            if (equipmentRef.equipmentId != equipmentId || !equipmentRef.isEnabled) {
                continue;
            }

            for (const auto& equipment : snapshot.allEquipments) {
                if (equipment.equipmentId == equipmentId) {
                    return ResolvedEquipmentRef{
                        &equipment,
                        &ship,
                        ship.worldX + equipment.offsetX,
                        ship.worldY + equipment.offsetY,
                        ship.worldZ + equipment.offsetZ};
                }
            }
        }
    }

    return std::nullopt;
}

std::vector<ResolvedEquipmentRef> collectEnabledEquipments(
    const DataModel::DataSnapshot& snapshot,
    bool (*predicate)(const EquipmentData&)) {
    std::vector<ResolvedEquipmentRef> result;
    for (const auto& ship : snapshot.allShips) {
        for (const auto& equipmentRef : ship.equipmentRefs) {
            if (!equipmentRef.isEnabled) {
                continue;
            }

            for (const auto& equipment : snapshot.allEquipments) {
                if (equipment.equipmentId == equipmentRef.equipmentId && predicate(equipment)) {
                    result.push_back(
                        ResolvedEquipmentRef{
                            &equipment,
                            &ship,
                            ship.worldX + equipment.offsetX,
                            ship.worldY + equipment.offsetY,
                            ship.worldZ + equipment.offsetZ});
                    break;
                }
            }
        }
    }
    return result;
}

Transmitter_PE_data makeTransmitterPEData(const ResolvedEquipmentRef& transmitter) {
    Transmitter_PE_data peData;
    peData.shipName = transmitter.ship->shipId;
    peData.equipmenName = transmitter.equipment->equipmentId.toStdString();
    peData.X_offset = transmitter.worldX;
    peData.Y_offset = transmitter.worldY;
    peData.Z_offset = transmitter.worldZ;
    peData.power_dbm = transmitter.equipment->transmitterPowerDbm;
    peData.antenna_height = transmitter.worldZ;
    peData.beamWidth_deg = transmitter.equipment->transmitterBeamWidthDeg;
    peData.antennaPhi_deg = transmitter.equipment->transmitterAntennaPhiDeg;
    peData.centralF_Ghz = transmitter.equipment->transmitterCenterFrequencyGHz;
    return peData;
}

double computeNoiseFloorDbm(const EquipmentData& receiver) {
    return -173.97 + 10.0 * std::log10(receiver.receiverBandwidthMHz * 1.0e6) + receiver.receiverNoiseFigureDb;
}

ScalarField2D cloneMetricField(
    const ScalarField2D& source,
    const QString& fieldId,
    const QString& displayName,
    ScalarFieldQuantity quantity) {
    ScalarField2D field = source;
    field.fieldId = fieldId;
    field.displayName = displayName;
    field.quantity = quantity;
    field.valueUnit = expectedValueUnit(quantity);
    return field;
}

double meanOf(const std::vector<double>& values) {
    if (values.empty()) {
        return 0.0;
    }

    double total = 0.0;
    for (double value : values) {
        total += value;
    }
    return total / static_cast<double>(values.size());
}

double maxOf(const std::vector<double>& values) {
    if (values.empty()) {
        return 0.0;
    }
    return *std::max_element(values.begin(), values.end());
}

}  // namespace

EMCMetricsCalculator::ComputeResult EMCMetricsCalculator::compute(
    ModelType modelType,
    const DataModel::DataSnapshot& inputSnapshot,
    const EMCComputationResult& computationResult) {
    ComputeResult result;

    if (computationResult.status != SimulationResultStatus::Succeeded) {
        result.errorMessage = QStringLiteral("仅成功仿真结果可计算 DerivedMetrics");
        return result;
    }

    if (computationResult.emitterResults.empty()) {
        result.errorMessage = QStringLiteral("缺少发射机原始结果，无法计算 DerivedMetrics");
        return result;
    }

    auto fleet = TransferToEngine::convertDataModelToFleet(inputSnapshot);
    if (!fleet) {
        result.errorMessage = QStringLiteral("无法从冻结快照构建 Fleet 以计算 DerivedMetrics");
        return result;
    }

    const auto referenceTransmitter = resolveEnabledEquipment(
        inputSnapshot,
        inputSnapshot.emcAnalysisConfig.referenceTransmitterId);
    const auto referenceReceiver = resolveEnabledEquipment(
        inputSnapshot,
        inputSnapshot.emcAnalysisConfig.referenceReceiverId);
    if (!referenceTransmitter.has_value() || !referenceReceiver.has_value()) {
        result.errorMessage = QStringLiteral("参考链路设备解析失败");
        return result;
    }

    PEPropagationSolver solver(modelType, fleet.get());
    DerivedMetrics metrics;
    metrics.available = true;

    const double referenceNoiseFloorDbm = computeNoiseFloorDbm(*referenceReceiver->equipment);
    const std::vector<ResolvedEquipmentRef> transmitters =
        collectEnabledEquipments(inputSnapshot, isTransmitterLike);
    const std::vector<ResolvedEquipmentRef> receivers =
        collectEnabledEquipments(inputSnapshot, isReceiverLike);

    metrics.scf.thermalNoiseFloorDbm = referenceNoiseFloorDbm;
    metrics.scf.linkCount = 0;
    metrics.scf.couplingMatrix.matrixId = QStringLiteral("scf-coupling-matrix");
    metrics.scf.couplingMatrix.displayName = QStringLiteral("SCF 耦合矩阵");
    metrics.scf.couplingMatrix.valueUnit = QStringLiteral("dB");
    metrics.scf.couplingMatrix.rows = static_cast<int>(receivers.size());
    metrics.scf.couplingMatrix.cols = static_cast<int>(transmitters.size());
    metrics.scf.couplingMatrix.rowLabels.reserve(receivers.size());
    metrics.scf.couplingMatrix.colLabels.reserve(transmitters.size());
    metrics.scf.couplingMatrix.values.assign(
        static_cast<std::size_t>(receivers.size() * transmitters.size()),
        std::numeric_limits<double>::quiet_NaN());

    for (const auto& receiver : receivers) {
        metrics.scf.couplingMatrix.rowLabels.push_back(receiver.equipment->equipmentId);
    }
    for (const auto& transmitter : transmitters) {
        metrics.scf.couplingMatrix.colLabels.push_back(transmitter.equipment->equipmentId);
    }

    double scfSumDb = 0.0;
    for (int row = 0; row < static_cast<int>(receivers.size()); ++row) {
        for (int col = 0; col < static_cast<int>(transmitters.size()); ++col) {
            const auto& receiver = receivers[static_cast<std::size_t>(row)];
            const auto& transmitter = transmitters[static_cast<std::size_t>(col)];
            if (receiver.ship->shipId == transmitter.ship->shipId) {
                continue;
            }

            const double distanceM = std::hypot(
                receiver.worldX - transmitter.worldX,
                receiver.worldY - transmitter.worldY);
            const double pathLossDb = solver.computePathLossAtRange(
                makeTransmitterPEData(transmitter),
                inputSnapshot.environmentConfig,
                receiver.worldZ,
                distanceM);
            const double receivedPowerDbm =
                transmitter.equipment->transmitterPowerDbm +
                transmitter.equipment->gainDbi +
                receiver.equipment->gainDbi +
                receiver.equipment->receiverInterferenceMarginDb -
                pathLossDb;
            const double aboveNoiseDb = receivedPowerDbm - referenceNoiseFloorDbm;

            metrics.scf.couplingMatrix.values[static_cast<std::size_t>(row * transmitters.size() + col)] = aboveNoiseDb;
            scfSumDb += aboveNoiseDb;
            metrics.scf.linkCount += 1;
        }
    }

    if (metrics.scf.linkCount <= 0) {
        result.errorMessage = QStringLiteral("未找到可用于 SCF 的跨平台干扰链路");
        return result;
    }
    metrics.scf.scalarDb = scfSumDb / static_cast<double>(metrics.scf.linkCount);

    const Transmitter_PE_data referencePeData = makeTransmitterPEData(*referenceTransmitter);
    EnvironmentData baselineEnvironment = inputSnapshot.environmentConfig;
    baselineEnvironment.windSpeed = inputSnapshot.emcAnalysisConfig.s3iBaselineWindSpeedMps;

    const LineMap baselineLoss = solver.compute1D(
        referencePeData,
        baselineEnvironment,
        referenceReceiver->worldZ);
    const LineMap currentLoss = solver.compute1D(
        referencePeData,
        inputSnapshot.environmentConfig,
        referenceReceiver->worldZ);

    const std::size_t curveSize = std::min(baselineLoss.size(), currentLoss.size());
    if (curveSize == 0) {
        result.errorMessage = QStringLiteral("S3I 曲线为空");
        return result;
    }

    metrics.s3i.referenceTransmitterId = inputSnapshot.emcAnalysisConfig.referenceTransmitterId;
    metrics.s3i.referenceReceiverId = inputSnapshot.emcAnalysisConfig.referenceReceiverId;
    metrics.s3i.baselineWindSpeedMps = inputSnapshot.emcAnalysisConfig.s3iBaselineWindSpeedMps;
    metrics.s3i.currentWindSpeedMps = inputSnapshot.environmentConfig.windSpeed;
    metrics.s3i.calmCurve.seriesId = QStringLiteral("s3i-calm");
    metrics.s3i.calmCurve.displayName = QStringLiteral("平静海况曲线");
    metrics.s3i.calmCurve.xUnit = QStringLiteral("m");
    metrics.s3i.calmCurve.yUnit = QStringLiteral("dB");
    metrics.s3i.currentCurve.seriesId = QStringLiteral("s3i-current");
    metrics.s3i.currentCurve.displayName = QStringLiteral("当前海况曲线");
    metrics.s3i.currentCurve.xUnit = QStringLiteral("m");
    metrics.s3i.currentCurve.yUnit = QStringLiteral("dB");

    double s3iSumDb = 0.0;
    std::size_t s3iSampleCount = 0;
    for (std::size_t index = 0; index < curveSize; ++index) {
        const double rangeM = static_cast<double>(index + 1) * inputSnapshot.environmentConfig.dx;
        if (rangeM > inputSnapshot.environmentConfig.maxRange) {
            break;
        }
        if (rangeM <= 10.0) {
            continue;
        }

        const double calmValueDb = -baselineLoss[index];
        const double currentValueDb = -currentLoss[index];
        if (!std::isfinite(calmValueDb) || !std::isfinite(currentValueDb)) {
            continue;
        }

        metrics.s3i.calmCurve.xValues.push_back(rangeM);
        metrics.s3i.calmCurve.yValues.push_back(calmValueDb);
        metrics.s3i.currentCurve.xValues.push_back(rangeM);
        metrics.s3i.currentCurve.yValues.push_back(currentValueDb);
        s3iSumDb += std::abs(currentValueDb - calmValueDb);
        s3iSampleCount += 1;
    }

    if (s3iSampleCount == 0) {
        result.errorMessage = QStringLiteral("S3I 有效采样为空");
        return result;
    }
    metrics.s3i.scalarDb = s3iSumDb / static_cast<double>(s3iSampleCount);

    const ScalarField2D& sourceField = computationResult.emitterResults.front().field2D;
    std::vector<double> totalInterferenceMw(sourceField.values.size(), 0.0);

    for (const auto& emitterResult : computationResult.emitterResults) {
        if (emitterResult.status != EmitterResultStatus::Succeeded) {
            continue;
        }

        if (emitterResult.field2D.values.size() != sourceField.values.size() ||
            emitterResult.field2D.rows != sourceField.rows ||
            emitterResult.field2D.cols != sourceField.cols) {
            result.errorMessage = QStringLiteral("发射机结果场图尺寸不一致");
            return result;
        }

        const auto transmitter = resolveEnabledEquipment(inputSnapshot, emitterResult.emitterId);
        if (!transmitter.has_value() || !isTransmitterLike(*transmitter->equipment)) {
            result.errorMessage = QStringLiteral("无法根据发射机结果回溯发射设备: %1").arg(emitterResult.emitterId);
            return result;
        }

        for (std::size_t index = 0; index < emitterResult.field2D.values.size(); ++index) {
            const double pathLossDb = emitterResult.field2D.values[index];
            const double backgroundPowerDbm =
                emitterResult.transmitPowerDbm +
                transmitter->equipment->gainDbi -
                pathLossDb;
            totalInterferenceMw[index] += dbmToMw(backgroundPowerDbm);
        }
    }

    const double thermalNoiseMw = dbmToMw(referenceNoiseFloorDbm);
    metrics.tElev.field = cloneMetricField(
        sourceField,
        QStringLiteral("t-elev"),
        QStringLiteral("背景噪声抬升场"),
        ScalarFieldQuantity::NoiseElevationDb);
    metrics.dDesense.field = cloneMetricField(
        sourceField,
        QStringLiteral("d-desense"),
        QStringLiteral("灵敏度恶化场"),
        ScalarFieldQuantity::DesenseDb);
    metrics.dDesense.victimReceiverId = inputSnapshot.emcAnalysisConfig.referenceReceiverId;

    metrics.tElev.field.values.clear();
    metrics.dDesense.field.values.clear();
    metrics.tElev.field.values.reserve(totalInterferenceMw.size());
    metrics.dDesense.field.values.reserve(totalInterferenceMw.size());

    int exceedCount = 0;
    double desenseIntegral = 0.0;
    const double cellArea = sourceField.stepX * sourceField.stepY;
    const double totalArea =
        static_cast<double>(std::max(1, sourceField.cols - 1)) *
        static_cast<double>(std::max(1, sourceField.rows - 1)) *
        cellArea;

    for (double totalMw : totalInterferenceMw) {
        const double noiseElevationDb = 10.0 * std::log10((totalMw + thermalNoiseMw) / thermalNoiseMw);
        metrics.tElev.field.values.push_back(noiseElevationDb);

        const double interferenceAtVictimDbm = mwToDbm(totalMw) + referenceReceiver->equipment->gainDbi;
        const double desenseDb = std::max(
            interferenceAtVictimDbm - referenceReceiver->equipment->receiverSensitivityDbm,
            0.0);
        metrics.dDesense.field.values.push_back(desenseDb);

        if (desenseDb > 0.1) {
            exceedCount += 1;
        }
        desenseIntegral += desenseDb * cellArea;
    }

    metrics.tElev.maxDb = maxOf(metrics.tElev.field.values);
    metrics.tElev.meanDb = meanOf(metrics.tElev.field.values);
    metrics.dDesense.peakDb = maxOf(metrics.dDesense.field.values);
    metrics.dDesense.coveragePercent =
        100.0 * static_cast<double>(exceedCount) / static_cast<double>(metrics.dDesense.field.values.size());
    metrics.dDesense.adiDbPerSquareMeter = desenseIntegral / totalArea;

    const auto metricsValidation = metrics.validate();
    if (!metricsValidation.first) {
        result.errorMessage = metricsValidation.second;
        return result;
    }

    result.success = true;
    result.metrics = std::move(metrics);
    return result;
}
