#pragma once

#include <QObject>
#include <QString>

#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "Interface/SchemaConstants.h"

inline constexpr int X_MIN = -50000;
inline constexpr int X_MAX = 50000;
inline constexpr int Y_MIN = -50000;
inline constexpr int Y_MAX = 50000;
inline constexpr int Z_MIN = 0;
inline constexpr int Z_MAX = 50000;

namespace DataModelSchemaValues {

inline QString transmitterType() {
    return QString::fromLatin1(SchemaValues::Transmitter);
}

inline QString receiverType() {
    return QString::fromLatin1(SchemaValues::Receiver);
}

inline QString transceiverType() {
    return QString::fromLatin1(SchemaValues::Transceiver);
}

inline QString verticalPolarization() {
    return QString::fromLatin1(SchemaValues::Vertical);
}

inline QString horizontalPolarization() {
    return QString::fromLatin1(SchemaValues::Horizontal);
}

inline QString omniAntennaType() {
    return QString::fromLatin1(SchemaValues::Omni);
}

inline bool isPolarizationValue(const QString& value) {
    return value == verticalPolarization() || value == horizontalPolarization();
}

inline bool isAntennaTypeValue(const QString& value) {
    return value == QString::fromLatin1(SchemaValues::Omni) ||
           value == QString::fromLatin1(SchemaValues::Directional) ||
           value == QString::fromLatin1(SchemaValues::Horn) ||
           value == QString::fromLatin1(SchemaValues::ShapedBeam) ||
           value == QString::fromLatin1(SchemaValues::Reflector);
}

inline bool supportsTransmitterRole(const QString& value) {
    return value == transmitterType() || value == transceiverType();
}

inline bool supportsReceiverRole(const QString& value) {
    return value == receiverType() || value == transceiverType();
}

}  // namespace DataModelSchemaValues

struct EquipmentData {
    QString equipmentId;
    QString equipmentType;
    double gainDbi = 0.0;

    double offsetX = 0.0;
    double offsetY = 0.0;
    double offsetZ = 0.0;

    double receiverCenterFrequencyGHz = 0.0;
    double receiverBandwidthMHz = 0.0;
    double receiverSensitivityDbm = -100.0;
    double receiverInterferenceMarginDb = 0.0;
    double receiverSinrMarginDb = 0.0;
    double receiverNoiseFigureDb = 3.0;

    double transmitterCenterFrequencyGHz = 0.0;
    double transmitterBandwidthMHz = 0.0;
    double transmitterPowerDbm = 0.0;
    double transmitterAntennaPhiDeg = 0.0;
    double transmitterBeamWidthDeg = 0.0;
    QString transmitterPolarization = DataModelSchemaValues::verticalPolarization();
    QString transmitterAntennaType = DataModelSchemaValues::omniAntennaType();

    double antennaCenterFrequencyGHz = 0.0;
    double antennaBandwidthMHz = 0.0;
    double antennaPowerDbm = 0.0;
    double antennaPhiDeg = 0.0;
    double antennaBeamWidthDeg = 0.0;
    QString antennaPolarization = DataModelSchemaValues::verticalPolarization();
    QString antennaType = DataModelSchemaValues::omniAntennaType();

    std::pair<bool, QString> validateEquipmentBaseInfo() const {
        if (equipmentId.trimmed().isEmpty()) {
            return {false, QStringLiteral("设备 ID 不能为空")};
        }

        const auto inRange = [](double value, double minValue, double maxValue) {
            return value >= minValue && value <= maxValue;
        };

        if (!inRange(offsetX, X_MIN, X_MAX) ||
            !inRange(offsetY, Y_MIN, Y_MAX) ||
            !inRange(offsetZ, Z_MIN, Z_MAX)) {
            return {false, QStringLiteral("设备相对坐标超出当前场景边界")};
        }

        return {true, QString()};
    }

    std::pair<bool, QString> validateReceiver() const {
        const auto baseResult = validateEquipmentBaseInfo();
        if (!baseResult.first) {
            return baseResult;
        }

        if (receiverCenterFrequencyGHz <= 0.0) {
            return {false, QStringLiteral("接收机中心频率必须大于 0 GHz")};
        }
        if (receiverBandwidthMHz <= 0.0) {
            return {false, QStringLiteral("接收机带宽必须大于 0 MHz")};
        }
        if (receiverSensitivityDbm >= 0.0) {
            return {false, QStringLiteral("接收机灵敏度必须为负值 dBm")};
        }
        if (receiverNoiseFigureDb < 0.0) {
            return {false, QStringLiteral("接收机噪声系数不能为负值")};
        }

        return {true, QString()};
    }

