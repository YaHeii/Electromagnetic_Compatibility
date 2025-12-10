#pragma once
#include <string>

/// <summary>
/// 
/// 
/// </summary>
class Antenna {
public:
    Antenna(const std::string& id,double relative_height,double beam_width_deg, double tilt_deg = 0.0)
        : _id(id),_relative_height(relative_height),
        _beam_width_deg(beam_width_deg), _tilt_deg(tilt_deg) {}
    virtual ~Antenna() = default;

    std::string getID() const { return _id; }
	double getRelativeHeight() const { return _relative_height; }
	double getBeamWidthDeg() const { return _beam_width_deg; }
	double getTiltDeg() const { return _tilt_deg; }

protected:
    std::string _id;
    double _relative_height;
    double _beam_width_deg;
    double _tilt_deg;
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