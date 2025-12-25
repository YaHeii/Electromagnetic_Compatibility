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
/// @param power_dbm 设备发射功率（dBm）
/// @brief 设备基类
/// </summary>
class Equipment {
public:
    Equipment(const std::string& id, EquipmentType type, const Point2D& relative_pos = {0,0}, Antenna* antenna_ptr = nullptr, double power_dbm = 0)
        : _id(id), _type(type), _relative_position(relative_pos), _antenna(antenna_ptr)
        ,_power_dbm(power_dbm) {}
    virtual ~Equipment() = default;

    std::string getID() const { return _id; }
    EquipmentType getType() const { return _type; }
    Point2D getRelativePosition() const { return _relative_position; }
    double getPowerDBm() const { return _power_dbm; }
    Antenna* getAntenna() const {return _antenna.get(); }
    // 设置设备相对位置的接口
    void setRelativePosition(const Point2D& position) { _relative_position = position; }
    void setRelativePosition(double x, double y) { _relative_position.x = x; _relative_position.y = y; }

    void setAntenna(std::unique_ptr<Antenna> antenna) {
        _antenna = std::move(antenna);
    }

protected:
    std::string _id;
    EquipmentType _type;
    Point2D _relative_position;
    double _power_dbm;
    std::unique_ptr<Antenna> _antenna;
};

/// <summary>
/// @param _frequency_mhz 中心频率
/// @param _bandwidth_khz 带宽
/// </summary>
class Transmitter : public Equipment {//发射机
public:
    Transmitter(const std::string& id,
                double frequency_mhz,
                double power_dbm,
                double bandwidth_khz,
                const Point2D& relative_pos = {0,0})
        : Equipment(id, EquipmentType::TRANSMITTER, relative_pos,nullptr, power_dbm),
          _frequency_mhz(frequency_mhz),
          _bandwidth_khz(bandwidth_khz) {}

    double getFrequencyMHz() const { return _frequency_mhz; }
    double getPowerDBm() const { return _power_dbm; }
    double getBandwidthKHz() const { return _bandwidth_khz; }

private:
    double _frequency_mhz;
    double _bandwidth_khz;
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
             const Point2D& relative_pos = {0.0,0.0})
        : Equipment(id, EquipmentType::RECEIVER, relative_pos, nullptr, 0),
          _frequency_mhz(frequency_mhz),
          _sensitivity_dbm(sensitivity_dbm),
          _bandwidth_khz(bandwidth_khz),
          _noise_figure_db(noise_figure_db),
          _SINR_threshold_db(SINR_threshold_db),
          _interference_threshold_db(interference_threshold_db),
          _transmitter_id(transmitter_id),
          _transmitter_in_ship_id(transmitter_in_ship_id) {}

    double getFrequencyMHz() const { return _frequency_mhz; }
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
    double _frequency_mhz;
    double _sensitivity_dbm;
    double _bandwidth_khz;
    double _noise_figure_db;
    double _SINR_threshold_db;
    double _interference_threshold_db;
    std::string _transmitter_id;
    std::string _transmitter_in_ship_id;
};