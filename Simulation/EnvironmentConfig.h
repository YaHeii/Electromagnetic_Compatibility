#pragma once
#include <QString>
#include <QList>
#include <QObject>
#include <vector> 
#include <cmath>  
//TODO:merge to Datamodel.h
struct EnvironmentConfig {
    double max_range = 2000.0;// 20 km
    double duct_height = 20.0; // 蒸发波导高度 H0 (m)
    double wind_speed = 7.0;   // 风速 (m/s)，用于计算 Miller-Brown 粗糙度
    double dx = 5.0;          // 步进 50m
    double dz = 0.2;           // 垂直分辨率 0.2m (越高越好，建议 <= lambda/2)
    int nz = 2048;             // 物理高度网格 (总高度 ~400m)
	int angle_step_deg = 5; // 角度步进 5度 (用于2D仿真)
    //std::pair<bool,QString> validate_EnvironmentConfig() const {
    //    
    //}
};