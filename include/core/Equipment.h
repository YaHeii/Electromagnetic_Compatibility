#pragma once
#include <string>
#include <memory>
#include "Antenna.h"
#include "ship.h"
#include "../utils/point_2D.h"
#include "../utils/conversions.h"


enum class EquipmentType {
    TRANSMITTER,//发射机
    RECEIVER,//接收机
    TRANSCEIVER, // 收发一体
};
/// <summary>
/// @param id 设备标识符
/// @param type 设备类型
/// @param relative_pos 设备相对位置
/// @param antenna 设备天线指针
/// @param power_dbm 发射接收增益（dBm）
/// @brief 设备基类
/// </summary>
class Equipment {
public:
    Equipment(const std::string& id, EquipmentType type, const Point3D& relative_pos = {0,0,0}, Antenna* antenna_ptr = nullptr, double gain_dbm = 0)
        : _id(id), _type(type), _relative_position(relative_pos), _antenna(antenna_ptr)
        ,_gain_dbm(gain_dbm) {}
    virtual ~Equipment() = default;

    std::string getID() const { return _id; }
    EquipmentType getType() const { return _type; }
    Point3D getRelativePosition() const { return _relative_position; }
    double getPowerDBm() const { return _gain_dbm; }
    Antenna* getAntenna() const {return _antenna.get(); }
    // 设置设备相对位置的接口
    void setRelativePosition(const Point3D& position) { _relative_position = position; }
    void setRelativePosition(double x, double y, double z) { _relative_position._x = x; _relative_position._y = y; _relative_position._z = z; }

    void setAntenna(std::unique_ptr<Antenna> antenna) {
        _antenna = std::move(antenna);
    }

protected:
    std::string _id;
    EquipmentType _type;
    Point3D _relative_position;
    double _gain_dbm;
    std::unique_ptr<Antenna> _antenna;

};


//发射机
/// <summary>
/// @param _frequency_mhz 中心频率
/// @param _bandwidth_khz 带宽
/// </summary>
class Transmitter : public Equipment {
public:
    Transmitter(const std::string& id,
                double frequency_mhz,
                double gain_dbm,
                double bandwidth_khz,
                double antennaPhi = 0,
                double beamWidth = 0,
                QString polarizationMethod = "",
                QString antennaType_string = "",
                const Point3D& relative_pos = {0,0,0})
        : Equipment(id, EquipmentType::TRANSMITTER, relative_pos, nullptr, gain_dbm),
          _centralF_mhz(frequency_mhz),
          _bandwidth_khz(bandwidth_khz),
          _antennaPhi(antennaPhi),
          _beamWidth(beamWidth),
          _polarizationMethod_string(polarizationMethod),
          _antennaType_string(antennaType_string) {
            // 创建天线对象
            Antenna::create(_id, _antennaType_string, _polarizationMethod_string, _relative_position, _gain_dbm, _antennaPhi);
          }

    double getFrequencyMHz() const { return _centralF_mhz; }
    double getBandWidthKHz() const { return _bandwidth_khz; }
    double getPowerDBm() const { return _power_dbm; }
    double getBeamWidth() const { return _beamWidth; }
    double getAntennaPhi() const { return _antennaPhi; }
    PolarizationMethod getPolarizationMethod() const {
        if(_polarizationMethod_string == "垂直极化") return PolarizationMethod::VERTICAL;
        else if(_polarizationMethod_string == "水平极化") return PolarizationMethod::HORIZONTAL;
    }
    AntennaType getAntennaType_string() const {
        if(_antennaType_string == "喇叭天线")  return AntennaType::HORN;
        else if(_antennaType_string == "赋型波束天线") return AntennaType::ShapedBeam;
        else if (_antennaType_string == "抛物面天线") return AntennaType::Reflector;
    }
    
    void setAntennaPhi(double antennaPhi) {this->_antennaPhi = antennaPhi;}

private:
    double _centralF_mhz;
    double _bandwidth_khz;
    double _power_dbm;
    double _antennaPhi;
    double _beamWidth;

    QString _antennaType_string;
    AntennaType _antennaType;

    QString _polarizationMethod_string;
    PolarizationMethod _polarizationMethod;
};

