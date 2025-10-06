#pragma once
#include <string>

class Antenna {
public:
    Antenna(const std::string& id, double orientation_deg = 0.0)
        : m_id(id), m_orientation_deg(orientation_deg) {}
    virtual ~Antenna() = default;

    virtual double getGainDbi(double azimuth_deg_relative_to_antenna) const {
        // 默认实现全向天线, 0 dBi增益
        return 0.0;
    }

    std::string getID() const { return m_id; }
    double getOrientationDeg() const { return m_orientation_deg; }

protected:
    std::string m_id;
    double m_orientation_deg; // 天线自身相对于安装平台的朝向
};

// 一个简单的全向天线
class OmniAntenna : public Antenna {
public:
    OmniAntenna(const std::string& id, double gain_dbi = 0.0)
        : Antenna(id), m_gain_dbi(gain_dbi) {}

    double getGainDbi(double azimuth_deg_relative_to_antenna) const override {
        return m_gain_dbi; // 任何方向增益都一样
    }
private:
    double m_gain_dbi;
};

//定向天线
class DirectionalAntenna : public Antenna {
public:
    DirectionalAntenna(const std::string& id, double gain_dbi = 0.0, double azimuth_deg = 0.0, double elevation_deg = 0.0)//天线朝向为azimuth_deg, 仰角为elevation_deg, 增益为gain_dbi
        : Antenna(id, azimuth_deg), m_gain_dbi(gain_dbi), m_azimuth_deg(azimuth_deg), m_elevation_deg(elevation_deg) {}

    double getGainDbi(double azimuth_deg_relative_to_antenna) const override {
        return m_gain_dbi; // 任何方向增益都一样
    }
private:
    double m_gain_dbi;
    double m_azimuth_deg;
    double m_elevation_deg;
};