    std::pair<bool, QString> validateTransmitter() const {
        const auto baseResult = validateEquipmentBaseInfo();
        if (!baseResult.first) {
            return baseResult;
        }

        if (transmitterCenterFrequencyGHz <= 0.0) {
            return {false, QStringLiteral("发射机中心频率必须大于 0 GHz")};
        }
        if (transmitterBandwidthMHz <= 0.0) {
            return {false, QStringLiteral("发射机带宽必须大于 0 MHz")};
        }
        if (transmitterAntennaPhiDeg < 0.0 || transmitterAntennaPhiDeg > 180.0) {
            return {false, QStringLiteral("发射机下倾角必须位于 [0, 180]")};
        }
        if (transmitterBeamWidthDeg < 0.0 || transmitterBeamWidthDeg > 360.0) {
            return {false, QStringLiteral("发射机波束宽度必须位于 [0, 360]")};
        }
        if (!DataModelSchemaValues::isPolarizationValue(transmitterPolarization)) {
            return {false, QStringLiteral("发射机极化方式必须为 schema 定义枚举")};
        }
        if (!DataModelSchemaValues::isAntennaTypeValue(transmitterAntennaType)) {
            return {false, QStringLiteral("发射机天线类型必须为 schema 定义枚举")};
        }

        return {true, QString()};
    }

    std::pair<bool, QString> validate() const {
        if (equipmentType == DataModelSchemaValues::receiverType()) {
            return validateReceiver();
        }
        if (equipmentType == DataModelSchemaValues::transmitterType()) {
            return validateTransmitter();
        }
        if (equipmentType == DataModelSchemaValues::transceiverType()) {
            const auto transmitterResult = validateTransmitter();
            if (!transmitterResult.first) {
                return transmitterResult;
            }
            return validateReceiver();
        }

        return {false, QStringLiteral("未知设备类型")};
    }

    bool operator==(const EquipmentData& other) const {
        return equipmentId == other.equipmentId &&
               equipmentType == other.equipmentType &&
               gainDbi == other.gainDbi &&
               offsetX == other.offsetX &&
               offsetY == other.offsetY &&
               offsetZ == other.offsetZ &&
               receiverCenterFrequencyGHz == other.receiverCenterFrequencyGHz &&
               receiverBandwidthMHz == other.receiverBandwidthMHz &&
               receiverSensitivityDbm == other.receiverSensitivityDbm &&
               receiverInterferenceMarginDb == other.receiverInterferenceMarginDb &&
               receiverSinrMarginDb == other.receiverSinrMarginDb &&
               receiverNoiseFigureDb == other.receiverNoiseFigureDb &&
               transmitterCenterFrequencyGHz == other.transmitterCenterFrequencyGHz &&
               transmitterBandwidthMHz == other.transmitterBandwidthMHz &&
               transmitterPowerDbm == other.transmitterPowerDbm &&
               transmitterAntennaPhiDeg == other.transmitterAntennaPhiDeg &&
               transmitterBeamWidthDeg == other.transmitterBeamWidthDeg &&
               transmitterPolarization == other.transmitterPolarization &&
               transmitterAntennaType == other.transmitterAntennaType &&
               antennaCenterFrequencyGHz == other.antennaCenterFrequencyGHz &&
               antennaBandwidthMHz == other.antennaBandwidthMHz &&
               antennaPowerDbm == other.antennaPowerDbm &&
               antennaPhiDeg == other.antennaPhiDeg &&
               antennaBeamWidthDeg == other.antennaBeamWidthDeg &&
               antennaPolarization == other.antennaPolarization &&
               antennaType == other.antennaType;
    }
};

struct EquipmentOnShip {
    QString equipmentId;
    bool isEnabled = true;

    bool operator==(const EquipmentOnShip& other) const {
        return equipmentId == other.equipmentId && isEnabled == other.isEnabled;
    }
};

struct ShipData {
    std::string shipId;
    double worldX = 0.0;
    double worldY = 0.0;
    double worldZ = 0.0;
    double shipOrientationDeg = 0.0;
    double shipSpeedMps = 0.0;
    std::vector<EquipmentOnShip> equipmentRefs;

    std::pair<bool, QString> validateShip() const {
        if (shipId.empty()) {
            return {false, QStringLiteral("船只 ID 不能为空")};
        }
        if (shipSpeedMps < 0.0) {
            return {false, QStringLiteral("船速不能为负值")};
        }

        const auto inRange = [](double value, double minValue, double maxValue) {
            return value >= minValue && value <= maxValue;
        };

        if (!inRange(worldX, X_MIN, X_MAX) ||
            !inRange(worldY, Y_MIN, Y_MAX) ||
            !inRange(worldZ, Z_MIN, Z_MAX)) {
            return {false, QStringLiteral("船只世界坐标超出当前场景边界")};
        }

        if (shipOrientationDeg < 0.0 || shipOrientationDeg > 360.0) {
            return {false, QStringLiteral("船只朝向角必须位于 [0, 360]")};
        }

        return {true, QString()};
    }