/// <summary>
/// @param _frequency_mhz 中心频率
/// @param _sensitivity_dbm 接收机灵敏度
/// @param _bandwidth_khz 带宽
/// @param _noise_figure_db 噪声系数
/// @param _SINR_threshold_db 信噪比阈值
/// @param _interference_threshold_db 干扰阈值
/// @func  getNoiseFloorDBm 计算接收机内部噪声基底 (dBm)
/// </summary>
class Receiver : public Equipment {//接收机
public:
    Receiver(const std::string& id,
             double frequency_mhz,//频率
             double sensitivity_dbm, // 接收机灵敏度
             double bandwidth_khz,//带宽
             std::string transmitter_id,//表示发送设备id
             std::string transmitter_in_ship_id,//表示设备所在船ID
             double noise_figure_db = 3.0, // 噪声系数 (dB)
             double SINR_threshold_db = 10.0, // 信噪比阈值 (dB)
             double interference_threshold_db = 10.0, // 干扰阈值 (dB)
             const Point3D& relative_pos = {0.0,0.0,0.0})
        : Equipment(id, EquipmentType::RECEIVER, relative_pos, nullptr, 0),
          _centralF_mhz(frequency_mhz),
          _sensitivity_dbm(sensitivity_dbm),
          _bandwidth_khz(bandwidth_khz),
          _noise_figure_db(noise_figure_db),
          _SINR_threshold_db(SINR_threshold_db),
          _interference_threshold_db(interference_threshold_db),
          _transmitter_id(transmitter_id),
          _transmitter_in_ship_id(transmitter_in_ship_id) {}

    Receiver(const std::string& id,
        double frequency_mhz,//频率
        double gain_dbm,
        double sensitivity_dbm, // 接收机灵敏度
        double bandwidth_khz,//带宽
        double noise_figure_db, // 噪声系数 (dB)
        double SINR_threshold_db, // 信噪比阈值 (dB)
        double interference_threshold_db, // 干扰阈值 (dB)
        const Point3D& relative_pos)
        : Equipment(id, EquipmentType::RECEIVER, relative_pos, nullptr, gain_dbm),
        _centralF_mhz(frequency_mhz),
        _sensitivity_dbm(sensitivity_dbm),
        _bandwidth_khz(bandwidth_khz),
        _noise_figure_db(noise_figure_db),
        _SINR_threshold_db(SINR_threshold_db),
        _interference_threshold_db(interference_threshold_db){}

    double getFrequencyMHz() const { return _centralF_mhz; }
    double getSensitivityDBm() const { return _sensitivity_dbm; }
    double getBandwidthKHz() const { return _bandwidth_khz; }
    double getNoiseFigureDB() const { return _noise_figure_db; }
    double getSINRThresholdDB() const { return _SINR_threshold_db; }
    double getInterferenceThresholdDB() const { return _interference_threshold_db; }
    std::string getTransmitterID() const { return _transmitter_id; }
    std::string getTransmitterInShipID() const { return _transmitter_in_ship_id; }
    // 计算接收机内部噪声基底 (dBm)
    // N = k * T0 * B * NF_linear, k = 1.380649e-23 J/K (Boltzmann constant)
    // T0 = 290 K (standard temperature)
    // k*T0 in dBm/Hz = -173.97 dBm/Hz
    double getNoiseFloorDBm() const {
        double noise_floor_dbm_per_hz = -173.97;
        double bandwidth_hz = _bandwidth_khz * 1000.0;
        return noise_floor_dbm_per_hz + 10.0 * std::log10(bandwidth_hz) + _noise_figure_db;
    }

private:
    double _centralF_mhz;
    double _bandwidth_khz;
    double _sensitivity_dbm;
    double _noise_figure_db;
    double _SINR_threshold_db;
    double _interference_threshold_db;
    std::string _transmitter_id;
    std::string _transmitter_in_ship_id;
};

