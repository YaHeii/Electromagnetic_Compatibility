#pragma once

#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStringList>

#include <cmath>
#include <unordered_set>
#include <vector>

#include "Interface/DataModel.h"
#include "Interface/SchemaConstants.h"
#include "spdlog/spdlog.h"

class JsonLoader {
public:
    static bool LoadFile(const QString& filePath) {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            spdlog::error("cannot open file: {}, {}", filePath.toStdString(), file.errorString().toStdString());
            return false;
        }

        QString jsonString = QString::fromUtf8(file.readAll());
        file.close();

        stripSingleLineComments(jsonString);

        QJsonParseError error;
        const QJsonDocument doc = QJsonDocument::fromJson(jsonString.toUtf8(), &error);
        if (error.error != QJsonParseError::NoError) {
            spdlog::error("JSON 解析错误: {}", error.errorString().toStdString());
            return false;
        }

        if (!doc.isObject()) {
            spdlog::error("JSON 格式错误: 根节点必须是对象");
            return false;
        }

        EnvironmentData environmentConfig;
        std::vector<ShipData> allShips;
        std::vector<EquipmentData> allEquipments;
        QString errorMessage;

        if (!parseRootObject(doc.object(), environmentConfig, allShips, allEquipments, errorMessage)) {
            spdlog::error("Schema 校验失败: {}", errorMessage.toStdString());
            return false;
        }

        DataModel* model = DataModel::instance();
        model->allShips = std::move(allShips);
        model->allEquipments = std::move(allEquipments);
        model->environmentConfig = environmentConfig;

        spdlog::info(
            "成功加载新 schema 配置: {}, 船只数量: {}, 设备数量: {}",
            filePath.toStdString(),
            model->allShips.size(),
            model->allEquipments.size());
        return true;
    }

