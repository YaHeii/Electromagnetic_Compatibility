#pragma once

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStringList>

#include <cmath>
#include <limits>

#include "Interface/DataModel.h"
#include "Interface/SchemaConstants.h"
#include "spdlog/spdlog.h"

class JsonLoader {
public:
    static bool LoadFile(const QString& filePath) {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            spdlog::error("无法打开配置文件: {}, {}", filePath.toStdString(), file.errorString().toStdString());
            return false;
        }

        QString jsonString = QString::fromUtf8(file.readAll());
        file.close();
        stripSingleLineComments(jsonString);

        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(jsonString.toUtf8(), &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            spdlog::error("JSON 解析失败: {}", parseError.errorString().toStdString());
            return false;
        }
        if (!document.isObject()) {
            spdlog::error("JSON 根节点必须是 object");
            return false;
        }

        DataModel::DataSnapshot snapshot;
        QString errorMessage;
        if (!parseRootObject(document.object(), snapshot, errorMessage)) {
            spdlog::error("Schema 基础校验失败: {}", errorMessage.toStdString());
            return false;
        }

        const auto validationResult = DataModel::validateSnapshot(snapshot);
        if (!validationResult.first) {
            spdlog::error("DataModel 核心校验失败: {}", validationResult.second.toStdString());
            return false;
        }

        DataModel* model = DataModel::instance();
        model->allEquipments = std::move(snapshot.allEquipments);
        model->allShips = std::move(snapshot.allShips);
        model->environmentConfig = snapshot.environmentConfig;
        model->emcAnalysisConfig = snapshot.emcAnalysisConfig;

        spdlog::info(
            "成功加载 schema 配置: {}, 船只数量: {}, 设备数量: {}",
            filePath.toStdString(),
            model->allShips.size(),
            model->allEquipments.size());
        return true;
    }

