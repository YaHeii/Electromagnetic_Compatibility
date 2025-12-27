#include "../../include/utils/TransferToEngin.h"
#include <iostream>
#include <spdlog/spdlog.h>

AntennaType TransferToEngine::stringToAntennaType(const QString& typeStr) {
    if (typeStr == "喇叭天线") return AntennaType::HORN;
    if (typeStr == "赋形波束天线") return AntennaType::ShapedBeam;
    if (typeStr == "抛物面天线") return AntennaType::Reflector;
    return AntennaType::OMNI; // 默认
}

PolarizationMethod TransferToEngine::stringToPolarization(const QString& polStr) {
    if (polStr == "水平极化") return PolarizationMethod::HORIZONTAL;
    return PolarizationMethod::VERTICAL; // 默认
}

std::unique_ptr<Fleet> TransferToEngine::convertDataModelToFleet(const DataModel& dataSnapshot) {
    auto fleet = std::make_unique<Fleet>();

    // 建立哈希索引，避免双重循环查找
    // Key: EquipmentID (std::string), Value: EquipmentData指针
    std::unordered_map<std::string, const EquipmentData*> equipMap;
    for (const auto& eq : dataSnapshot.allEquipments) {
        equipMap[eq.equipmentID.toStdString()] = &eq;
    }

    for (const ShipData& shipData : dataSnapshot.allShips) {
        // 传入 Map 进行快速查找
        std::unique_ptr<ship> convertedShip = convertShipDataToShip(shipData, equipMap);
        if (convertedShip) {
            fleet->addShip(std::move(convertedShip));
        }
    }
    return fleet;
}

std::unique_ptr<ship> TransferToEngine::convertShipDataToShip(
    const ShipData& shipData,
    const std::unordered_map<std::string, const EquipmentData*>& equipMap)
{
    Point3D location{ shipData.X_offset, shipData.Y_offset, shipData.Z_offset };

    auto shipObj = std::make_unique<ship>(
        shipData.shipName.toStdString(),
        location,
        shipData.ship_Orienteation,
        shipData.ship_Speed
    );

    // O(1) 查找设备
    for (const EquipmentOnShip& shipEq : shipData.Equipments) {
        std::string id = shipEq.equipmentID.toStdString();
        auto it = equipMap.find(id);

        if (it != equipMap.end()) {
            // 找到设备数据，开始转换
            std::unique_ptr<Equipment> deviceObj = convertDeviceDataToEquipment(*(it->second));
            if (deviceObj) {
                shipObj->addEquipment(std::move(deviceObj));
            }
        }
        else {
            // 警告船上挂载了不存在的设备
             spdlog::debug("Ship {} has unknown equipment ID: {}", shipData.shipName.toStdString(), id);
        }
    }
    return shipObj;
}

std::unique_ptr<Equipment> TransferToEngine::convertDeviceDataToEquipment(const EquipmentData& d) {
    std::string id = d.equipmentID.toStdString();
    Point3D pos{ d.X_offset, d.Y_offset, d.Z_offset }; // 相对坐标

    // --- 发射机 ---
    if (d.equipmentType == "发射机") {
        TxParams params;
        params._centralF_mhz = d.CentralF_Transmitter;
        params._bandwidth_khz = d.Bandwidth_Transmitter;
        params._power_dbm = d.Power_Transmitter;
        params._antennaPhi = d.antennaPhi_Transmitter;
        params._beamWidth = d.Beamwidth_Transmitter;

        // 转换 Enum
        params._polarization = stringToPolarization(d.PolarizationMethod_Transmitter);
        params._antennaType = stringToAntennaType(d.antennaType_Transmitter);

        return std::make_unique<Transmitter>(id, params, pos);
    }

    // --- 接收机 ---
    else if (d.equipmentType == "接收机") {
        RxParams params;
        params._centralF_mhz = d.CentralF_Reciever;
        params._bandwidth_khz = d.Bandwidth_Reciever;
        params._sensitivity_dbm = d.Sensitive_reciever;
        params._noise_figure_db = d.noiseFigure;
        params._SINR_threshold_db = d.SINRMargin;
        params._interference_threshold_db = d.interferenceMargin;

        return std::make_unique<Receiver>(id, params, "", pos);
    }

    // --- 收发一体机 ---
    else if (d.equipmentType == "收发一体机") {
        // 同时构建两套参数
        TxParams txParams;
        txParams._centralF_mhz = d.CentralF_Transmitter;
        txParams._bandwidth_khz = d.Bandwidth_Transmitter;
        txParams._power_dbm = d.Power_Transmitter;
        txParams._antennaPhi = d.antennaPhi_Transmitter;
        txParams._beamWidth = d.Beamwidth_Transmitter;
        txParams._polarization = stringToPolarization(d.PolarizationMethod_Transmitter);
        txParams._antennaType = stringToAntennaType(d.antennaType_Transmitter);

        RxParams rxParams;
        rxParams._centralF_mhz = d.CentralF_Reciever;
        rxParams._bandwidth_khz = d.Bandwidth_Reciever;
        rxParams._sensitivity_dbm = d.Sensitive_reciever;
        rxParams._noise_figure_db = d.noiseFigure;
        rxParams._SINR_threshold_db = d.SINRMargin;
        rxParams._interference_threshold_db = d.interferenceMargin;

        return std::make_unique<Transceiver>(id, txParams, rxParams, pos);
    }

    // 暂不支持纯天线作为独立 Equipment，或者需要为其定义专门的 Equipment 子类
    return nullptr;
}

// // 对外工具函数
// std::unique_ptr<Antenna> TransferToEngine::createAntenna(const EquipmentData& deviceData) {
//     std::unique_ptr<Antenna> antenna = nullptr;
//     Point3D position{ deviceData.X_offset, deviceData.Y_offset, deviceData.Z_offset };
//     if (deviceData.equipmentType == "天线") {
//         if (deviceData.antennaType_Antenna == "喇叭天线") {
//             antenna = Antenna::create(
//                 deviceData.equipmentID.toStdString(),
//                 "喇叭天线",
//                 deviceData.PolarizationMethod_Antenna,
//                 position,
//                 deviceData.Gain,
//                 deviceData.antennaPhi_Antenna
//             );
//         }
//         if (deviceData.antennaType_Antenna == "赋形波束天线") {
//             antenna = Antenna::create(
//                 deviceData.equipmentID.toStdString(),
//                 "赋形波束天线",
//                 deviceData.PolarizationMethod_Antenna,
//                 position,
//                 deviceData.Gain,
//                 deviceData.antennaPhi_Antenna
//             );
//         }
//         if (deviceData.antennaType_Antenna == "抛物面天线") {
//             antenna = Antenna::create(
//                 deviceData.equipmentID.toStdString(),
//                 "抛物面天线",
//                 deviceData.PolarizationMethod_Antenna,
//                 position,
//                 deviceData.Gain,
//                 deviceData.antennaPhi_Antenna
//             );
//         }
//     } 
//     return antenna;
// }