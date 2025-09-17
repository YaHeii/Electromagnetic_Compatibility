#pragma once
#include <vector>
#include <string>
#include <iostream> // For temporary output
#include "fleet.h"
#include "equipment.h" // For Transmitter, Receiver
#include "../models/PropagationModle.h"
#include "../utils/conversions.h" // For dBmToWatts, wattsToDbm

namespace Electromagnetic_compatibility {
namespace core {

// Structure to hold a single interference result
struct InterferenceResult {//
    std::string aggressor_ship_id;//干扰源船ID
    std::string aggressor_equip_id;//干扰源设备ID
    std::string victim_ship_id;//受害船ID
    std::string victim_equip_id;//受害设备ID 
    double victim_rx_freq_mhz;//受害设备接收频率
    double interference_power_at_rx_input_dbm;//在受干扰设备接收端口处测得的干扰功率（单位：dBm
    double victim_noise_floor_dbm;//受干扰设备的噪声底（单位：dBm）
    double interference_plus_noise_dbm; // I+N
    double interference_margin_db; // Sensitivity - (I+N)，干扰裕度
    double communication_performance_db; // 通信性能，SINR
    bool is_communication_degraded; // 通信是否受损
    bool is_interference_degraded; // 干扰裕度是否超限
};

class EMCEngine {
public:
    EMCEngine(const models::PropagationModel& prop_model);
    std::vector<InterferenceResult> analyzeFleet(const core::Fleet& fleet);

private:
    const models::PropagationModel& m_prop_model;
};
}
} // namespace Electromagnetic_compatibility