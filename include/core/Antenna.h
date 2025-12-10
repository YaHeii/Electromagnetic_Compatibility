#pragma once
#include <string>

/// <summary>
/// @param id 天线标识符
/// @param relative_height 天线相对高度（相对于地面或海平面）
/// @param beam_width_deg 天线波束宽度（度）
/// @param tilt_deg 天线倾斜角（度），默认为0
/// @brief 天线基类
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

private:
    std::string _id;
    double _relative_height;
    double _beam_width_deg;
    double _tilt_deg;
};

// 一个简单的全向天线
class OmniAntenna : public Antenna {
public:
    //弃用
    OmniAntenna(const std::string& id, double gain_dbi = 0.0)
        : Antenna(id, 0.0, 360.0, 0.0),
        _gain_dbi(gain_dbi) {}
    double getGainDbi(double azimuth_deg_relative_to_antenna) const {
        return _gain_dbi; // 任何方向增益都一样
    }
private:
    double _gain_dbi;
};

//定向天线
class DirectionalAntenna : public Antenna {
public:
    // 弃用
    DirectionalAntenna(const std::string& id, double gain_dbi = 0.0, double azimuth_deg = 0.0, double tilt_deg = 0.0)//天线朝向为azimuth_deg, 仰角为elevation_deg, 增益为gain_dbi
        : Antenna(id, 0.0, 360.0, tilt_deg),  _gain_dbi(gain_dbi), _azimuth_deg(azimuth_deg) {}
    //TODO：弃用，使用垂直场分布u(0,z)
    double getGainDbi(double azimuth_deg_relative_to_antenna) const {
        return _gain_dbi; // 任何方向增益都一样
    }
private:
    double _gain_dbi;
    double _azimuth_deg;
};