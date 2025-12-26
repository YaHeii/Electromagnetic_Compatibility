#include "../include/core/ship.h"
#include "../include/core/Equipment.h"
#include "../include/core/Antenna.h"
#include "../../include/utils/point_2D.h"
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>


ship::ship(std::string id,Point2D location, std::vector<double> distance,double orientation_deg,double speed)
: _id(id), // 使用初始化列表初始化 m_location  
_location(location), // 使用初始化列表初始化 m_location  
_distance(distance),  // 初始化 m_distance
_orientation_deg(orientation_deg),//初始化穿的朝向
_speed(speed)//初始化船速  
{}

//查找设备
Equipment* ship::findEquipmentByID(const std::string& eq_id) const {
        for (const auto& eq_ptr : _equipmentList) {
            if (eq_ptr->getID() == eq_id) {
                return eq_ptr.get();
            }
        }
        return nullptr;
    }
//添加设备
void ship::addEquipment(std::unique_ptr<Equipment> eq) {
        _equipmentList.push_back(std::move(eq));
    }

//获取设备列表
const std::vector<std::unique_ptr<Equipment>>& ship::getEquipmentList(){
        return _equipmentList;
    }
    //设置坐标
void ship::setLocation(Point2D position_new) {
    _location = position_new;
}


