#include "../include/core/ship.h"
#include "../include/core/Equipment.h"
#include "../include/core/Antenna.h"
#include "../../include/utils/point_2D.h"
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>


ship::ship(string id,Point2D location, vector<double> distance,double orientation_deg,double speed)
: m_id(id), // 使用初始化列表初始化 m_location  
m_location(location), // 使用初始化列表初始化 m_location  
m_distance(distance),  // 初始化 m_distance
m_orientation_deg(orientation_deg),//初始化穿的朝向
m_speed(speed)//初始化船速  
{}

//查找设备
Equipment* ship::findEquipmentByID(const std::string& eq_id) const {
        for (const auto& eq_ptr : m_equipmentList) {
            if (eq_ptr->getID() == eq_id) {
                return eq_ptr.get();
            }
        }
        return nullptr;
    }
//添加设备
void ship::addEquipment(std::unique_ptr<Equipment> eq) {
        m_equipmentList.push_back(std::move(eq));
    }

//获取设备列表
const std::vector<std::unique_ptr<Equipment>>& ship::getEquipmentList(){
        return m_equipmentList;
    }
    //设置坐标
void ship::setLocation(Point2D position_new) {
    m_location = position_new;
}


