#include "../../include/utils/TransferToEngin.h"
#include <iostream>

std::unique_ptr<Fleet> TransferToEngine::convertDataModelToFleet(const DataModel* dataModel) {
    if (!dataModel) {
        return nullptr;
    }
    
    auto fleet = std::make_unique<Fleet>();
    
    // 遍历所有船只数据并转换
    for (const ShipData& shipData : dataModel->allShips) {
        std::unique_ptr<ship> convertedShip = convertShipDataToShip(shipData, dataModel->allDevices);
        if (convertedShip) {
            fleet->addShip(std::move(convertedShip));
        }
    }
    
    return fleet;
}

std::unique_ptr<ship> TransferToEngine::convertShipDataToShip(const ShipData& shipData, const std::vector<DeviceData>& allDevices) {
    // 创建船只对象
    Point2D location{shipData.ship_X, shipData.ship_Y};
    auto shipObj = std::make_unique<ship>(
        shipData.shipName.toStdString(), 
        location, 
        std::vector<double>(), // distance 暂时为空
        shipData.ship_Orienteation, 
        shipData.ship_Speed
    );
    
    // 添加船只上的设备
    //// TODO:使用双重循环匹配，考虑优化
    for (const DeviceOnShipConfig& deviceConfig : shipData.configuredDevices) {
        // 在设备库中查找对应的设备
        for (const DeviceData& deviceData : allDevices) {
            if (deviceData.equipmentID == deviceConfig.deviceID) {
                std::unique_ptr<Equipment> equipment = convertDeviceDataToEquipment(deviceData);
                if (equipment) {
                    // 设置设备在船上的相对位置
                    Point2D relativePos{deviceConfig.device_X_offset, deviceConfig.device_Y_offset};
                    //equipment->setRelativePosition(relativePos);
                    shipObj->addEquipment(std::move(equipment));
                }
                break;
            }
        }
    }
    return shipObj;
}

std::unique_ptr<Equipment> TransferToEngine::convertDeviceDataToEquipment(const DeviceData& deviceData) {
    std::unique_ptr<Equipment> equipment = nullptr;
    
    if (deviceData.equipmentType == "发射机") {
        Point3D position{deviceData.X_offset, deviceData.Y_offset, deviceData.Z_offset};
        equipment = std::make_unique<Transmitter>(
            deviceData.equipmentID.toStdString(),
            deviceData.CentralF_Transmitter,
            deviceData.Gain,
            deviceData.Bandwidth_Transmitter,
            deviceData.antennaPhi_Transmitter,
            deviceData.Beamwidth_Transmitter,
            deviceData.PolarizationMethod_Transmitter,
            deviceData.antennaType_Transmitter,
            position // 使用新添加的频率字段
        );
    } 
    else if (deviceData.equipmentType == "接收机") {
        Point3D position{deviceData.X_offset, deviceData.Y_offset, deviceData.Z_offset};
        equipment = std::make_unique<Receiver>(
            deviceData.equipmentID.toStdString(),
            deviceData.CentralF_Reciever, // 使用新添加的频率字段
            deviceData.Gain,
            deviceData.Sensitive_reciever,
            deviceData.Bandwidth_Reciever,
            deviceData.noiseFigure,
            deviceData.SINRMargin,
            deviceData.interferenceMargin,
            position
        );
    } 
    else if (deviceData.equipmentType == "收发一体机") {
        Point3D position{deviceData.X_offset, deviceData.Y_offset, deviceData.Z_offset};
        // 对于收发一体机，可能需要创建特殊的设备类型或分别创建发射和接收部分
        equipment = std::make_unique<Transceiver>(
            deviceData.equipmentID.toStdString(),
            deviceData.Gain,
            position,
            // 创建发射部分
            deviceData.CentralF_Transmitter,
            deviceData.Bandwidth_Transmitter,
            deviceData.Power_Transmitter,
            deviceData.antennaPhi_Transmitter,
            deviceData.Beamwidth_Transmitter,
            deviceData.PolarizationMethod_Transmitter,
            deviceData.antennaType_Transmitter,
            // 创建接收部分
            deviceData.CentralF_Reciever,
            deviceData.Bandwidth_Reciever,
            deviceData.Sensitive_reciever,
            deviceData.noiseFigure,
            deviceData.SINRMargin,
            deviceData.interferenceMargin
        );
    }
    return equipment;
}

// 对外工具函数
std::unique_ptr<Antenna> TransferToEngine::createAntenna(const DeviceData& deviceData) {
    std::unique_ptr<Antenna> antenna = nullptr;
    Point3D position{ deviceData.X_offset, deviceData.Y_offset, deviceData.Z_offset };
    if (deviceData.equipmentType == "天线") {
        if (deviceData.antennaType_Antenna == "喇叭天线") {
            antenna = Antenna::create(
                deviceData.equipmentID.toStdString(),
                "喇叭天线",
                deviceData.PolarizationMethod_Antenna,
                position,
                deviceData.Gain,
                deviceData.antennaPhi_Antenna
            );
        }
        if (deviceData.antennaType_Antenna == "赋形波束天线") {
            antenna = Antenna::create(
                deviceData.equipmentID.toStdString(),
                "赋形波束天线",
                deviceData.PolarizationMethod_Antenna,
                position,
                deviceData.Gain,
                deviceData.antennaPhi_Antenna
            );
        }
        if (deviceData.antennaType_Antenna == "抛物面天线") {
            antenna = Antenna::create(
                deviceData.equipmentID.toStdString(),
                "抛物面天线",
                deviceData.PolarizationMethod_Antenna,
                position,
                deviceData.Gain,
                deviceData.antennaPhi_Antenna
            );
        }
    } 
    
    return antenna;
}