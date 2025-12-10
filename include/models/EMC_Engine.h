#pragma once
#include <vector>
#include <string>
#include "RayModel.h"
#include "PEModel.h"
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
struct PE_data {
	double sender_antenna_height = 25.0; // 天线高度 (m)
	double beam_width_deg = 2.0;     // 波束宽度 (度)
	double elevation_deg = 2.0;     // 天线仰角 (度)
    double freq = 9.4e9;       // 9.4 GHz (X-band)
    double dx = 50.0;          // 步进 50m
    double dz = 0.2;           // 垂直分辨率 0.2m (越高越好，建议 <= lambda/2)
    int nz = 2048;             // 物理高度网格 (总高度 ~400m)
    double max_range = 20000.0;// 20 km
	double duct_height = 20.0; // 蒸发波导高度 H0 (m)
    double wind_speed = 7.0;   // 风速 (m/s)，用于计算 Miller-Brown 粗糙度
};  

class Propagation_Engine {
public:
    Propagation_Engine(std::string Model_type)
        : _model_type(Model_type) {
        if (_model_type == "PEModel") {
            //初始化PEModel
        }
        else if (_model_type == "RayModel") {
            //初始化RayModel
        }
    }
    void initializePEmodel(PE_data _PEdata, double reciever_antenna_height);
private:
    std::string _model_type;
	PE_data _PEdata;
};


class EMC_Engine {
public:
    EMC_Engine(Equipment* analysed_Equipment) 
        :_analysed_Equipment(analysed_Equipment) {}
    //std::vector<InterferenceResult> EMC_computing(const Fleet& fleet);

private:
    Equipment* _analysed_Equipment;
};