    bool operator==(const ShipData& other) const {
        return shipId == other.shipId &&
               worldX == other.worldX &&
               worldY == other.worldY &&
               worldZ == other.worldZ &&
               shipOrientationDeg == other.shipOrientationDeg &&
               shipSpeedMps == other.shipSpeedMps &&
               equipmentRefs == other.equipmentRefs;
    }
};

struct EnvironmentData {
    // 作为空模型或 UI 初始态的回退值，正式输入应来自 JSON 或环境配置界面。
    double maxRange = 2000.0;
    double ductHeight = 20.0;
    double windSpeed = 7.0;
    double dx = 5.0;
    double dz = 0.1;
    int nz = 2048;
    int angleStepDeg = 5;

    std::pair<bool, QString> validateEnvironmentConfig() const {
        if (maxRange <= 0.0) {
            return {false, QStringLiteral("最大传播距离必须大于 0")};
        }
        if (ductHeight < 0.0) {
            return {false, QStringLiteral("蒸发波导高度不能为负值")};
        }
        if (windSpeed < 0.0) {
            return {false, QStringLiteral("风速不能为负值")};
        }
        if (dx <= 0.0) {
            return {false, QStringLiteral("水平步进必须大于 0")};
        }
        if (dz <= 0.0) {
            return {false, QStringLiteral("垂直分辨率必须大于 0")};
        }
        if (nz <= 0) {
            return {false, QStringLiteral("垂直网格数量必须为正整数")};
        }
        if (angleStepDeg <= 0 || angleStepDeg > 360) {
            return {false, QStringLiteral("角度步进必须位于 [1, 360]")};
        }

        return {true, QString()};
    }

    bool operator==(const EnvironmentData& other) const {
        return maxRange == other.maxRange &&
               ductHeight == other.ductHeight &&
               windSpeed == other.windSpeed &&
               dx == other.dx &&
               dz == other.dz &&
               nz == other.nz &&
               angleStepDeg == other.angleStepDeg;
    }
};

struct EMCAnalysisConfig {
    double fieldPlaneHeightM = 25.0;
    QString referenceTransmitterId;
    QString referenceReceiverId;
    double s3iBaselineWindSpeedMps = 0.5;

    std::pair<bool, QString> validateBasic() const {
        if (fieldPlaneHeightM <= 0.0) {
            return {false, QStringLiteral("fieldPlaneHeightM 必须 > 0")};
        }
        if (referenceTransmitterId.trimmed().isEmpty()) {
            return {false, QStringLiteral("referenceTransmitterId 不能为空")};
        }
        if (referenceReceiverId.trimmed().isEmpty()) {
            return {false, QStringLiteral("referenceReceiverId 不能为空")};
        }
        if (s3iBaselineWindSpeedMps < 0.0) {
            return {false, QStringLiteral("s3iBaselineWindSpeedMps 必须 >= 0")};
        }

        return {true, QString()};
    }

    bool operator==(const EMCAnalysisConfig& other) const {
        return fieldPlaneHeightM == other.fieldPlaneHeightM &&
               referenceTransmitterId == other.referenceTransmitterId &&
               referenceReceiverId == other.referenceReceiverId &&
               s3iBaselineWindSpeedMps == other.s3iBaselineWindSpeedMps;
    }
};

class DataModel : public QObject {
    Q_OBJECT

public:
    struct DataSnapshot {
        std::vector<EquipmentData> allEquipments;
        std::vector<ShipData> allShips;
        EnvironmentData environmentConfig;
        EMCAnalysisConfig emcAnalysisConfig;

        bool operator==(const DataSnapshot& other) const {
            return allEquipments == other.allEquipments &&
                   allShips == other.allShips &&
                   environmentConfig == other.environmentConfig &&
                   emcAnalysisConfig == other.emcAnalysisConfig;
        }

        bool operator!=(const DataSnapshot& other) const {
            return !(*this == other);
        }
    };

    static DataModel* instance() {
        static DataModel instance;
        return &instance;
    }

