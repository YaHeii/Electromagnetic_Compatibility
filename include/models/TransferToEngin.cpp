#include "TransferToEngin.h"
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

std::unique_ptr<ship> TransferToEngine::convertShipDataToShip(const ShipData& shipData, const QList<DeviceData>& allDevices) {
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
                    equipment->setRelativePosition(relativePos);
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
    
    // 根据设备类型创建不同的设备对象
    if (deviceData.equipmentType == "通用设备") {
        Point2D position{deviceData.X_offset, deviceData.Y_offset};
        equipment = std::make_unique<Equipment>(
            deviceData.equipmentID.toStdString(),
            EquipmentType::GENERIC,
            position
        );
    } 
    else if (deviceData.equipmentType == "发射机") {
        Point2D position{deviceData.X_offset, deviceData.Y_offset};
        equipment = std::make_unique<Transmitter>(
            deviceData.equipmentID.toStdString(),
            deviceData.transmitterFrequency, // 使用新添加的频率字段
            deviceData.transmitterPower,
            deviceData.transmitterBandwidth,
            position
        );
    } 
    else if (deviceData.equipmentType == "接收机") {
        Point2D position{deviceData.X_offset, deviceData.Y_offset};
        equipment = std::make_unique<Receiver>(
            deviceData.equipmentID.toStdString(),
            deviceData.recieverFrequency, // 使用新添加的频率字段
            deviceData.recieverSensitive,
            deviceData.recieverBandwidth,
            deviceData.reciever_TransmiterID.toStdString(),
            "", // transmitter_in_ship_id 需要确定如何获取
            deviceData.noiseFigure,
            deviceData.SNRMargin,
            deviceData.interferenceMargin,
            position
        );
    } 
    else if (deviceData.equipmentType == "收发一体机") {
        Point2D position{deviceData.X_offset, deviceData.Y_offset};
        // 对于收发一体机，可能需要创建特殊的设备类型或分别创建发射和接收部分
        equipment = std::make_unique<Equipment>(
            deviceData.equipmentID.toStdString(),
            EquipmentType::TRANSCEIVER,
            position
        );
        // 注意：收发一体机需要同时具备发射和接收功能，但当前的DataModel不支持存储这些信息
        // 可以考虑扩展DeviceData结构以支持收发一体机的参数
    }
    
    // 为设备添加天线
    if (equipment) {
        std::unique_ptr<Antenna> antenna = createAntennaForEquipment(deviceData);
        if (antenna) {
            equipment->setAntenna(std::move(antenna));
        }
    }
    
    return equipment;
}

std::unique_ptr<Antenna> TransferToEngine::createAntennaForEquipment(const DeviceData& deviceData) {
    std::unique_ptr<Antenna> antenna = nullptr;
    
    if (deviceData.antennaType == "全向天线") {
        antenna = std::make_unique<OmniAntenna>(
            ("Antenna_" + deviceData.equipmentID).toStdString(),
            deviceData.Gain
        );
    } 
    else if (deviceData.antennaType == "定向天线") {
        antenna = std::make_unique<DirectionalAntenna>(
            ("Antenna_" + deviceData.equipmentID).toStdString(),
            deviceData.Gain,
            deviceData.antennaTheta,
            deviceData.antennaPhi
        );
    }
    
    return antenna;
}