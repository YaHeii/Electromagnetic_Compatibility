#pragma once
#include <string>
#include "../../include/utils/point_2D.h"
#include <QString>

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
    Antenna(const std::string& id, QString antenna_type_string, 
        QString polarization_method_string, const Point3D& relative_pos = { 0,0,0 }, 
        double gain_dbm = 0, double tilt_deg = 0.0)
        : _id(id), _antenna_type_string(antenna_type_string), _polarization_method_string(polarization_method_string),
        _relative_position(relative_pos), _gain_dbm(gain_dbm), _tilt_deg(tilt_deg) {}
    virtual ~Antenna() = default;

    static std::unique_ptr<Antenna> create(
        const std::string& id,
        AntennaType type,               // 决定创建哪个子类
        QString polarization_method,    // 透传给基类
        const Point3D& relative_pos = { 0,0,0 },
        double gain_dbm = 0.0,
        double tilt_deg = 0.0
    );

    std::string getID() const { return _id; }
    QString getAntennaTypeString() const { return _antenna_type_string; }
    QString getPolarizationString() const { return _polarization_method_string; }
    Point3D getRelativePosition() const { return _relative_position; }
    double getTiltDeg() const { return _tilt_deg; }
    double getGain() const { return _gain_dbm; }
    PolarizationMethod getPolarizationMethod() const {
        if (_polarization_method_string == "垂直极化") return PolarizationMethod::VERTICAL;
        else if (_polarization_method_string == "水平极化") return PolarizationMethod::HORIZONTAL;
    }
    AntennaType getAntennaType_string() const {
        if (_antenna_type_string == "喇叭天线")  return AntennaType::HORN;
        else if (_antenna_type_string == "赋型波束天线") return AntennaType::ShapedBeam;
        else if (_antenna_type_string == "抛物面天线") return AntennaType::Reflector;
    }
private:
    std::string _id;
    Point3D _relative_position;
    double _gain_dbm;
    double _tilt_deg;

    QString _antenna_type_string;
    AntennaType _type;
    QString _polarization_method_string;
    PolarizationMethod _polarization_method = PolarizationMethod::VERTICAL;
};

// 全向天线
class OmniAntenna : public Antenna{
public:
    OmniAntenna(const std::string & id, QString polarization, const Point3D & pos, double gain, double tilt)
        : Antenna(id, "全向天线", polarization, pos, gain, tilt) {}
};
//定向天线
class DirectionalAntenna : public Antenna {
public:
    DirectionalAntenna(const std::string& id, QString polarization, const Point3D& pos, double gain, double tilt)
        : Antenna(id, "定向天线", polarization, pos, gain, tilt) {}
};
//喇叭天线
class HornAntenna : public Antenna {
public:
    // 子类构造函数：自动将 "喇叭天线" 字符串传给基类
    HornAntenna(const std::string& id, QString polarization, const Point3D& pos, double gain, double tilt)
        : Antenna(id, "喇叭天线", polarization, pos, gain, tilt){}

    VerticalFieldDistribution getDistribution() const { return VerticalFieldDistribution::GAUSSIAN; }
};


//赋形波束天线
class ShapedBeamAntenna : public Antenna {
public:
    // 自动传递 "赋形波束天线"
    ShapedBeamAntenna(const std::string& id, QString polarization, const Point3D& pos, double gain, double tilt)
        : Antenna(id, "赋形波束天线", polarization, pos, gain, tilt)
    {}

    VerticalFieldDistribution getDistribution() const { return VerticalFieldDistribution::SINC; }
};


//抛物面天线
class ReflectorAntenna : public Antenna {
public:
    // 自动传递 "抛物面天线"
    ReflectorAntenna(const std::string& id, QString polarization, const Point3D& pos, double gain, double tilt)
        : Antenna(id, "抛物面天线", polarization, pos, gain, tilt)
    {}

    VerticalFieldDistribution getDistribution() const { return VerticalFieldDistribution::COSINE; }
};

inline std::unique_ptr<Antenna> Antenna::create(
    const std::string& id,
    AntennaType type,
    QString polarization_method,
    const Point3D& relative_pos,
    double gain_dbm,
    double tilt_deg)
{
    switch (type) {
    case AntennaType::HORN:
        return std::make_unique<HornAntenna>(id, polarization_method, relative_pos, gain_dbm, tilt_deg);

    case AntennaType::ShapedBeam:
        return std::make_unique<ShapedBeamAntenna>(id, polarization_method, relative_pos, gain_dbm, tilt_deg);

    case AntennaType::Reflector:
        return std::make_unique<ReflectorAntenna>(id, polarization_method, relative_pos, gain_dbm, tilt_deg);

    case AntennaType::OMNI:
        return std::make_unique<OmniAntenna>(id, polarization_method, relative_pos, gain_dbm, tilt_deg);

    case AntennaType::DIRECTIONAL:
        return std::make_unique<DirectionalAntenna>(id, polarization_method, relative_pos, gain_dbm, tilt_deg);

    default:
        // 默认处理：返回空指针或默认的全向天线
        return nullptr;
    }
}