    static std::pair<bool, QString> validateSnapshot(const DataSnapshot& snapshot) {
        const auto environmentResult = snapshot.environmentConfig.validateEnvironmentConfig();
        if (!environmentResult.first) {
            return environmentResult;
        }

        const auto analysisResult = snapshot.emcAnalysisConfig.validateBasic();
        if (!analysisResult.first) {
            return analysisResult;
        }

        const double maxFieldHeightM =
            snapshot.environmentConfig.dz * static_cast<double>(snapshot.environmentConfig.nz - 1);
        if (snapshot.emcAnalysisConfig.fieldPlaneHeightM > maxFieldHeightM) {
            return {
                false,
                QStringLiteral("fieldPlaneHeightM 超出当前垂直网格限制：%1")
                    .arg(QString::number(maxFieldHeightM))};
        }

        if (snapshot.allShips.empty()) {
            return {false, QStringLiteral("至少需要一艘船只")};
        }

        std::unordered_set<std::string> equipmentIds;
        for (const auto& equipment : snapshot.allEquipments) {
            const auto equipmentResult = equipment.validate();
            if (!equipmentResult.first) {
                return equipmentResult;
            }

            const std::string equipmentId = equipment.equipmentId.toStdString();
            if (!equipmentIds.insert(equipmentId).second) {
                return {false, QStringLiteral("设备 ID 重复: %1").arg(equipment.equipmentId)};
            }
        }

        std::unordered_set<std::string> shipIds;
        for (const auto& ship : snapshot.allShips) {
            const auto shipResult = ship.validateShip();
            if (!shipResult.first) {
                return shipResult;
            }

            if (!shipIds.insert(ship.shipId).second) {
                return {false, QStringLiteral("船只 ID 重复: %1").arg(QString::fromStdString(ship.shipId))};
            }

            std::unordered_set<std::string> shipEquipmentRefs;
            for (const auto& equipmentRef : ship.equipmentRefs) {
                if (equipmentRef.equipmentId.trimmed().isEmpty()) {
                    return {false, QStringLiteral("船只挂载设备引用不能为空")};
                }

                const std::string equipmentId = equipmentRef.equipmentId.toStdString();
                if (equipmentIds.find(equipmentId) == equipmentIds.end()) {
                    return {false, QStringLiteral("船只引用了不存在的设备 ID: %1").arg(equipmentRef.equipmentId)};
                }
                if (!shipEquipmentRefs.insert(equipmentId).second) {
                    return {false, QStringLiteral("同一船只内重复引用设备 ID: %1").arg(equipmentRef.equipmentId)};
                }
            }
        }

        struct ResolvedEquipmentRef {
            const EquipmentData* equipment{nullptr};
            const ShipData* ship{nullptr};
        };

        const auto resolveEnabledEquipment = [&](const QString& equipmentId) -> ResolvedEquipmentRef {
            for (const auto& ship : snapshot.allShips) {
                for (const auto& equipmentRef : ship.equipmentRefs) {
                    if (equipmentRef.equipmentId != equipmentId || !equipmentRef.isEnabled) {
                        continue;
                    }

                    for (const auto& equipment : snapshot.allEquipments) {
                        if (equipment.equipmentId == equipmentId) {
                            return {&equipment, &ship};
                        }
                    }
                }
            }
            return {};
        };

        const ResolvedEquipmentRef referenceTransmitter =
            resolveEnabledEquipment(snapshot.emcAnalysisConfig.referenceTransmitterId);
        if (referenceTransmitter.equipment == nullptr) {
            return {false, QStringLiteral("referenceTransmitterId 必须解析为已启用的设备")};
        }
        if (!DataModelSchemaValues::supportsTransmitterRole(referenceTransmitter.equipment->equipmentType)) {
            return {false, QStringLiteral("referenceTransmitterId 必须指向发射器或收发器")};
        }

        const ResolvedEquipmentRef referenceReceiver =
            resolveEnabledEquipment(snapshot.emcAnalysisConfig.referenceReceiverId);
        if (referenceReceiver.equipment == nullptr) {
            return {false, QStringLiteral("referenceReceiverId 必须解析为已启用的设备")};
        }
        if (!DataModelSchemaValues::supportsReceiverRole(referenceReceiver.equipment->equipmentType)) {
            return {false, QStringLiteral("referenceReceiverId 必须指向接收器或收发器")};
        }

        if (referenceTransmitter.ship == nullptr ||
            referenceReceiver.ship == nullptr ||
            referenceTransmitter.ship->shipId == referenceReceiver.ship->shipId) {
            return {false, QStringLiteral("参考链接必须是跨平台的")};
        }

        return {true, QString()};
    }

    DataSnapshot createSnapshot() const {
        return {allEquipments, allShips, environmentConfig, emcAnalysisConfig};
    }

    std::pair<bool, QString> validateCurrentModel() const {
        return validateSnapshot(createSnapshot());
    }

    const EquipmentData* findEquipmentByID(const QString& id) const {
        for (const auto& equipment : allEquipments) {
            if (equipment.equipmentId == id) {
                return &equipment;
            }
        }
        return nullptr;
    }

    std::vector<EquipmentData> allEquipments;
    std::vector<ShipData> allShips;
    EnvironmentData environmentConfig;
    EMCAnalysisConfig emcAnalysisConfig;

private:
    DataModel() = default;
    ~DataModel() override = default;
    DataModel(const DataModel&) = delete;
    DataModel& operator=(const DataModel&) = delete;
};
