#include "Utils/Reportflow/StandardInputExporter.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

#include "Interface/SchemaConstants.h"

namespace {

QJsonArray buildVector3(double x, double y, double z) {
    QJsonArray array;
    array.append(x);
    array.append(y);
    array.append(z);
    return array;
}

QJsonObject buildPoint3D(double x, double y, double z) {
    QJsonObject object;
    object.insert(
        QString::fromLatin1(SchemaKeys::Type),
        QString::fromLatin1(SchemaValues::Point3D));
    object.insert(QString::fromLatin1(SchemaKeys::Coordinates), buildVector3(x, y, z));
    return object;
}

const EquipmentData* findEquipment(
    const DataModel::DataSnapshot& snapshot,
    const QString& equipmentId) {
    for (const EquipmentData& equipment : snapshot.allEquipments) {
        if (equipment.equipmentId == equipmentId) {
            return &equipment;
        }
    }
    return nullptr;
}

bool validateExportableSnapshot(
    const DataModel::DataSnapshot& snapshot,
    QString* errorMessage) {
    const auto validation = DataModel::validateSnapshot(snapshot);
    if (!validation.first) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("标准输入导出前快照校验失败：%1").arg(validation.second);
        }
        return false;
    }

    for (const ShipData& ship : snapshot.allShips) {
        for (const EquipmentOnShip& equipmentRef : ship.equipmentRefs) {
            if (equipmentRef.isEnabled) {
                continue;
            }

            if (errorMessage) {
                *errorMessage = QStringLiteral(
                                    "标准输入导出暂不支持禁用设备引用：船只 %1 的设备 %2 当前为禁用状态")
                                    .arg(QString::fromStdString(ship.shipId), equipmentRef.equipmentId);
            }
            return false;
        }
    }

    return true;
}

QJsonObject buildTransmitterObject(const EquipmentData& equipment) {
    QJsonObject object;
    object.insert(QString::fromLatin1(SchemaKeys::ID), equipment.equipmentId);
    object.insert(QString::fromLatin1(SchemaKeys::Type), equipment.equipmentType);
    object.insert(QString::fromLatin1(SchemaKeys::GainDbi), equipment.gainDbi);
    object.insert(
        QString::fromLatin1(SchemaKeys::LocationOffset),
        buildVector3(equipment.offsetX, equipment.offsetY, equipment.offsetZ));
    object.insert(
        QString::fromLatin1(SchemaKeys::CenterFrequencyGHz),
        equipment.transmitterCenterFrequencyGHz);
    object.insert(
        QString::fromLatin1(SchemaKeys::BandwidthMHz),
        equipment.transmitterBandwidthMHz);
    object.insert(QString::fromLatin1(SchemaKeys::PowerDbm), equipment.transmitterPowerDbm);
    object.insert(
        QString::fromLatin1(SchemaKeys::AntennaPhiDeg),
        equipment.transmitterAntennaPhiDeg);
    object.insert(
        QString::fromLatin1(SchemaKeys::BeamWidthDeg),
        equipment.transmitterBeamWidthDeg);
    object.insert(
        QString::fromLatin1(SchemaKeys::Polarization),
        equipment.transmitterPolarization);
    object.insert(
        QString::fromLatin1(SchemaKeys::AntennaType),
        equipment.transmitterAntennaType);
    return object;
}

QJsonObject buildReceiverObject(const EquipmentData& equipment) {
    QJsonObject object;
    object.insert(QString::fromLatin1(SchemaKeys::ID), equipment.equipmentId);
    object.insert(QString::fromLatin1(SchemaKeys::Type), equipment.equipmentType);
    object.insert(QString::fromLatin1(SchemaKeys::GainDbi), equipment.gainDbi);
    object.insert(
        QString::fromLatin1(SchemaKeys::LocationOffset),
        buildVector3(equipment.offsetX, equipment.offsetY, equipment.offsetZ));
    object.insert(
        QString::fromLatin1(SchemaKeys::CenterFrequencyGHz),
        equipment.receiverCenterFrequencyGHz);
    object.insert(
        QString::fromLatin1(SchemaKeys::BandwidthMHz),
        equipment.receiverBandwidthMHz);
    object.insert(
        QString::fromLatin1(SchemaKeys::SensitivityDbm),
        equipment.receiverSensitivityDbm);
    object.insert(
        QString::fromLatin1(SchemaKeys::InterferenceMarginDb),
        equipment.receiverInterferenceMarginDb);
    object.insert(
        QString::fromLatin1(SchemaKeys::SinrMarginDb),
        equipment.receiverSinrMarginDb);
    object.insert(
        QString::fromLatin1(SchemaKeys::NoiseFigureDb),
        equipment.receiverNoiseFigureDb);
    return object;
}

