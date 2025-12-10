#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include "Equipment.h"
#include "../utils/point_2D.h"
using namespace std;

/// <summary>
/// @param id 船只标识符
/// @param location 船只位置
/// @param distance 船只间距
/// @param orientation_deg 船体朝向，0度为X轴正向
/// @param speed 船速，单位：m/s  弃用
/// </summary>

class ship{
public:
    ship(string id, Point2D location, vector<double> distance = {}, double orientation_deg = 30.0, double speed = 1.0);

    const std::string& getID() const { return _id; }
    Point2D getLocation() const {return _location; }
    const std::vector<double>& getDistance() const { return _distance; }
	auto getEquipmentList() const { return _equipmentList; }
    double getOrientationDeg() const { return _orientation_deg; } // 船体朝向，0度为X轴正向

    Equipment* findEquipmentByID(const std::string& eq_id) const ;
    //添加设备
    void addEquipment(std::unique_ptr<Equipment> equip0);
    //获取设备列表
    const std::vector<std::unique_ptr<Equipment>>& getEquipmentList();
    //获取角度
    void setOrientationDeg(double deg) { _orientation_deg = deg; }
    //获取船速
    double getSpeed() const { return _speed; }
    //设置船速
    void setSpeed(double speed) { _speed = speed; }
    //设置坐标
    void setLocation(Point2D position_new);

private:
    string _id;
    Point2D _location;
    std::vector<double> _distance;
    std::vector<std::unique_ptr<Equipment>>_equipmentList;
    double _orientation_deg = 0.0;
    double _speed = 1.0;//弃用
};










