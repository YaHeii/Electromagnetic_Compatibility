#pragma once
#include <vector>
#include <string>
#include "PropagationModle.h"
#include "core/Equipment.h"

struct InterferenceResult {
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
class Propagation_Engine {


};
class EMC_Engine {
public:
    EMC_Engine(Equipment* analysed_Equipment) 
        :_analysed_Equipment(analysed_Equipment) {}
    //std::vector<InterferenceResult> EMC_computing(const Fleet& fleet);

private:
    Equipment* _analysed_Equipment;
};
