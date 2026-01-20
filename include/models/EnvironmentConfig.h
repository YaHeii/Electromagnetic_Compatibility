#pragma once

#include <QString>
#include <QList>
#include <QObject>
#include <vector> 
#include <cmath>  


struct EnvironmentConfig{
    double max_range = 20000.0;// 20 km
    double duct_height = 20.0; // 蒸发波导高度 H0 (m)
    double wind_speed = 7.0;   // 风速 (m/s)，用于计算 Miller-Brown 粗糙度

    std::pair<bool,QString> validate_EnvironmentConfig() const {
        
    }
}