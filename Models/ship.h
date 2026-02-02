#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include "Equipment.h"
#include "Utils/point_2D.h"

class Equipment;
/// <summary>
/// @param id 船只标识符
/// @param location 船只位置
/// @param distance 船只间距
/// @param orientation_deg 船体朝向，0度为X轴正向
/// @param speed 船速，单位：m/s  弃用
/// </summary>

class ship{
public:
    ship(std::string id, Point3D location, double orientation_deg = 30.0, double speed = 1.0)
    : _id(id), // 使用初始化列表初始化 m_location  
        _location(location), // 使用初始化列表初始化 m_location  
        _orientation_deg(orientation_deg),//初始化穿的朝向
        _speed(speed) {}   //初始化船速 


    const std::vector<double>& getDistance() const { return _distance; }
    // 返回设备列表的常量引用，避免拷贝唯一所有权指针
    const std::vector<std::unique_ptr<Equipment>>& getEquipmentList() const { return _equipmentList; }
    double getOrientationDeg() const { return _orientation_deg; } // 船体朝向，0度为X轴正向

	// 获取船位置
    Point3D getLocation() const { return _location; }
	// 获取船ID
    const std::string& getID() const { return _id; }
    // 获取船速
    double getSpeed() const { return _speed; }
    // 获取角度
    void setOrientationDeg(double deg) { _orientation_deg = deg; }
    // 设置距离矩阵
	void setDistance(const std::vector<double>& distance) { _distance = distance; }
    //设置船速
    void setSpeed(double speed) { _speed = speed; }


    //添加设备
    void addEquipment(std::unique_ptr<Equipment> eq) {
        _equipmentList.push_back(std::move(eq));
    }

    //获取设备列表
    const std::vector<std::unique_ptr<Equipment>>& getEquipmentList() {
        return _equipmentList;
    }
    //设置坐标
    void setLocation(Point3D position_new) {
        _location = position_new;
    }

    ////查找设备
    //Equipment* findEquipmentByID(const std::string& eq_id) const {
    //    for (const auto& eq_ptr : _equipmentList) {
    //        if (eq_ptr->getID() == eq_id) {
    //            return eq_ptr.get();
    //        }
    //    }
    //    return nullptr;
    //}


private:
    std::string _id;
    Point3D _location;
    std::vector<double> _distance;
    std::vector<std::unique_ptr<Equipment>>_equipmentList;
    double _orientation_deg = 0.0;
    double _speed = 1.0;//弃用
};