private:
    static void stripSingleLineComments(QString& jsonString) {
        const QRegularExpression commentPattern(QStringLiteral("//[^\\n\\r]*"));
        jsonString.replace(commentPattern, QString());
    }

    static bool parseRootObject(
        const QJsonObject& rootObject,
        DataModel::DataSnapshot& snapshot,
        QString& errorMessage) {
        if (!validateAllowedKeys(
                rootObject,
                {
                    QString::fromLatin1(SchemaKeys::SchemaVersion),
                    QString::fromLatin1(SchemaKeys::Environment),
                    QString::fromLatin1(SchemaKeys::EMCAnalysisConfig),
                    QString::fromLatin1(SchemaKeys::Usvs),
                },
                QStringLiteral("root"),
                errorMessage)) {
            return false;
        }

        QString schemaVersion;
        if (!readRequiredString(rootObject, SchemaKeys::SchemaVersion, schemaVersion, errorMessage)) {
            return false;
        }
        if (schemaVersion != QString::fromLatin1(SchemaValues::SchemaVersion_1_0_0)) {
            errorMessage = QStringLiteral("schemaVersion 必须为 %1")
                               .arg(QString::fromLatin1(SchemaValues::SchemaVersion_1_0_0));
            return false;
        }

        QJsonObject environmentObject;
        if (!readRequiredObject(rootObject, SchemaKeys::Environment, environmentObject, errorMessage)) {
            return false;
        }
        if (!parseEnvironment(environmentObject, snapshot.environmentConfig, errorMessage)) {
            return false;
        }

        QJsonObject analysisObject;
        if (!readRequiredObject(rootObject, SchemaKeys::EMCAnalysisConfig, analysisObject, errorMessage)) {
            return false;
        }
        if (!parseAnalysisConfig(analysisObject, snapshot.emcAnalysisConfig, errorMessage)) {
            return false;
        }

        QJsonArray usvArray;
        if (!readRequiredArray(rootObject, SchemaKeys::Usvs, usvArray, errorMessage)) {
            return false;
        }

        for (int index = 0; index < usvArray.size(); ++index) {
            if (!usvArray.at(index).isObject()) {
                errorMessage = QStringLiteral("usvs[%1] 必须是 object").arg(index);
                return false;
            }

            ShipData shipData;
            if (!parseShip(
                    usvArray.at(index).toObject(),
                    index,
                    shipData,
                    snapshot.allEquipments,
                    errorMessage)) {
                return false;
            }
            snapshot.allShips.push_back(std::move(shipData));
        }

        return true;
    }

    static bool parseEnvironment(
        const QJsonObject& environmentObject,
        EnvironmentData& environmentConfig,
        QString& errorMessage) {
        if (!validateAllowedKeys(
                environmentObject,
                {
                    QString::fromLatin1(SchemaKeys::MaxRange),
                    QString::fromLatin1(SchemaKeys::DuctHeight),
                    QString::fromLatin1(SchemaKeys::WindSpeed),
                    QString::fromLatin1(SchemaKeys::Dx),
                    QString::fromLatin1(SchemaKeys::Dz),
                    QString::fromLatin1(SchemaKeys::Nz),
                    QString::fromLatin1(SchemaKeys::AngleStepDeg),
                },
                QStringLiteral("environment"),
                errorMessage)) {
            return false;
        }

        return readRequiredNumber(environmentObject, SchemaKeys::MaxRange, environmentConfig.maxRange, errorMessage) &&
               readRequiredNumber(environmentObject, SchemaKeys::DuctHeight, environmentConfig.ductHeight, errorMessage) &&
               readRequiredNumber(environmentObject, SchemaKeys::WindSpeed, environmentConfig.windSpeed, errorMessage) &&
               readRequiredNumber(environmentObject, SchemaKeys::Dx, environmentConfig.dx, errorMessage) &&
               readRequiredNumber(environmentObject, SchemaKeys::Dz, environmentConfig.dz, errorMessage) &&
               readRequiredInteger(environmentObject, SchemaKeys::Nz, environmentConfig.nz, errorMessage) &&
               readRequiredInteger(environmentObject, SchemaKeys::AngleStepDeg, environmentConfig.angleStepDeg, errorMessage);
    }

    static bool parseAnalysisConfig(
        const QJsonObject& analysisObject,
        EMCAnalysisConfig& analysisConfig,
        QString& errorMessage) {
        if (!validateAllowedKeys(
                analysisObject,
                {
                    QString::fromLatin1(SchemaKeys::FieldPlaneHeightM),
                    QString::fromLatin1(SchemaKeys::ReferenceTransmitterId),
                    QString::fromLatin1(SchemaKeys::ReferenceReceiverId),
                    QString::fromLatin1(SchemaKeys::S3IBaselineWindSpeedMps),
                },
                QStringLiteral("emcAnalysisConfig"),
                errorMessage)) {
            return false;
        }

        return readRequiredNumber(
                   analysisObject,
                   SchemaKeys::FieldPlaneHeightM,
                   analysisConfig.fieldPlaneHeightM,
                   errorMessage) &&
               readRequiredString(
                   analysisObject,
                   SchemaKeys::ReferenceTransmitterId,
                   analysisConfig.referenceTransmitterId,
                   errorMessage) &&
               readRequiredString(
                   analysisObject,
                   SchemaKeys::ReferenceReceiverId,
                   analysisConfig.referenceReceiverId,
                   errorMessage) &&
               readRequiredNumber(
                   analysisObject,
                   SchemaKeys::S3IBaselineWindSpeedMps,
                   analysisConfig.s3iBaselineWindSpeedMps,
                   errorMessage);
    }

    static bool parseShip(
        const QJsonObject& shipObject,
        int shipIndex,
        ShipData& shipData,
        std::vector<EquipmentData>& allEquipments,
        QString& errorMessage) {
        const QString shipContext = QStringLiteral("usvs[%1]").arg(shipIndex);

        if (!validateAllowedKeys(
                shipObject,
                {
                    QString::fromLatin1(SchemaKeys::ID),
                    QString::fromLatin1(SchemaKeys::Location),
                    QString::fromLatin1(SchemaKeys::Speed),
                    QString::fromLatin1(SchemaKeys::ShipOrientationDeg),
                    QString::fromLatin1(SchemaKeys::Transmitters),
                    QString::fromLatin1(SchemaKeys::Receivers),
                    QString::fromLatin1(SchemaKeys::Transceivers),
                },
                shipContext,
                errorMessage)) {
            return false;
        }

        QString shipId;
        if (!readRequiredString(shipObject, SchemaKeys::ID, shipId, errorMessage)) {
            return false;
        }
        shipData.shipId = shipId.toStdString();

        QJsonObject locationObject;
        if (!readRequiredObject(shipObject, SchemaKeys::Location, locationObject, errorMessage)) {
            return false;
        }
        if (!parsePoint3D(
                locationObject,
                shipData.worldX,
                shipData.worldY,
                shipData.worldZ,
                shipContext + QStringLiteral(".location"),
                errorMessage)) {
            return false;
        }

        if (!readRequiredNumber(shipObject, SchemaKeys::Speed, shipData.shipSpeedMps, errorMessage)) {
            return false;
        }
        if (!readRequiredNumber(shipObject, SchemaKeys::ShipOrientationDeg, shipData.shipOrientationDeg, errorMessage)) {
            return false;
        }

        QJsonArray transmitterArray;
        if (!readRequiredArray(shipObject, SchemaKeys::Transmitters, transmitterArray, errorMessage)) {
            return false;
        }
        if (!parseDeviceArray(
                transmitterArray,
                SchemaDeviceType::Transmitter,
                shipContext + QStringLiteral(".transmitters"),
                shipData,
                allEquipments,
                errorMessage)) {
            return false;
        }

        QJsonArray receiverArray;
        if (!readRequiredArray(shipObject, SchemaKeys::Receivers, receiverArray, errorMessage)) {
            return false;
        }
        if (!parseDeviceArray(
                receiverArray,
                SchemaDeviceType::Receiver,
                shipContext + QStringLiteral(".receivers"),
                shipData,
                allEquipments,
                errorMessage)) {
            return false;
        }

        QJsonArray transceiverArray;
        if (!readRequiredArray(shipObject, SchemaKeys::Transceivers, transceiverArray, errorMessage)) {
            return false;
        }
        if (!parseDeviceArray(
                transceiverArray,
                SchemaDeviceType::Transceiver,
                shipContext + QStringLiteral(".transceivers"),
                shipData,
                allEquipments,
                errorMessage)) {
            return false;
        }

        return true;
    }

    static bool parseDeviceArray(
        const QJsonArray& deviceArray,
        SchemaDeviceType deviceType,
        const QString& context,
        ShipData& shipData,
        std::vector<EquipmentData>& allEquipments,
        QString& errorMessage) {
        for (int index = 0; index < deviceArray.size(); ++index) {
            if (!deviceArray.at(index).isObject()) {
                errorMessage = QStringLiteral("%1[%2] 必须是 object").arg(context).arg(index);
                return false;
            }

            EquipmentData equipmentData;
            QString equipmentId;
            const QString deviceContext = QStringLiteral("%1[%2]").arg(context).arg(index);
            bool parsed = false;
            if (deviceType == SchemaDeviceType::Transmitter) {
                parsed = parseTransmitter(deviceArray.at(index).toObject(), equipmentData, equipmentId, deviceContext, errorMessage);
            } else if (deviceType == SchemaDeviceType::Receiver) {
                parsed = parseReceiver(deviceArray.at(index).toObject(), equipmentData, equipmentId, deviceContext, errorMessage);
            } else {
                parsed = parseTransceiver(deviceArray.at(index).toObject(), equipmentData, equipmentId, deviceContext, errorMessage);
            }
            if (!parsed) {
                return false;
            }

            shipData.equipmentRefs.push_back(EquipmentOnShip{equipmentId, true});
            allEquipments.push_back(std::move(equipmentData));
        }

        return true;
    }

    static bool parseSharedDeviceFields(
        const QJsonObject& deviceObject,
        EquipmentData& equipmentData,
        QString& errorMessage) {
        return readRequiredNumber(deviceObject, SchemaKeys::GainDbi, equipmentData.gainDbi, errorMessage) &&
               readRequiredVector3(
                   deviceObject,
                   SchemaKeys::LocationOffset,
                   equipmentData.offsetX,
                   equipmentData.offsetY,
                   equipmentData.offsetZ,
                   errorMessage);
    }

    static bool parseTransmitterConfig(
        const QJsonObject& transmitterObject,
        EquipmentData& equipmentData,
        const QString& context,
        QString& errorMessage) {
        if (!validateAllowedKeys(
                transmitterObject,
                {
                    QString::fromLatin1(SchemaKeys::CenterFrequencyGHz),
                    QString::fromLatin1(SchemaKeys::BandwidthMHz),
                    QString::fromLatin1(SchemaKeys::PowerDbm),
                    QString::fromLatin1(SchemaKeys::AntennaPhiDeg),
                    QString::fromLatin1(SchemaKeys::BeamWidthDeg),
                    QString::fromLatin1(SchemaKeys::Polarization),
                    QString::fromLatin1(SchemaKeys::AntennaType),
                },
                context,
                errorMessage)) {
            return false;
        }

        QString polarization;
        if (!readRequiredString(transmitterObject, SchemaKeys::Polarization, polarization, errorMessage)) {
            return false;
        }
        if (!validateEnumValue(
                polarization,
                {
                    QString::fromLatin1(SchemaValues::Vertical),
                    QString::fromLatin1(SchemaValues::Horizontal),
                },
                context + QStringLiteral(".polarization"),
                errorMessage)) {
            return false;
        }

        QString antennaType;
        if (!readRequiredString(transmitterObject, SchemaKeys::AntennaType, antennaType, errorMessage)) {
            return false;
        }
        if (!validateEnumValue(
                antennaType,
                {
                    QString::fromLatin1(SchemaValues::Omni),
                    QString::fromLatin1(SchemaValues::Directional),
                    QString::fromLatin1(SchemaValues::Horn),
                    QString::fromLatin1(SchemaValues::ShapedBeam),
                    QString::fromLatin1(SchemaValues::Reflector),
                },
                context + QStringLiteral(".antennaType"),
                errorMessage)) {
            return false;
        }

        equipmentData.transmitterPolarization = polarization;
        equipmentData.transmitterAntennaType = antennaType;

        return readRequiredNumber(transmitterObject, SchemaKeys::CenterFrequencyGHz, equipmentData.transmitterCenterFrequencyGHz, errorMessage) &&
               readRequiredNumber(transmitterObject, SchemaKeys::BandwidthMHz, equipmentData.transmitterBandwidthMHz, errorMessage) &&
               readRequiredNumber(transmitterObject, SchemaKeys::PowerDbm, equipmentData.transmitterPowerDbm, errorMessage) &&
               readRequiredNumber(transmitterObject, SchemaKeys::AntennaPhiDeg, equipmentData.transmitterAntennaPhiDeg, errorMessage) &&
               readRequiredNumber(transmitterObject, SchemaKeys::BeamWidthDeg, equipmentData.transmitterBeamWidthDeg, errorMessage);
    }

    static bool parseReceiverConfig(
        const QJsonObject& receiverObject,
        EquipmentData& equipmentData,
        const QString& context,
        QString& errorMessage) {
        if (!validateAllowedKeys(
                receiverObject,
                {
                    QString::fromLatin1(SchemaKeys::CenterFrequencyGHz),
                    QString::fromLatin1(SchemaKeys::BandwidthMHz),
                    QString::fromLatin1(SchemaKeys::SensitivityDbm),
                    QString::fromLatin1(SchemaKeys::InterferenceMarginDb),
                    QString::fromLatin1(SchemaKeys::SinrMarginDb),
                    QString::fromLatin1(SchemaKeys::NoiseFigureDb),
                },
                context,
                errorMessage)) {
            return false;
        }

        return readRequiredNumber(receiverObject, SchemaKeys::CenterFrequencyGHz, equipmentData.receiverCenterFrequencyGHz, errorMessage) &&
               readRequiredNumber(receiverObject, SchemaKeys::BandwidthMHz, equipmentData.receiverBandwidthMHz, errorMessage) &&
               readRequiredNumber(receiverObject, SchemaKeys::SensitivityDbm, equipmentData.receiverSensitivityDbm, errorMessage) &&
               readRequiredNumber(receiverObject, SchemaKeys::InterferenceMarginDb, equipmentData.receiverInterferenceMarginDb, errorMessage) &&
               readRequiredNumber(receiverObject, SchemaKeys::SinrMarginDb, equipmentData.receiverSinrMarginDb, errorMessage) &&
               readRequiredNumber(receiverObject, SchemaKeys::NoiseFigureDb, equipmentData.receiverNoiseFigureDb, errorMessage);
    }

    static bool parseTransmitter(
        const QJsonObject& transmitterObject,
        EquipmentData& equipmentData,
        QString& equipmentId,
        const QString& context,
        QString& errorMessage) {
        if (!validateAllowedKeys(
                transmitterObject,
                {
                    QString::fromLatin1(SchemaKeys::ID),
                    QString::fromLatin1(SchemaKeys::Type),
                    QString::fromLatin1(SchemaKeys::GainDbi),
                    QString::fromLatin1(SchemaKeys::LocationOffset),
                    QString::fromLatin1(SchemaKeys::CenterFrequencyGHz),
                    QString::fromLatin1(SchemaKeys::BandwidthMHz),
                    QString::fromLatin1(SchemaKeys::PowerDbm),
                    QString::fromLatin1(SchemaKeys::AntennaPhiDeg),
                    QString::fromLatin1(SchemaKeys::BeamWidthDeg),
                    QString::fromLatin1(SchemaKeys::Polarization),
                    QString::fromLatin1(SchemaKeys::AntennaType),
                },
                context,
                errorMessage)) {
            return false;
        }

        if (!readRequiredString(transmitterObject, SchemaKeys::ID, equipmentId, errorMessage)) {
            return false;
        }

        QString type;
        if (!readRequiredString(transmitterObject, SchemaKeys::Type, type, errorMessage)) {
            return false;
        }
        if (type != QString::fromLatin1(SchemaValues::Transmitter)) {
            errorMessage = QStringLiteral("%1.type 必须为 %2")
                               .arg(context, QString::fromLatin1(SchemaValues::Transmitter));
            return false;
        }

        QString polarization;
        if (!readRequiredString(transmitterObject, SchemaKeys::Polarization, polarization, errorMessage)) {
            return false;
        }
        if (!validateEnumValue(
                polarization,
                {
                    QString::fromLatin1(SchemaValues::Vertical),
                    QString::fromLatin1(SchemaValues::Horizontal),
                },
                context + QStringLiteral(".polarization"),
                errorMessage)) {
            return false;
        }

        QString antennaType;
        if (!readRequiredString(transmitterObject, SchemaKeys::AntennaType, antennaType, errorMessage)) {
            return false;
        }
        if (!validateEnumValue(
                antennaType,
                {
                    QString::fromLatin1(SchemaValues::Omni),
                    QString::fromLatin1(SchemaValues::Directional),
                    QString::fromLatin1(SchemaValues::Horn),
                    QString::fromLatin1(SchemaValues::ShapedBeam),
                    QString::fromLatin1(SchemaValues::Reflector),
                },
                context + QStringLiteral(".antennaType"),
                errorMessage)) {
            return false;
        }

        equipmentData.equipmentId = equipmentId;
        equipmentData.equipmentType = QString::fromLatin1(SchemaValues::Transmitter);
        equipmentData.transmitterPolarization = polarization;
        equipmentData.transmitterAntennaType = antennaType;

        return parseSharedDeviceFields(transmitterObject, equipmentData, errorMessage) &&
               readRequiredNumber(transmitterObject, SchemaKeys::CenterFrequencyGHz, equipmentData.transmitterCenterFrequencyGHz, errorMessage) &&
               readRequiredNumber(transmitterObject, SchemaKeys::BandwidthMHz, equipmentData.transmitterBandwidthMHz, errorMessage) &&
               readRequiredNumber(transmitterObject, SchemaKeys::PowerDbm, equipmentData.transmitterPowerDbm, errorMessage) &&
               readRequiredNumber(transmitterObject, SchemaKeys::AntennaPhiDeg, equipmentData.transmitterAntennaPhiDeg, errorMessage) &&
               readRequiredNumber(transmitterObject, SchemaKeys::BeamWidthDeg, equipmentData.transmitterBeamWidthDeg, errorMessage);
    }

    static bool parseReceiver(
        const QJsonObject& receiverObject,
        EquipmentData& equipmentData,
        QString& equipmentId,
        const QString& context,
        QString& errorMessage) {
        if (!validateAllowedKeys(
                receiverObject,
                {
                    QString::fromLatin1(SchemaKeys::ID),
                    QString::fromLatin1(SchemaKeys::Type),
                    QString::fromLatin1(SchemaKeys::GainDbi),
                    QString::fromLatin1(SchemaKeys::LocationOffset),
                    QString::fromLatin1(SchemaKeys::CenterFrequencyGHz),
                    QString::fromLatin1(SchemaKeys::BandwidthMHz),
                    QString::fromLatin1(SchemaKeys::SensitivityDbm),
                    QString::fromLatin1(SchemaKeys::InterferenceMarginDb),
                    QString::fromLatin1(SchemaKeys::SinrMarginDb),
                    QString::fromLatin1(SchemaKeys::NoiseFigureDb),
                },
                context,
                errorMessage)) {
            return false;
        }

        if (!readRequiredString(receiverObject, SchemaKeys::ID, equipmentId, errorMessage)) {
            return false;
        }

        QString type;
        if (!readRequiredString(receiverObject, SchemaKeys::Type, type, errorMessage)) {
            return false;
        }
        if (type != QString::fromLatin1(SchemaValues::Receiver)) {
            errorMessage = QStringLiteral("%1.type 必须为 %2")
                               .arg(context, QString::fromLatin1(SchemaValues::Receiver));
            return false;
        }

        equipmentData.equipmentId = equipmentId;
        equipmentData.equipmentType = QString::fromLatin1(SchemaValues::Receiver);

        return parseSharedDeviceFields(receiverObject, equipmentData, errorMessage) &&
               readRequiredNumber(receiverObject, SchemaKeys::CenterFrequencyGHz, equipmentData.receiverCenterFrequencyGHz, errorMessage) &&
               readRequiredNumber(receiverObject, SchemaKeys::BandwidthMHz, equipmentData.receiverBandwidthMHz, errorMessage) &&
               readRequiredNumber(receiverObject, SchemaKeys::SensitivityDbm, equipmentData.receiverSensitivityDbm, errorMessage) &&
               readRequiredNumber(receiverObject, SchemaKeys::InterferenceMarginDb, equipmentData.receiverInterferenceMarginDb, errorMessage) &&
               readRequiredNumber(receiverObject, SchemaKeys::SinrMarginDb, equipmentData.receiverSinrMarginDb, errorMessage) &&
               readRequiredNumber(receiverObject, SchemaKeys::NoiseFigureDb, equipmentData.receiverNoiseFigureDb, errorMessage);
    }

    static bool parseTransceiver(
        const QJsonObject& transceiverObject,
        EquipmentData& equipmentData,
        QString& equipmentId,
        const QString& context,
        QString& errorMessage) {
        if (!validateAllowedKeys(
                transceiverObject,
                {
                    QString::fromLatin1(SchemaKeys::ID),
                    QString::fromLatin1(SchemaKeys::Type),
                    QString::fromLatin1(SchemaKeys::GainDbi),
                    QString::fromLatin1(SchemaKeys::LocationOffset),
                    QString::fromLatin1(SchemaKeys::TransmitterConfig),
                    QString::fromLatin1(SchemaKeys::ReceiverConfig),
                },
                context,
                errorMessage)) {
            return false;
        }

        if (!readRequiredString(transceiverObject, SchemaKeys::ID, equipmentId, errorMessage)) {
            return false;
        }

        QString type;
        if (!readRequiredString(transceiverObject, SchemaKeys::Type, type, errorMessage)) {
            return false;
        }
        if (type != QString::fromLatin1(SchemaValues::Transceiver)) {
            errorMessage = QStringLiteral("%1.type 必须为 %2")
                               .arg(context, QString::fromLatin1(SchemaValues::Transceiver));
            return false;
        }

        QJsonObject transmitterConfig;
        if (!readRequiredObject(transceiverObject, SchemaKeys::TransmitterConfig, transmitterConfig, errorMessage)) {
            return false;
        }

        QJsonObject receiverConfig;
        if (!readRequiredObject(transceiverObject, SchemaKeys::ReceiverConfig, receiverConfig, errorMessage)) {
            return false;
        }

        equipmentData.equipmentId = equipmentId;
        equipmentData.equipmentType = QString::fromLatin1(SchemaValues::Transceiver);

        return parseSharedDeviceFields(transceiverObject, equipmentData, errorMessage) &&
               parseTransmitterConfig(
                   transmitterConfig,
                   equipmentData,
                   context + QStringLiteral(".") + QString::fromLatin1(SchemaKeys::TransmitterConfig),
                   errorMessage) &&
               parseReceiverConfig(
                   receiverConfig,
                   equipmentData,
                   context + QStringLiteral(".") + QString::fromLatin1(SchemaKeys::ReceiverConfig),
                   errorMessage);
    }

    static bool parsePoint3D(
        const QJsonObject& pointObject,
        double& x,
        double& y,
        double& z,
        const QString& context,
        QString& errorMessage) {
        if (!validateAllowedKeys(
                pointObject,
                {
                    QString::fromLatin1(SchemaKeys::Type),
                    QString::fromLatin1(SchemaKeys::Coordinates),
                },
                context,
                errorMessage)) {
            return false;
        }

        QString pointType;
        if (!readRequiredString(pointObject, SchemaKeys::Type, pointType, errorMessage)) {
            return false;
        }
        if (pointType != QString::fromLatin1(SchemaValues::Point3D)) {
            errorMessage = QStringLiteral("%1.type 必须为 %2")
                               .arg(context, QString::fromLatin1(SchemaValues::Point3D));
            return false;
        }

        return readRequiredVector3(pointObject, SchemaKeys::Coordinates, x, y, z, errorMessage);
    }

    static bool validateAllowedKeys(
        const QJsonObject& object,
        const QStringList& allowedKeys,
        const QString& context,
        QString& errorMessage) {
        for (auto it = object.begin(); it != object.end(); ++it) {
            if (!allowedKeys.contains(it.key())) {
                errorMessage = QStringLiteral("%1 包含未定义字段: %2").arg(context, it.key());
                return false;
            }
        }
        return true;
    }

    static bool validateEnumValue(
        const QString& value,
        const QStringList& allowedValues,
        const QString& context,
        QString& errorMessage) {
        if (!allowedValues.contains(value)) {
            errorMessage = QStringLiteral("%1 的枚举值非法: %2").arg(context, value);
            return false;
        }
        return true;
    }

    static bool readRequiredObject(
        const QJsonObject& object,
        const char* key,
        QJsonObject& result,
        QString& errorMessage) {
        const QString keyString = QString::fromLatin1(key);
        if (!object.contains(keyString) || !object.value(keyString).isObject()) {
            errorMessage = QStringLiteral("%1 必须存在且类型为 object").arg(keyString);
            return false;
        }
        result = object.value(keyString).toObject();
        return true;
    }

    static bool readRequiredArray(
        const QJsonObject& object,
        const char* key,
        QJsonArray& result,
        QString& errorMessage) {
        const QString keyString = QString::fromLatin1(key);
        if (!object.contains(keyString) || !object.value(keyString).isArray()) {
            errorMessage = QStringLiteral("%1 必须存在且类型为 array").arg(keyString);
            return false;
        }
        result = object.value(keyString).toArray();
        return true;
    }

    static bool readRequiredString(
        const QJsonObject& object,
        const char* key,
        QString& result,
        QString& errorMessage) {
        const QString keyString = QString::fromLatin1(key);
        if (!object.contains(keyString) || !object.value(keyString).isString()) {
            errorMessage = QStringLiteral("%1 必须存在且类型为 string").arg(keyString);
            return false;
        }

        result = object.value(keyString).toString().trimmed();
        if (result.isEmpty()) {
            errorMessage = QStringLiteral("%1 不能为空字符串").arg(keyString);
            return false;
        }
        return true;
    }

    static bool readRequiredNumber(
        const QJsonObject& object,
        const char* key,
        double& result,
        QString& errorMessage) {
        const QString keyString = QString::fromLatin1(key);
        if (!object.contains(keyString) || !object.value(keyString).isDouble()) {
            errorMessage = QStringLiteral("%1 必须存在且类型为 number").arg(keyString);
            return false;
        }

        result = object.value(keyString).toDouble();
        return true;
    }

    static bool readRequiredInteger(
        const QJsonObject& object,
        const char* key,
        int& result,
        QString& errorMessage) {
        double rawValue = 0.0;
        if (!readRequiredNumber(object, key, rawValue, errorMessage)) {
            return false;
        }

        if (std::floor(rawValue) != rawValue) {
            errorMessage = QStringLiteral("%1 必须为整数").arg(QString::fromLatin1(key));
            return false;
        }
        if (rawValue < static_cast<double>(std::numeric_limits<int>::min()) ||
            rawValue > static_cast<double>(std::numeric_limits<int>::max())) {
            errorMessage = QStringLiteral("%1 超出 int 范围").arg(QString::fromLatin1(key));
            return false;
        }

        result = static_cast<int>(rawValue);
        return true;
    }

    static bool readRequiredVector3(
        const QJsonObject& object,
        const char* key,
        double& x,
        double& y,
        double& z,
        QString& errorMessage) {
        QJsonArray array;
        if (!readRequiredArray(object, key, array, errorMessage)) {
            return false;
        }
        if (array.size() != 3) {
            errorMessage = QStringLiteral("%1 必须是长度为 3 的 number 数组").arg(QString::fromLatin1(key));
            return false;
        }
        if (!array.at(0).isDouble() || !array.at(1).isDouble() || !array.at(2).isDouble()) {
            errorMessage = QStringLiteral("%1 的三个元素都必须是 number").arg(QString::fromLatin1(key));
            return false;
        }

        x = array.at(0).toDouble();
        y = array.at(1).toDouble();
        z = array.at(2).toDouble();
        return true;
    }
};
