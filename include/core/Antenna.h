#pragma once
#include <string>
#include "../../include/utils/point_2D.h"


class HornAntenna;
class ShapedBeamAntenna;
class ReflectorAntenna;

enum class AntennaType {
    OMNI,//全向天线,弃用
    DIRECTIONAL, //定向天线，弃用
    HORN, //喇叭天线，使用高斯垂直场分布
    ShapedBeam, // 赋形波束天线，使用Sinc分布
    Reflector // 抛物面天线，余弦及余弦幂分布 TODO
};

enum class VerticalFieldDistribution {
    GAUSSIAN,
    SINC,
    COSINE  //TODO
};

enum class PolarizationMethod {
    VERTICAL,
    HORIZONTAL
};
/// <summary>
/// @param id 天线标识符
/// @param relative_height 天线相对高度（相对于地面或海平面）
/// @param beam_width_deg 天线波束宽度（度）
/// @param tilt_deg 天线倾斜角（度），默认为0
/// @brief 天线基类
/// </summary>
class Antenna {
public:
    Antenna(const std::string& id, AntennaType type,
        PolarizationMethod pol, const Point3D& relative_pos = { 0,0,0 },
        double gain_dbm = 0, double tilt_deg = 0.0)
        : _id(id), _type(type), _polarization(pol),
        _relative_position(relative_pos), _gain_dbm(gain_dbm), _tilt_deg(tilt_deg) {
    }

    virtual ~Antenna() = default;

    // 工厂方法：接收 Enum
    static std::unique_ptr<Antenna> create(
        const std::string& id,
        AntennaType type,
        PolarizationMethod pol,
        const Point3D& relative_pos = { 0,0,0 },
        double gain_dbm = 0.0,
        double tilt_deg = 0.0
    );

    // Getters
    std::string getID() const { return _id; }
    AntennaType getType() const { return _type; }
    PolarizationMethod getPolarization() const { return _polarization; }
    Point3D getRelativePosition() const { return _relative_position; }
    double getGain() const { return _gain_dbm; }

    // 虚函数：获取垂直场分布 (基类默认抛异常或返回默认)
    virtual VerticalFieldDistribution getDistribution() const { return VerticalFieldDistribution::GAUSSIAN; }
protected:
    std::string _id;
    AntennaType _type;
    PolarizationMethod _polarization;
    Point3D _relative_position;
    double _gain_dbm;
    double _tilt_deg;
};

// 全向天线
class OmniAntenna : public Antenna{
public:
    OmniAntenna(const std::string & id, std::string pol, const Point3D & pos, double gain, double tilt)
        : Antenna(id, AntennaType::OMNI, pol, pos, gain, tilt) {}
};
//定向天线
class DirectionalAntenna : public Antenna {
public:
    DirectionalAntenna(const std::string& id, std::string pol, const Point3D& pos, double gain, double tilt)
        : Antenna(id, AntennaType::DIRECTIONAL, pol, pos, gain, tilt) {}
};
//喇叭天线
class HornAntenna : public Antenna {
public:
    HornAntenna(const std::string& id, PolarizationMethod pol, const Point3D& pos, double gain, double tilt)
        : Antenna(id, AntennaType::HORN, pol, pos, gain, tilt) {
    }

    VerticalFieldDistribution getDistribution() const override { return VerticalFieldDistribution::GAUSSIAN; }
};


//赋形波束天线
class ShapedBeamAntenna : public Antenna {
public:
    // 自动传递 "赋形波束天线"
    ShapedBeamAntenna(const std::string& id, std::string pol, const Point3D& pos, double gain, double tilt)
        : Antenna(id, AntennaType::ShapedBeam, pol, pos, gain, tilt)
    {}

    VerticalFieldDistribution getDistribution() const { return VerticalFieldDistribution::SINC; }
};


//抛物面天线
class ReflectorAntenna : public Antenna {
public:
    // 自动传递 "抛物面天线"
    ReflectorAntenna(const std::string& id, std::string pol, const Point3D& pos, double gain, double tilt)
        : Antenna(id, AntennaType::Reflector, pol, pos, gain, tilt)
    {}

    VerticalFieldDistribution getDistribution() const { return VerticalFieldDistribution::COSINE; }
};

inline std::unique_ptr<Antenna> Antenna::create(
    const std::string& id, 
    AntennaType type, 
    PolarizationMethod pol, 
    const Point3D& pos, 
    double gain, 
    double tilt) {
{

    switch (type) {
    case AntennaType::HORN:
        return std::make_unique<HornAntenna>(id, pol, pos, gain, tilt);
    case AntennaType::ShapedBeam:
        return std::make_unique<ShapedBeamAntenna>(id, pol, pos, gain, tilt);
    case AntennaType::Reflector:
        return std::make_unique<ReflectorAntenna>(id, pol, pos, gain, tilt);
    case AntennaType::OMNI:
        return std::make_unique<OmniAntenna>(id, pol, pos, gain, tilt);
    case AntennaType::DIRECTIONAL:
        return std::make_unique<DirectionalAntenna>(id, pol, pos, gain, tilt);
    default:
        // 默认处理：返回空指针或默认的全向天线
        return nullptr;
    }
}