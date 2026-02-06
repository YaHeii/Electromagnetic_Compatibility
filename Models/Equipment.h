#pragma once
#include <string>
#include <memory>
#include "Antenna.h"
#include "ship.h"
#include "../Utils/point_2D.h"
#include "Utils/conversions.h"
enum class EquipmentType {
    TRANSMITTER,//发射机
    RECEIVER,//接收机
    TRANSCEIVER, // 收发一体
};

// 参数结构体：发射参数包
struct TxParams {
    double _centralF_Ghz = 10000.0;
    double _bandwidth_Mhz = 100.0;
    double _power_dbm = 20.0;
    double _beamWidth = 0.0;
    double _antennaPhi = 0.0; // 指向角
    AntennaType _antennaType = AntennaType::OMNI;
    PolarizationMethod _polarization = PolarizationMethod::VERTICAL;
};

// 参数结构体：接收参数包
struct RxParams {
    double _centralF_Ghz = 1000.0;
    double _bandwidth_Mhz = 100.0;
    double _sensitivity_dbm = -100.0;
    double _noise_figure_db = 3.0;
    double _SINR_threshold_db = 10.0;
    double _interference_threshold_db = 10.0;
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

    Equipment(const std::string& id, EquipmentType type, const Point3D& relative_pos = { 0,0,0 }, double gain_dbm = 0)
        : _id(id), _type(type), _relative_position(relative_pos), _gain_dbm(gain_dbm) { }

    virtual ~Equipment() = default;


    std::string getID() const { return _id; }
    EquipmentType getType() const { return _type; }
    Point3D getRelativePosition() const { return _relative_position; }
    double getHeight() const { return _relative_position.getZ(); }
    double getPowerDBm() const { return _gain_dbm; }
    Antenna* getAntenna() const { return _antenna.get(); }
    void setAntenna(std::unique_ptr<Antenna> antenna) { _antenna = std::move(antenna); }
    AntennaType getAntennaType() const { return _antenna ? _antenna->getType() : AntennaType::OMNI; }
    // 设置设备相对位置的接口
    void setRelativePosition(const Point3D& position) { _relative_position = position; }
    void setRelativePosition(double x, double y, double z) { _relative_position._x = x; _relative_position._y = y; _relative_position._z = z; }

protected:
    std::string _id;
    EquipmentType _type;
    Point3D _relative_position;
    double _gain_dbm;
    std::unique_ptr<Antenna> _antenna;

};


//发射机
/// <summary>
/// @param _params 发射参数包
/// </summary>
class Transmitter : public Equipment {
public:
    Transmitter(const std::string& id, 
        const TxParams& params, 
        const Point3D& pos = { 0,0,0 })
        : Equipment(id, EquipmentType::TRANSMITTER, pos, params._power_dbm),
        _params(params)
    {
        auto antenna = Antenna::create(
            id + "_ant", // 自动生成天线ID
            params._antennaType,
            params._polarization,
            pos, // 天线相对位置通常跟随设备，或者需要单独传入
            params._power_dbm,
            params._antennaPhi // 这里假设 phi 对应 tilt 或其他角度
        );
        this->setAntenna(std::move(antenna)); 
    }


    std::string getID() const { return _id; };
    double getFrequencyGHz() const { return _params._centralF_Ghz; }
    double getBandWidthMHz() const { return _params._bandwidth_Mhz; }
    double getPowerDBm() const { return _params._power_dbm; }
    double getBeamWidth() const { return _params._beamWidth; }
    double getAntennaPhi() const { return _params._antennaPhi; }
    PolarizationMethod getPolarizationMethod() const { return _params._polarization; }
    AntennaType getAntennaType_string() const { return _params._antennaType; }

private:
    TxParams _params;
};

///接收机
/// <summary>
/// _params 接收参数包
/// </summary>
class Receiver : public Equipment {//接收机
public:
    Receiver(const std::string& id, const RxParams& params, const std::string& tx_id = "", const Point3D& pos = { 0,0,0 })
        : Equipment(id, EquipmentType::RECEIVER, pos, 0),
        _params(params), _transmitter_in_ship_id(tx_id) {}