private:
    static void stripSingleLineComments(QString& jsonString) {
        const QRegularExpression commentPattern("//[^\\n\\r]*");
        jsonString.replace(commentPattern, "");
    }

    static bool parseRootObject(
        const QJsonObject& rootObject,
        EnvironmentData& environmentConfig,
        std::vector<ShipData>& allShips,
        std::vector<EquipmentData>& allEquipments,
        QString& errorMessage) {
        if (!validateAllowedKeys(
                rootObject,
                {
                    SchemaKeys::SchemaVersion,
                    SchemaKeys::Environment,
                    SchemaKeys::Usvs,
                },
                QStringLiteral("根对象"),
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
        if (!parseEnvironment(environmentObject, environmentConfig, errorMessage)) {
            return false;
        }

        QJsonArray usvArray;
        if (!readRequiredArray(rootObject, SchemaKeys::Usvs, usvArray, errorMessage)) {
            return false;
        }
        if (usvArray.isEmpty()) {
            errorMessage = QStringLiteral("usvs 数组不能为空");
            return false;
        }

        std::unordered_set<std::string> shipIds;
        std::unordered_set<std::string> equipmentIds;

        for (int index = 0; index < usvArray.size(); ++index) {
            if (!usvArray.at(index).isObject()) {
                errorMessage = QStringLiteral("usvs[%1] 必须是对象").arg(index);
                return false;
            }

            ShipData shipData;
            if (!parseShip(
                    usvArray.at(index).toObject(),
                    index,
                    shipData,
                    allEquipments,
                    shipIds,
                    equipmentIds,
                    errorMessage)) {
                return false;
            }
            allShips.push_back(std::move(shipData));
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
                    SchemaKeys::MaxRange,
                    SchemaKeys::DuctHeight,
                    SchemaKeys::WindSpeed,
                    SchemaKeys::Dx,
                    SchemaKeys::Dz,
                    SchemaKeys::Nz,
                    SchemaKeys::AngleStepDeg,
                },
                QStringLiteral("environment"),
                errorMessage)) {
            return false;
        }

        if (!readRequiredPositiveNumber(environmentObject, SchemaKeys::MaxRange, environmentConfig.max_range, errorMessage)) {
            return false;
        }
        if (!readRequiredNonNegativeNumber(environmentObject, SchemaKeys::DuctHeight, environmentConfig.duct_height, errorMessage)) {
            return false;
        }
        if (!readRequiredNonNegativeNumber(environmentObject, SchemaKeys::WindSpeed, environmentConfig.wind_speed, errorMessage)) {
            return false;
        }
        if (!readRequiredPositiveNumber(environmentObject, SchemaKeys::Dx, environmentConfig.dx, errorMessage)) {
            return false;
        }
        if (!readRequiredPositiveNumber(environmentObject, SchemaKeys::Dz, environmentConfig.dz, errorMessage)) {
            return false;
        }
        if (!readRequiredPositiveInteger(environmentObject, SchemaKeys::Nz, environmentConfig.nz, errorMessage)) {
            return false;
        }
        if (!readRequiredPositiveInteger(environmentObject, SchemaKeys::AngleStepDeg, environmentConfig.angle_step_deg, errorMessage)) {
            return false;
        }
        if (environmentConfig.angle_step_deg > 360) {
            errorMessage = QStringLiteral("angleStepDeg 必须位于 [1, 360] 范围内");
            return false;
        }
        return true;
    }

    static bool parseShip(
        const QJsonObject& shipObject,
        int shipIndex,
        ShipData& shipData,
        std::vector<EquipmentData>& allEquipments,
        std::unordered_set<std::string>& shipIds,
        std::unordered_set<std::string>& equipmentIds,
        QString& errorMessage) {
        const QString shipContext = QStringLiteral("usvs[%1]").arg(shipIndex);

        if (!validateAllowedKeys(
                shipObject,
                {
                    SchemaKeys::ID,
                    SchemaKeys::Location,
                    SchemaKeys::Speed,
                    SchemaKeys::ShipOrientationDeg,
                    SchemaKeys::Transmitters,
                    SchemaKeys::Receivers,
                },
                shipContext,
                errorMessage)) {
            return false;
        }

        QString shipId;
        if (!readRequiredString(shipObject, SchemaKeys::ID, shipId, errorMessage)) {
            return false;
        }

        const std::string shipIdStd = shipId.toStdString();
        if (!shipIds.insert(shipIdStd).second) {
            errorMessage = QStringLiteral("船只 ID 重复: %1").arg(shipId);
            return false;
        }

        shipData.shipID = shipIdStd;

        QJsonObject locationObject;
        if (!readRequiredObject(shipObject, SchemaKeys::Location, locationObject, errorMessage)) {
            return false;
        }
        if (!parsePoint3D(locationObject, shipData.X_offset, shipData.Y_offset, shipData.Z_offset, shipContext + ".location", errorMessage)) {
            return false;
        }

        if (!readRequiredNonNegativeNumber(shipObject, SchemaKeys::Speed, shipData.ship_Speed, errorMessage)) {
            return false;
        }
        if (!readRequiredNumber(shipObject, SchemaKeys::ShipOrientationDeg, shipData.ship_Orienteation, errorMessage)) {
            return false;
        }
        if (shipData.ship_Orienteation < 0.0 || shipData.ship_Orienteation > 360.0) {
            errorMessage = QStringLiteral("%1 必须位于 [0, 360] 范围内")
                               .arg(QString::fromLatin1(SchemaKeys::ShipOrientationDeg));
            return false;
        }

        QJsonArray transmitterArray;
        if (!readRequiredArray(shipObject, SchemaKeys::Transmitters, transmitterArray, errorMessage)) {
            return false;
        }
        if (!parseDeviceArray(
                transmitterArray,
                shipId,
                SchemaDeviceType::Transmitter,
                shipContext + ".transmitters",
                shipData,
                allEquipments,
                equipmentIds,
                errorMessage)) {
            return false;
        }

        QJsonArray receiverArray;
        if (!readRequiredArray(shipObject, SchemaKeys::Receivers, receiverArray, errorMessage)) {
            return false;
        }
        if (!parseDeviceArray(
                receiverArray,
                shipId,
                SchemaDeviceType::Receiver,
                shipContext + ".receivers",
                shipData,
                allEquipments,
                equipmentIds,
                errorMessage)) {
            return false;
        }

        return true;
    }

    static bool parseDeviceArray(
        const QJsonArray& deviceArray,
        const QString& shipId,
        SchemaDeviceType deviceType,
        const QString& context,
        ShipData& shipData,
        std::vector<EquipmentData>& allEquipments,
        std::unordered_set<std::string>& equipmentIds,
        QString& errorMessage) {
        for (int index = 0; index < deviceArray.size(); ++index) {
            if (!deviceArray.at(index).isObject()) {
                errorMessage = QStringLiteral("%1[%2] 必须是对象").arg(context).arg(index);
                return false;
            }

            EquipmentData equipmentData;
            QString equipmentId;
            const QString deviceContext = QStringLiteral("%1[%2]").arg(context).arg(index);
            const bool parsed = (deviceType == SchemaDeviceType::Transmitter)
                                    ? parseTransmitter(deviceArray.at(index).toObject(), equipmentData, equipmentId, deviceContext, errorMessage)
                                    : parseReceiver(deviceArray.at(index).toObject(), equipmentData, equipmentId, deviceContext, errorMessage);
            if (!parsed) {
                return false;
            }

            const std::string equipmentIdStd = equipmentId.toStdString();
            if (!equipmentIds.insert(equipmentIdStd).second) {
                errorMessage = QStringLiteral("设备 ID 重复: %1。当前解析到船只 %2").arg(equipmentId, shipId);
                return false;
            }

            EquipmentOnShip equipmentOnShip;
            equipmentOnShip.equipmentID = equipmentId;
            equipmentOnShip.isEnabled = true;
            shipData.Equipments.push_back(equipmentOnShip);

            allEquipments.push_back(std::move(equipmentData));
        }
        return true;
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
                    SchemaKeys::ID,
                    SchemaKeys::Type,
                    SchemaKeys::GainDbi,
                    SchemaKeys::LocationOffset,
                    SchemaKeys::CenterFrequencyGHz,
                    SchemaKeys::BandwidthMHz,
                    SchemaKeys::PowerDbm,
                    SchemaKeys::AntennaPhiDeg,
                    SchemaKeys::BeamWidthDeg,
                    SchemaKeys::Polarization,
                    SchemaKeys::AntennaType,
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

        equipmentData.equipmentID = equipmentId;
        equipmentData.equipmentType = QStringLiteral("发射机");

        if (!readRequiredNumber(transmitterObject, SchemaKeys::GainDbi, equipmentData.Gain, errorMessage)) {
            return false;
        }
        if (!readRequiredVector3(
                transmitterObject,
                SchemaKeys::LocationOffset,
                equipmentData.X_offset,
                equipmentData.Y_offset,
                equipmentData.Z_offset,
                errorMessage)) {
            return false;
        }
        if (!readRequiredPositiveNumber(transmitterObject, SchemaKeys::CenterFrequencyGHz, equipmentData.CentralF_Transmitter, errorMessage)) {
            return false;
        }
        if (!readRequiredPositiveNumber(transmitterObject, SchemaKeys::BandwidthMHz, equipmentData.Bandwidth_Transmitter, errorMessage)) {
            return false;
        }
        if (!readRequiredNumber(transmitterObject, SchemaKeys::PowerDbm, equipmentData.Power_Transmitter, errorMessage)) {
            return false;
        }
        if (!readRequiredNumber(transmitterObject, SchemaKeys::AntennaPhiDeg, equipmentData.antennaPhi_Transmitter, errorMessage)) {
            return false;
        }
        if (equipmentData.antennaPhi_Transmitter < 0.0 || equipmentData.antennaPhi_Transmitter > 180.0) {
            errorMessage = QStringLiteral("%1.%2 必须位于 [0, 180] 范围内")
                               .arg(context, QString::fromLatin1(SchemaKeys::AntennaPhiDeg));
            return false;
        }
        if (!readRequiredNumber(transmitterObject, SchemaKeys::BeamWidthDeg, equipmentData.Beamwidth_Transmitter, errorMessage)) {
            return false;
        }
        if (equipmentData.Beamwidth_Transmitter < 0.0 || equipmentData.Beamwidth_Transmitter > 360.0) {
            errorMessage = QStringLiteral("%1.%2 必须位于 [0, 360] 范围内")
                               .arg(context, QString::fromLatin1(SchemaKeys::BeamWidthDeg));
            return false;
        }

        QString polarization;
        if (!readRequiredString(transmitterObject, SchemaKeys::Polarization, polarization, errorMessage)) {
            return false;
        }
        if (!validateEnumValue(
                polarization,
                {
                    SchemaValues::Vertical,
                    SchemaValues::Horizontal,
                },
                context + ".polarization",
                errorMessage)) {
            return false;
        }
        equipmentData.PolarizationMethod_Transmitter = mapPolarizationToInternalValue(polarization);

        QString antennaType;
        if (!readRequiredString(transmitterObject, SchemaKeys::AntennaType, antennaType, errorMessage)) {
            return false;
        }
        if (!validateEnumValue(
                antennaType,
                {
                    SchemaValues::Omni,
                    SchemaValues::Directional,
                    SchemaValues::Horn,
                    SchemaValues::ShapedBeam,
                    SchemaValues::Reflector,
                },
                context + ".antennaType",
                errorMessage)) {
            return false;
        }
        equipmentData.antennaType_Transmitter = mapAntennaTypeToInternalValue(antennaType);
        return true;
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
                    SchemaKeys::ID,
                    SchemaKeys::Type,
                    SchemaKeys::GainDbi,
                    SchemaKeys::LocationOffset,
                    SchemaKeys::CenterFrequencyGHz,
                    SchemaKeys::BandwidthMHz,
                    SchemaKeys::SensitivityDbm,
                    SchemaKeys::InterferenceMarginDb,
                    SchemaKeys::SinrMarginDb,
                    SchemaKeys::NoiseFigureDb,
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

        equipmentData.equipmentID = equipmentId;
        equipmentData.equipmentType = QStringLiteral("接收机");

        if (!readRequiredNumber(receiverObject, SchemaKeys::GainDbi, equipmentData.Gain, errorMessage)) {
            return false;
        }
        if (!readRequiredVector3(
                receiverObject,
                SchemaKeys::LocationOffset,
                equipmentData.X_offset,
                equipmentData.Y_offset,
                equipmentData.Z_offset,
                errorMessage)) {
            return false;
        }
        if (!readRequiredPositiveNumber(receiverObject, SchemaKeys::CenterFrequencyGHz, equipmentData.CentralF_Reciever, errorMessage)) {
            return false;
        }
        if (!readRequiredPositiveNumber(receiverObject, SchemaKeys::BandwidthMHz, equipmentData.Bandwidth_Reciever, errorMessage)) {
            return false;
        }
        if (!readRequiredNumber(receiverObject, SchemaKeys::SensitivityDbm, equipmentData.Sensitive_reciever, errorMessage)) {
            return false;
        }
        if (equipmentData.Sensitive_reciever >= 0.0) {
            errorMessage = QStringLiteral("%1.%2 必须为负值")
                               .arg(context, QString::fromLatin1(SchemaKeys::SensitivityDbm));
            return false;
        }
        if (!readRequiredNumber(receiverObject, SchemaKeys::InterferenceMarginDb, equipmentData.interferenceMargin, errorMessage)) {
            return false;
        }
        if (!readRequiredNumber(receiverObject, SchemaKeys::SinrMarginDb, equipmentData.SINRMargin, errorMessage)) {
            return false;
        }
        if (!readRequiredNonNegativeNumber(receiverObject, SchemaKeys::NoiseFigureDb, equipmentData.noiseFigure, errorMessage)) {
            return false;
        }
        return true;
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
                    SchemaKeys::Type,
                    SchemaKeys::Coordinates,
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
            errorMessage = QStringLiteral("%1 的取值非法: %2").arg(context, value);
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

    static bool readRequiredPositiveNumber(
        const QJsonObject& object,
        const char* key,
        double& result,
        QString& errorMessage) {
        if (!readRequiredNumber(object, key, result, errorMessage)) {
            return false;
        }
        if (result <= 0.0) {
            errorMessage = QStringLiteral("%1 必须大于 0").arg(QString::fromLatin1(key));
            return false;
        }
        return true;
    }

    static bool readRequiredNonNegativeNumber(
        const QJsonObject& object,
        const char* key,
        double& result,
        QString& errorMessage) {
        if (!readRequiredNumber(object, key, result, errorMessage)) {
            return false;
        }
        if (result < 0.0) {
            errorMessage = QStringLiteral("%1 不能为负值").arg(QString::fromLatin1(key));
            return false;
        }
        return true;
    }

    static bool readRequiredPositiveInteger(
        const QJsonObject& object,
        const char* key,
        int& result,
        QString& errorMessage) {
        double rawValue = 0.0;
        if (!readRequiredNumber(object, key, rawValue, errorMessage)) {
            return false;
        }
        if (rawValue <= 0.0 || std::floor(rawValue) != rawValue) {
            errorMessage = QStringLiteral("%1 必须为正整数").arg(QString::fromLatin1(key));
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

    static QString mapPolarizationToInternalValue(const QString& polarization) {
        if (polarization == QString::fromLatin1(SchemaValues::Horizontal)) {
            return QStringLiteral("水平极化");
        }
        return QStringLiteral("垂直极化");
    }

    static QString mapAntennaTypeToInternalValue(const QString& antennaType) {
        if (antennaType == QString::fromLatin1(SchemaValues::Directional)) {
            return QStringLiteral("定向天线");
        }
        if (antennaType == QString::fromLatin1(SchemaValues::Horn)) {
            return QStringLiteral("喇叭天线");
        }
        if (antennaType == QString::fromLatin1(SchemaValues::ShapedBeam)) {
            return QStringLiteral("赋形波束天线");
        }
        if (antennaType == QString::fromLatin1(SchemaValues::Reflector)) {
            return QStringLiteral("抛物面天线");
        }
        return QStringLiteral("全向天线");
    }
};