class Transceiver : public Equipment {
public:
    Transceiver(const std::string& id,
                double gain_dbm = 0,
                const Point3D& relative_pos = {0.0,0.0,0.0},
                // 发射参数
                double tx_centralF_mhz = 0, double tx_bw_khz = 0,
                double power_dbm = 0, double antennaPhi = 0, double beamWidth = 0,
                QString polarizationMethod_string = "", QString antennaType_string = "",

                // 接收参数
                double rx_centralF_mhz = 0, double rx_bw_khz = 0, double rx_sens_dbm = 0,
                double noise_figure_db = 0, double SINR_threshold_db = 0,
                double interference_threshold_db = 0)
        : Equipment(id, EquipmentType::TRANSCEIVER, relative_pos, nullptr, gain_dbm),
          // 初始化所有私有成员
        _tx_centralF_mhz(tx_centralF_mhz), _tx_bandwidth_khz(tx_bw_khz), _power_dbm(power_dbm),
        _antennaPhi(antennaPhi), _beamWidth(beamWidth),
        _polarizationMethod_string(polarizationMethod_string),
        _antennaType_string(antennaType_string),
         
        _rx_centralF_mhz(rx_centralF_mhz), _rx_bandwidth_khz(rx_bw_khz),
        _sensitivity_dbm(rx_sens_dbm), _noise_figure_db(noise_figure_db),
        _SINR_threshold_db(SINR_threshold_db),
        _interference_threshold_db(interference_threshold_db){}

    // --- 发射相关接口 ---
    double getTXFrequencyMHz() const { return _tx_centralF_mhz; }
    double getTXBandWidthKHz() const { return _tx_bandwidth_khz; }
    double getPowerDBm() const { return _power_dbm; }
    double getBeamWidth() const { return _beamWidth; }
    double getAntennaPhi() const { return _antennaPhi; }
    PolarizationMethod getPolarizationMethod() const {
        if (_polarizationMethod_string == "垂直极化") return PolarizationMethod::VERTICAL;
        else if (_polarizationMethod_string == "水平极化") return PolarizationMethod::HORIZONTAL;
    }
    AntennaType getAntennaType_string() const {
        if (_antennaType_string == "喇叭天线")  return AntennaType::HORN;
        else if (_antennaType_string == "赋型波束天线") return AntennaType::ShapedBeam;
        else if (_antennaType_string == "抛物面天线") return AntennaType::Reflector;
    }


    void setAntennaPhi(double antennaPhi) { this->_antennaPhi = antennaPhi; }

    // --- 接收相关接口 ---
    double getRXFrequencyMHz() const { return _rx_centralF_mhz; }
    double getSensitivityDBm() const { return _sensitivity_dbm; }
    double getRXBandwidthKHz() const { return _rx_bandwidth_khz; }
    double getNoiseFigureDB() const { return _noise_figure_db; }
    double getSINRThresholdDB() const { return _SINR_threshold_db; }
    double getInterferenceThresholdDB() const { return _interference_threshold_db; }
    std::string getTransmitterID() const { return _transmitter_id; }
    std::string getTransmitterInShipID() const { return _transmitter_in_ship_id; }
    // 计算接收机内部噪声基底 (dBm)
    // N = k * T0 * B * NF_linear, k = 1.380649e-23 J/K (Boltzmann constant)
    // T0 = 290 K (standard temperature)
    // k*T0 in dBm/Hz = -173.97 dBm/Hz
    double getNoiseFloorDBm() const {
        double noise_floor_dbm_per_hz = -173.97;
        double bandwidth_hz = _rx_bandwidth_khz * 1000.0;
        return noise_floor_dbm_per_hz + 10.0 * std::log10(bandwidth_hz) + _noise_figure_db;
    }
private:
    // 显式定义所有变量，不依赖继承
    // 1. 发射部分
    double _tx_centralF_mhz;
    double _tx_bandwidth_khz;
    double _power_dbm;
    double _antennaPhi;
    double _beamWidth;

    QString _polarizationMethod_string;
    PolarizationMethod _polarizationMethod;
    QString _antennaType_string;
    VerticalFieldDistribution _antennaType;

    // 2. 接收部分
    double _rx_centralF_mhz;
    double _rx_bandwidth_khz;
    double _sensitivity_dbm;
    double _noise_figure_db;
    double _SINR_threshold_db;
    double _interference_threshold_db;
    std::string _transmitter_id = "0";
    std::string _transmitter_in_ship_id = "0";
};