QJsonObject buildTransceiverObject(const EquipmentData& equipment) {
    QJsonObject transmitterConfig;
    transmitterConfig.insert(
        QString::fromLatin1(SchemaKeys::CenterFrequencyGHz),
        equipment.transmitterCenterFrequencyGHz);
    transmitterConfig.insert(
        QString::fromLatin1(SchemaKeys::BandwidthMHz),
        equipment.transmitterBandwidthMHz);
    transmitterConfig.insert(
        QString::fromLatin1(SchemaKeys::PowerDbm),
        equipment.transmitterPowerDbm);
    transmitterConfig.insert(
        QString::fromLatin1(SchemaKeys::AntennaPhiDeg),
        equipment.transmitterAntennaPhiDeg);
    transmitterConfig.insert(
        QString::fromLatin1(SchemaKeys::BeamWidthDeg),
        equipment.transmitterBeamWidthDeg);
    transmitterConfig.insert(
        QString::fromLatin1(SchemaKeys::Polarization),
        equipment.transmitterPolarization);
    transmitterConfig.insert(
        QString::fromLatin1(SchemaKeys::AntennaType),
        equipment.transmitterAntennaType);

    QJsonObject receiverConfig;
    receiverConfig.insert(
        QString::fromLatin1(SchemaKeys::CenterFrequencyGHz),
        equipment.receiverCenterFrequencyGHz);
    receiverConfig.insert(
        QString::fromLatin1(SchemaKeys::BandwidthMHz),
        equipment.receiverBandwidthMHz);
    receiverConfig.insert(
        QString::fromLatin1(SchemaKeys::SensitivityDbm),
        equipment.receiverSensitivityDbm);
    receiverConfig.insert(
        QString::fromLatin1(SchemaKeys::InterferenceMarginDb),
        equipment.receiverInterferenceMarginDb);
    receiverConfig.insert(
        QString::fromLatin1(SchemaKeys::SinrMarginDb),
        equipment.receiverSinrMarginDb);
    receiverConfig.insert(
        QString::fromLatin1(SchemaKeys::NoiseFigureDb),
        equipment.receiverNoiseFigureDb);

    QJsonObject object;
    object.insert(QString::fromLatin1(SchemaKeys::ID), equipment.equipmentId);
    object.insert(QString::fromLatin1(SchemaKeys::Type), equipment.equipmentType);
    object.insert(QString::fromLatin1(SchemaKeys::GainDbi), equipment.gainDbi);
    object.insert(
        QString::fromLatin1(SchemaKeys::LocationOffset),
        buildVector3(equipment.offsetX, equipment.offsetY, equipment.offsetZ));
    object.insert(
        QString::fromLatin1(SchemaKeys::TransmitterConfig),
        transmitterConfig);
    object.insert(
        QString::fromLatin1(SchemaKeys::ReceiverConfig),
        receiverConfig);
    return object;
}

}  // namespace

