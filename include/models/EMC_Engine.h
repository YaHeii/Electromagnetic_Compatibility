#pragma once
#include <vector>
#include <string>
#include "PEModel.h"
#include <omp.h>
#include "core/Equipment.h"
#include "core/fleet.h"
#include "DataModel.h"
#include <stdexcept>
#include <spdlog/spdlog.h>
#include "../utils/PaintImage.hpp"

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
    std::string shipName = "DefaultShip"; // 船名
    std::string equipmenName = "DefaultEquipment"; // 设备名
    AntennaType antennaType = AntennaType::OMNI; // 天线类型
	double sender_antenna_height = 25.0; // 天线高度 (m)(设备高度+天线高度)
	double beamWidth_deg = 2.0;     // 波束宽度 (度)
	double antennaPhi_deg = 2.0;     // 天线仰角 (度)
    double centralF_Ghz = 9.4e9;       // 9.4 GHz (X-band)
    double dx = 50.0;          // 步进 50m
    double dz = 0.2;           // 垂直分辨率 0.2m (越高越好，建议 <= lambda/2)
    int nz = 2048;             // 物理高度网格 (总高度 ~400m)
    double max_range = 20000.0;// 20 km
	double duct_height = 20.0; // 蒸发波导高度 H0 (m)
    double wind_speed = 7.0;   // 风速 (m/s)，用于计算 Miller-Brown 粗糙度
};  

enum class ModelType {
    PE,
    RayModel
};
using GridMap = std::vector<std::vector<double>>;
using Matrix = std::vector<std::vector<double>>;
using LineMap = std::vector<double>;

class Propagation_Engine {
public:
    Propagation_Engine(ModelType model_type, std::unique_ptr<Fleet> fleet)
        : _model_type(model_type), _fleet(std::move(fleet)) {
        if (_model_type == ModelType::PE) {
            //初始化PEModel
            GridMap Loss2D = PEmodel_computing2D(_PEdata, 25);
        }
        else if (_model_type == ModelType::RayModel) {
            //初始化RayModel
        }
    }
    LineMap PEmodel_computing1D(PE_data _PEdata, double reciever_antenna_height);
    GridMap PEmodel_computing2D(PE_data _PEdata, double reciever_antenna_height);
    // TODO: 将所有的Equipment转换为Matrix格式，
    std::vector<PE_data> EquipmentConvertToMatrix(std::unique_ptr<Fleet> fleet);

private:
    std::unique_ptr<Fleet> _fleet;
    ModelType _model_type;
	PE_data _PEdata;
    GridMap _LossGrid;
    LineMap _LossLine;
};

class EMC_Engine : public QObject {
    Q_OBJECT
public:
    EMC_Engine(ModelType modelType, std::unique_ptr<Fleet> fleet)
        : _modelType(modelType),
          _fleet(std::move(fleet)),
          _dataSnapshot(DataModel::instance()->createSnapshot()),
          _propagationEngine(nullptr) // Always initialize pointers
    {
        if (!_fleet) {
            spdlog::error("EMC_Engine initialization failed: fleet is null");
        }
    }

    void do_PE_computing();
    GridMap do_PE_test();
    //std::vector<InterferenceResult> EMC_computing(const Fleet& fleet);//返回受扰计算结果数组
private:
    GridMap _LossGrid;
    std::vector<PE_data> _peDataList;
    using DataSnapshot = DataModel::DataSnapshot; // Use the snapshot from DataModel
	std::unique_ptr<Fleet> _fleet;
	DataSnapshot _dataSnapshot;
    ModelType _modelType;
    Propagation_Engine* _propagationEngine; // Consider using std::unique_ptr here as well
signals:
    void peComputationFinished(const std::string& shipName, const std::string& equipmentName, const GridMap& lossGrid);
};
