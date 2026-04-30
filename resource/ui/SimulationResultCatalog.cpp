#include "Resource/ui/SimulationResultCatalog.h"

#include <iterator>

#include <QtGlobal>

namespace {

QString formatDb(double value) {
    return QString::number(value, 'f', 2) + QStringLiteral(" dB");
}

QString formatDbm(double value) {
    return QString::number(value, 'f', 2) + QStringLiteral(" dBm");
}

QString formatPercent(double value) {
    return QString::number(value, 'f', 1) + QStringLiteral("%");
}

QString formatMetersPerSecond(double value) {
    return QString::number(value, 'f', 1) + QStringLiteral(" m/s");
}

SimulationChartCardDescriptor makeCardDescriptor(const SimulationChartPayload& payload) {
    SimulationChartCardDescriptor card;
    card.key = payload.key;
    card.payloadType = payload.payloadType;
    card.title = payload.title;
    card.subtitle = payload.subtitle;
    card.available = payload.available;
    return card;
}

}  // namespace

std::vector<SimulationChartCardDescriptor> SimulationResultCatalog::buildCards(const SimulationTaskResult& result) {
    if (result.status != SimulationResultStatus::Succeeded) {
        return {};
    }

    const SimulationChartKey orderedKeys[] = {
        SimulationChartKey::AggregatedField,
        SimulationChartKey::ReferenceEmitterPathLoss,
        SimulationChartKey::ScfMatrix,
        SimulationChartKey::S3iCurve,
        SimulationChartKey::TElevField,
        SimulationChartKey::DDesenseField,
    };

    std::vector<SimulationChartCardDescriptor> cards;
    cards.reserve(std::size(orderedKeys));
    for (SimulationChartKey key : orderedKeys) {
        cards.push_back(makeCardDescriptor(payloadForKey(result, key)));
    }
    return cards;
}

SimulationChartPayload SimulationResultCatalog::payloadForKey(
    const SimulationTaskResult& result,
    SimulationChartKey key) {
    SimulationChartPayload payload;
    payload.key = key;

    switch (key) {
    case SimulationChartKey::AggregatedField:
        payload.payloadType = SimulationChartPayloadType::ScalarField2D;
        payload.title = QStringLiteral("总场分布");
        payload.subtitle = QStringLiteral("聚合电磁功率场");
        payload.detailSummary = QStringLiteral("单位：%1").arg(result.aggregatedField.valueUnit);
        payload.available = !result.aggregatedField.values.empty();
        payload.scalarField = payload.available ? &result.aggregatedField : nullptr;
        return payload;

    case SimulationChartKey::ReferenceEmitterPathLoss: {
        payload.payloadType = SimulationChartPayloadType::ScalarField2D;
        payload.title = QStringLiteral("参考发射机路径损耗");

        const EmitterResult* emitterResult = findReferenceEmitterResult(result);
        if (!emitterResult) {
            payload.subtitle = QStringLiteral("未找到参考发射机结果");
            payload.detailSummary = QStringLiteral("参考发射机：%1")
                                        .arg(result.inputSnapshot.emcAnalysisConfig.referenceTransmitterId);
            payload.available = false;
            return payload;
        }

        payload.subtitle = QStringLiteral("发射机：%1").arg(emitterResult->emitterId);
        payload.detailSummary = QStringLiteral("单位：%1，所属船只：%2")
                                    .arg(emitterResult->field2D.valueUnit, emitterResult->shipId);
        payload.available = !emitterResult->field2D.values.empty();
        payload.scalarField = payload.available ? &emitterResult->field2D : nullptr;
        return payload;
    }

    case SimulationChartKey::ScfMatrix:
        payload.payloadType = SimulationChartPayloadType::LabeledMatrix2D;
        payload.title = QStringLiteral("SCF");
        payload.subtitle = QStringLiteral("耦合矩阵");
        payload.detailSummary = QStringLiteral("SCF：%1，热噪声底：%2，链路数：%3")
                                    .arg(formatDb(result.derivedMetrics.scf.scalarDb))
                                    .arg(formatDbm(result.derivedMetrics.scf.thermalNoiseFloorDbm))
                                    .arg(result.derivedMetrics.scf.linkCount);
        payload.available = result.derivedMetrics.available &&
                            !result.derivedMetrics.scf.couplingMatrix.values.empty();
        payload.matrix = payload.available ? &result.derivedMetrics.scf.couplingMatrix : nullptr;
        return payload;

    case SimulationChartKey::S3iCurve:
        payload.payloadType = SimulationChartPayloadType::Series1D;
        payload.title = QStringLiteral("S3I");
        payload.subtitle = QStringLiteral("海况敏感度曲线");
        payload.detailSummary = QStringLiteral("链路：%1 -> %2，基准海况：%3，当前海况：%4")
                                    .arg(result.derivedMetrics.s3i.referenceTransmitterId)
                                    .arg(result.derivedMetrics.s3i.referenceReceiverId)
                                    .arg(formatMetersPerSecond(result.derivedMetrics.s3i.baselineWindSpeedMps))
                                    .arg(formatMetersPerSecond(result.derivedMetrics.s3i.currentWindSpeedMps));
        payload.available = result.derivedMetrics.available &&
                            !result.derivedMetrics.s3i.calmCurve.xValues.empty() &&
                            !result.derivedMetrics.s3i.currentCurve.xValues.empty();
        payload.primarySeries = payload.available ? &result.derivedMetrics.s3i.calmCurve : nullptr;
        payload.secondarySeries = payload.available ? &result.derivedMetrics.s3i.currentCurve : nullptr;
        return payload;

    case SimulationChartKey::TElevField:
        payload.payloadType = SimulationChartPayloadType::ScalarField2D;
        payload.title = QStringLiteral("T_elev");
        payload.subtitle = QStringLiteral("背景噪声抬升图");
        payload.detailSummary = QStringLiteral("最大值：%1，平均值：%2")
                                    .arg(formatDb(result.derivedMetrics.tElev.maxDb))
                                    .arg(formatDb(result.derivedMetrics.tElev.meanDb));
        payload.available = result.derivedMetrics.available &&
                            !result.derivedMetrics.tElev.field.values.empty();
        payload.scalarField = payload.available ? &result.derivedMetrics.tElev.field : nullptr;
        return payload;

    case SimulationChartKey::DDesenseField:
        payload.payloadType = SimulationChartPayloadType::ScalarField2D;
        payload.title = QStringLiteral("D_desense");
        payload.subtitle = QStringLiteral("接收机灵敏度恶化图");
        payload.detailSummary = QStringLiteral("受害机：%1，峰值：%2，覆盖率：%3")
                                    .arg(result.derivedMetrics.dDesense.victimReceiverId)
                                    .arg(formatDb(result.derivedMetrics.dDesense.peakDb))
                                    .arg(formatPercent(result.derivedMetrics.dDesense.coveragePercent));
        payload.available = result.derivedMetrics.available &&
                            !result.derivedMetrics.dDesense.field.values.empty();
        payload.scalarField = payload.available ? &result.derivedMetrics.dDesense.field : nullptr;
        return payload;
    }

    return payload;
}

const EmitterResult* SimulationResultCatalog::findReferenceEmitterResult(const SimulationTaskResult& result) {
    const QString referenceEmitterId = result.inputSnapshot.emcAnalysisConfig.referenceTransmitterId;
    for (const EmitterResult& emitterResult : result.emitterResults) {
        if (emitterResult.status != EmitterResultStatus::Succeeded) {
            continue;
        }
        if (emitterResult.emitterId == referenceEmitterId) {
            return &emitterResult;
        }
    }
    return nullptr;
}