bool StandardInputExporter::buildJsonObject(
    const DataModel::DataSnapshot& snapshot,
    QJsonObject* object,
    QString* errorMessage) {
    if (object == nullptr) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("标准输入导出目标对象不能为空");
        }
        return false;
    }
    if (!validateExportableSnapshot(snapshot, errorMessage)) {
        return false;
    }

    QJsonObject root;
    root.insert(
        QString::fromLatin1(SchemaKeys::SchemaVersion),
        QString::fromLatin1(SchemaValues::SchemaVersion_1_0_0));

    QJsonObject environment;
    environment.insert(QString::fromLatin1(SchemaKeys::MaxRange), snapshot.environmentConfig.maxRange);
    environment.insert(QString::fromLatin1(SchemaKeys::DuctHeight), snapshot.environmentConfig.ductHeight);
    environment.insert(QString::fromLatin1(SchemaKeys::WindSpeed), snapshot.environmentConfig.windSpeed);
    environment.insert(QString::fromLatin1(SchemaKeys::Dx), snapshot.environmentConfig.dx);
    environment.insert(QString::fromLatin1(SchemaKeys::Dz), snapshot.environmentConfig.dz);
    environment.insert(QString::fromLatin1(SchemaKeys::Nz), snapshot.environmentConfig.nz);
    environment.insert(QString::fromLatin1(SchemaKeys::AngleStepDeg), snapshot.environmentConfig.angleStepDeg);
    root.insert(QString::fromLatin1(SchemaKeys::Environment), environment);

    QJsonObject analysis;
    analysis.insert(
        QString::fromLatin1(SchemaKeys::FieldPlaneHeightM),
        snapshot.emcAnalysisConfig.fieldPlaneHeightM);
    analysis.insert(
        QString::fromLatin1(SchemaKeys::ReferenceTransmitterId),
        snapshot.emcAnalysisConfig.referenceTransmitterId);
    analysis.insert(
        QString::fromLatin1(SchemaKeys::ReferenceReceiverId),
        snapshot.emcAnalysisConfig.referenceReceiverId);
    analysis.insert(
        QString::fromLatin1(SchemaKeys::S3IBaselineWindSpeedMps),
        snapshot.emcAnalysisConfig.s3iBaselineWindSpeedMps);
    root.insert(QString::fromLatin1(SchemaKeys::EMCAnalysisConfig), analysis);

    QJsonArray usvs;
    for (const ShipData& ship : snapshot.allShips) {
        QJsonObject shipObject;
        shipObject.insert(QString::fromLatin1(SchemaKeys::ID), QString::fromStdString(ship.shipId));
        shipObject.insert(
            QString::fromLatin1(SchemaKeys::Location),
            buildPoint3D(ship.worldX, ship.worldY, ship.worldZ));
        shipObject.insert(QString::fromLatin1(SchemaKeys::Speed), ship.shipSpeedMps);
        shipObject.insert(QString::fromLatin1(SchemaKeys::ShipOrientationDeg), ship.shipOrientationDeg);

        QJsonArray transmitters;
        QJsonArray receivers;
        QJsonArray transceivers;

        for (const EquipmentOnShip& equipmentRef : ship.equipmentRefs) {
            const EquipmentData* equipment = findEquipment(snapshot, equipmentRef.equipmentId);
            if (equipment == nullptr) {
                continue;
            }

            if (equipment->equipmentType == QString::fromLatin1(SchemaValues::Transmitter)) {
                transmitters.append(buildTransmitterObject(*equipment));
            } else if (equipment->equipmentType == QString::fromLatin1(SchemaValues::Receiver)) {
                receivers.append(buildReceiverObject(*equipment));
            } else if (equipment->equipmentType == QString::fromLatin1(SchemaValues::Transceiver)) {
                transceivers.append(buildTransceiverObject(*equipment));
            }
        }

        shipObject.insert(QString::fromLatin1(SchemaKeys::Transmitters), transmitters);
        shipObject.insert(QString::fromLatin1(SchemaKeys::Receivers), receivers);
        shipObject.insert(QString::fromLatin1(SchemaKeys::Transceivers), transceivers);
        usvs.append(shipObject);
    }

    root.insert(QString::fromLatin1(SchemaKeys::Usvs), usvs);
    *object = root;
    return true;
}

bool StandardInputExporter::buildJsoncText(
    const DataModel::DataSnapshot& snapshot,
    QString* jsoncText,
    QString* errorMessage) {
    if (jsoncText == nullptr) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("标准输入导出目标文本不能为空");
        }
        return false;
    }

    QJsonObject object;
    if (!buildJsonObject(snapshot, &object, errorMessage)) {
        return false;
    }

    *jsoncText = QString::fromUtf8(QJsonDocument(object).toJson(QJsonDocument::Indented));
    return true;
}

bool StandardInputExporter::writeJsoncFile(
    const QString& filePath,
    const DataModel::DataSnapshot& snapshot,
    QString* errorMessage) {
    QString jsoncText;
    if (!buildJsoncText(snapshot, &jsoncText, errorMessage)) {
        return false;
    }

    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("无法写入文件：%1").arg(filePath);
        }
        return false;
    }

    const QByteArray content = jsoncText.toUtf8();
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
