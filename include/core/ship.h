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
class ship
{
public:
    ship(string id, Point2D location, vector<double> distance = {}, double orientation_deg = 30.0, double speed = 1.0);
    double getOrientationDeg() const { return m_orientation_deg; } // 船体朝向，0度为X轴正向
    const std::string& getID() const { return m_id; }

    Point2D getLocation() const {
    return m_location;
    }
    
    const std::vector<double>& getDistance() const { return m_distance; }
    //查找设备
    Equipment* findEquipmentByID(const std::string& eq_id) const ;
    //添加设备
    void addEquipment(std::unique_ptr<Equipment> equip0);
    //获取设备列表
    const std::vector<std::unique_ptr<Equipment>>& getEquipmentList();
    //获取角度
    void setOrientationDeg(double deg) { m_orientation_deg = deg; }
    //获取船速
    double getSpeed() const { return m_speed; }
    //设置船速
    void setSpeed(double speed) { m_speed = speed; }
    //设置坐标
    void setLocation(Point2D position_new);

private:
    string m_id;
    Point2D m_location;
    std::vector<double> m_distance;
    std::vector<std::unique_ptr<Equipment>>m_equipmentList;
    double m_orientation_deg = 0.0;
    double m_speed = 1.0;
};










