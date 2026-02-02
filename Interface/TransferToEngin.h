#pragma once


#include "Models/fleet.h"
#include "Models/ship.h"
#include "Models/Equipment.h"
#include "Models/Antenna.h"
#include "Interface/DataModel.h"
#include <memory>

/**
    * @brief 数据转换类，用于将前端UI数据模型转换为后端核心算法所需的数据结构
    * 该类提供了一组静态方法，用于将DataModel中的数据转换为Fleet、ship、Equipment等
    * 核心算法所需的对象。确保前后端数据类型匹配，避免精度损失。
    */
class TransferToEngine {
public:
/**
 * @brief 将DataModel转换为Fleet对象
 * @param dataModel 前端数据模型指针
 * @return 转换后的Fleet对象唯一指针，如果输入为空则返回nullptr
 */
  // 使用数据快照来返回fleet，目前仅有仿真过程需要fleet
  //TODO: 未来可扩展为直接传入DataModel*
    static std::unique_ptr<Fleet> convertDataModelToFleet(const DataModel::DataSnapshot& dataSnapshot);
    
private:
    /**
     * @brief 将ShipData转换为ship对象
     * @param shipData 船只数据
	 * @param equipMap 设备ID到设备数据的映射表
     * @param allDevices 所有设备数据列表（用于查找船上设备的详细信息）
     * @return 转换后的ship对象唯一指针
     */
    static std::unique_ptr<ship> convertShipDataToShip(
        const ShipData& shipData, 
        const std::unordered_map<std::string, const EquipmentData*>& equipMap);
    
    /**
     * @brief 将DeviceData转换为Equipment对象
     * @param deviceData 设备数据
     * @return 转换后的Equipment对象唯一指针
     */
    static std::unique_ptr<Equipment> convertDeviceDataToEquipment(
        const EquipmentData& equipmentData);


    // --- 辅助转换函数 (String -> Enum) ---
    static AntennaType stringToAntennaType(const QString& typeStr);
    static PolarizationMethod stringToPolarization(const QString& polStr);
};
