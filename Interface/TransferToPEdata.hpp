#pragma once
#include <vector>
#include <string>
#include "Simulation/PEModel.h"
#include <omp.h>
#include "Models/Equipment.h"
#include "Models/fleet.h"
#include "DataModel.h"
#include <stdexcept>
#include <spdlog/spdlog.h>
#include <fstream>
#include "Utils/PaintImage.hpp"

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

struct Transmitter_PE_data {
    std::string shipName = "DefaultShip"; // 船只ID
    std::string equipmenName = "DefaultEquipment"; // 设备名
    double X_offset = 0.0;  //X绝对坐标
	double Y_offset = 0.0;  //Y绝对坐标
	double Z_offset = 0.0;  //Z绝对坐标
    AntennaType antennaType = AntennaType::OMNI; // 天线类型
    double power_dbm = 0.0; // 发射功率 (dBm)
    double antenna_height = 25.0; // 天线高度 (m)(设备高度+天线高度)
    double beamWidth_deg = 2.0;     // 波束宽度 (度)
    double antennaPhi_deg = 2.0;     // 天线仰角 (度)
    double centralF_Ghz = 9.4e9;       // 9.4 GHz (X-band)
    GridMap PowerGrid = GridMap(0, LineMap(0)); // 传播损耗网格
};

inline std::vector<Transmitter_PE_data> EquipmentConvertToMatrix(Fleet* fleet) {
    std::vector<Transmitter_PE_data> pe_data_list;
    spdlog::info("Converting Fleet to PE_data list...");
    for (const auto& ship_ptr : fleet->getShips()) {
        for (const auto& equip_ptr : ship_ptr->getEquipmentList()) {
            Transmitter_PE_data data;
            if (equip_ptr->getType() == EquipmentType::TRANSMITTER || equip_ptr->getType() == EquipmentType::TRANSCEIVER) {
                Transmitter* transmitter_ptr = dynamic_cast<Transmitter*>(equip_ptr.get());
                data.shipName = ship_ptr->getID();
                data.equipmenName = transmitter_ptr->getID();
				data.X_offset = ship_ptr->getLocation().getX() + equip_ptr->getRelativePosition().getX();
				data.Y_offset = ship_ptr->getLocation().getY() + equip_ptr->getRelativePosition().getY();
				data.Z_offset = ship_ptr->getLocation().getZ() + equip_ptr->getRelativePosition().getZ();
                data.antennaType = transmitter_ptr->getAntennaType();
                data.power_dbm = transmitter_ptr->getPowerDBm();
                data.antenna_height = transmitter_ptr->getHeight() + ship_ptr->getHeight();
                data.beamWidth_deg = transmitter_ptr->getBeamWidth();
                data.antennaPhi_deg = transmitter_ptr->getAntennaPhi();
                data.centralF_Ghz = transmitter_ptr->getFrequencyGHz();
                pe_data_list.push_back(data);
                spdlog::info("Added Transmitter data for equipment: {}", data.equipmenName);
            }
            else {
                spdlog::warn("Skipping Reciever(or else) equipment: {}", equip_ptr->getID());
            }
        }
    }
    return pe_data_list;
}