    double getFrequencyGHz() const { return _params._centralF_Ghz; }
    double getSensitivityDBm() const { return _params._sensitivity_dbm; }
    double getBandwidthKHz() const { return _params._bandwidth_Mhz; }
    double getNoiseFigureDB() const { return _params._noise_figure_db; }
    double getSINRThresholdDB() const { return _params._SINR_threshold_db; }
    double getInterferenceThresholdDB() const { return _params._interference_threshold_db; }
    std::string getTransmitterID() const { return _transmitter_id; }
    std::string getTransmitterInShipID() const { return _transmitter_in_ship_id; }
    // 计算接收机内部噪声基底 (dBm)
    // N = k * T0 * B * NF_linear, k = 1.380649e-23 J/K (Boltzmann constant)
    // T0 = 290 K (standard temperature)
    // k*T0 in dBm/Hz = -173.97 dBm/Hz
    static double calculateNoiseFloor(double bandwidth_khz, double nf_db) {
        // N = -174 + 10log(B_Hz) + NF
        return -173.97 + 10.0 * std::log10(bandwidth_khz * 1000.0) + nf_db;
    }

    double getNoiseFloorDBm() const {
        return calculateNoiseFloor(_params._bandwidth_Mhz, _params._noise_figure_db);
    }

private:
    RxParams _params;
    std::string _transmitter_id;
	std::string _transmitter_in_ship_id;
};

class Transceiver : public Equipment {
public:
    Transceiver(const std::string& id,
        const TxParams& tx_params,
        const RxParams& rx_params,
        const Point3D& pos = { 0,0,0 })
        : Equipment(id, EquipmentType::TRANSCEIVER, pos, tx_params._power_dbm),
        _tx_params(tx_params), _rx_params(rx_params)
    {
        // 创建天线 (通常收发共用一个天线，或者使用发射参数创建)
        auto antenna = Antenna::create(
            id + "_ant",
            tx_params._antennaType,
            tx_params._polarization,
            pos,
            tx_params._power_dbm,
            tx_params._antennaPhi
        );
        this->setAntenna(std::move(antenna));
    }

    // --- 发射相关接口 ---
    std::string getID() const { return _id; };
    double getTXFrequencyMHz() const { return _tx_params._centralF_Ghz; }
    double getTXBandWidthKHz() const { return _tx_params._bandwidth_Mhz; }
    double getPowerDBm() const { return _tx_params._power_dbm; }
    double getBeamWidth() const { return _tx_params._beamWidth; }
    double getAntennaPhi() const { return _tx_params._antennaPhi; }
    PolarizationMethod getPolarizationMethod() const { return _tx_params._polarization; }
    AntennaType getAntennaType_string() const { return _tx_params._antennaType; }

    // --- 接收相关接口 ---
    double getRXFrequencyMHz() const { return _rx_params._centralF_Ghz; }
    double getSensitivityDBm() const { return _rx_params._sensitivity_dbm; }
    double getRXBandwidthKHz() const { return _rx_params._bandwidth_Mhz; }
    double getNoiseFigureDB() const { return _rx_params._noise_figure_db; }
    double getSINRThresholdDB() const { return _rx_params._SINR_threshold_db; }
    double getInterferenceThresholdDB() const { return _rx_params._interference_threshold_db; }
    std::string getTransmitterID() const { return _transmitter_id; }
    std::string getTransmitterInShipID() const { return _transmitter_in_ship_id; }

    // 计算接收机内部噪声基底 (dBm)
    // N = k * T0 * B * NF_linear, k = 1.380649e-23 J/K (Boltzmann constant)
    // T0 = 290 K (standard temperature)
    // k*T0 in dBm/Hz = -173.97 dBm/Hz
    double getNoiseFloorDBm() const {
        return Receiver::calculateNoiseFloor(_rx_params._bandwidth_Mhz, _rx_params._noise_figure_db);
    }


private:
    TxParams _tx_params;
    RxParams _rx_params;
	std::string _transmitter_id;
	std::string _transmitter_in_ship_id;
};
