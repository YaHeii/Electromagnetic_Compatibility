#pragma once
#include <string>
#include <memory>
#include "Antenna.h"
#include "ship.h"
#include "../utils/point_2D.h"
#include "../utils/conversions.h"


enum class EquipmentType {
    GENERIC,//通用设备
    TRANSMITTER,//发射机
    RECEIVER,//接收机
    TRANSCEIVER, // 收发一体,用于雷达
    RADAR//雷达
};

class Equipment {
public:
    Equipment(const std::string& id, EquipmentType type, const Point2D& relative_pos = {0,0}, double power_dbm = 0)
        : m_id(id), m_type(type), m_relative_position(relative_pos), m_antenna(nullptr),m_power_dbm(power_dbm) {}
    virtual ~Equipment() = default;

    std::string getID() const { return m_id; }
    EquipmentType getType() const { return m_type; }
    Point2D getRelativePosition() const { return m_relative_position; }

    void setAntenna(std::unique_ptr<Antenna> antenna) {
        m_antenna = std::move(antenna);
    }
    Antenna* getAntenna() const {
        return m_antenna.get();
    }
    double getPowerDBm() const { return m_power_dbm; }
protected:
    std::string m_id;
    EquipmentType m_type;
    Point2D m_relative_position; // 相对于船只参考点的位置
    double m_power_dbm;
    std::unique_ptr<Antenna> m_antenna;
};

// ... (Equipment class definition above) ...

class Transmitter : public Equipment {//发射机
public:
    Transmitter(const std::string& id,
                double frequency_mhz,//频率
                double power_dbm,//功率
                double bandwidth_khz,//带宽
                const Point2D& relative_pos = {0,0})
        : Equipment(id, EquipmentType::TRANSMITTER, relative_pos),
          m_frequency_mhz(frequency_mhz),
          m_power_dbm(power_dbm),
          m_bandwidth_khz(bandwidth_khz) {}

    double getFrequencyMHz() const { return m_frequency_mhz; }
    double getPowerDBm() const { return m_power_dbm; }
    double getBandwidthKHz() const { return m_bandwidth_khz; }

private:
    double m_frequency_mhz;
    double m_power_dbm;
    double m_bandwidth_khz;
};

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
        : Equipment(id, EquipmentType::RECEIVER, relative_pos),
          m_frequency_mhz(frequency_mhz),
          m_sensitivity_dbm(sensitivity_dbm),
          m_bandwidth_khz(bandwidth_khz),
          m_noise_figure_db(noise_figure_db),
          m_SINR_threshold_db(SINR_threshold_db),
          m_interference_threshold_db(interference_threshold_db),
          m_transmitter_id(transmitter_id),
          m_transmitter_in_ship_id(transmitter_in_ship_id) {}

    double getFrequencyMHz() const { return m_frequency_mhz; }
    double getSensitivityDBm() const { return m_sensitivity_dbm; }
    double getBandwidthKHz() const { return m_bandwidth_khz; }
    double getNoiseFigureDB() const { return m_noise_figure_db; }
    double getSINRThresholdDB() const { return m_SINR_threshold_db; }
    double getInterferenceThresholdDB() const { return m_interference_threshold_db; }
    std::string getTransmitterID() const { return m_transmitter_id; }
    std::string getTransmitterInShipID() const { return m_transmitter_in_ship_id; }
    // 计算接收机内部噪声基底 (dBm)
    // N = k * T0 * B * NF_linear, k = 1.380649e-23 J/K (Boltzmann constant)
    // T0 = 290 K (standard temperature)
    // k*T0 in dBm/Hz = -173.97 dBm/Hz
    //这个地方后面可以考虑加上，但是要注意对于不同的设备应该进行多态继承
    double getNoiseFloorDBm() const {
        double noise_floor_dbm_per_hz = -173.97;
        double bandwidth_hz = m_bandwidth_khz * 1000.0;
        return noise_floor_dbm_per_hz + 10.0 * std::log10(bandwidth_hz) + m_noise_figure_db;
    }


private:
    double m_frequency_mhz;
    double m_sensitivity_dbm;
    double m_bandwidth_khz;
    double m_noise_figure_db;
    double m_SINR_threshold_db;
    double m_interference_threshold_db;
    std::string m_transmitter_id;
    std::string m_transmitter_in_ship_id;
};




