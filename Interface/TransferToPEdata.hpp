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


//TODO：单独建立Interface/TransferToPEdata
struct Transmitter_PE_data {
    std::string shipName = "DefaultShip"; // 船只ID
    std::string equipmenName = "DefaultEquipment"; // 设备名
    AntennaType antennaType = AntennaType::OMNI; // 天线类型
    double power_dbm = 0.0; // 发射功率 (dBm)
    double antenna_height = 25.0; // 天线高度 (m)(设备高度+天线高度)
    double beamWidth_deg = 2.0;     // 波束宽度 (度)
    double antennaPhi_deg = 2.0;     // 天线仰角 (度)
    double centralF_Ghz = 9.4e9;       // 9.4 GHz (X-band)
    GridMap PowerGrid = GridMap(0, LineMap(0)); // 传播损耗网格
};

std::vector<Transmitter_PE_data> EquipmentConvertToMatrix(Fleet* fleet) {
    std::vector<Transmitter_PE_data> pe_data_list;
    spdlog::info("Converting Fleet to PE_data list...");
    for (const auto& ship_ptr : fleet->getShips()) {
        for (const auto& equip_ptr : ship_ptr->getEquipmentList()) {
            Transmitter_PE_data data;
            if (equip_ptr->getType() == EquipmentType::TRANSMITTER || equip_ptr->getType() == EquipmentType::TRANSCEIVER) {
                Transmitter* transmitter_ptr = dynamic_cast<Transmitter*>(equip_ptr.get());
                data.shipName = ship_ptr->getID();
                data.equipmenName = transmitter_ptr->getID();
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

