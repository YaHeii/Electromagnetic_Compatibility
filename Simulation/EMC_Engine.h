#pragma once
#include <vector>
#include <string>
#include "PEModel.h"
#include <omp.h>
#include "Models/Equipment.h"
#include "Models/fleet.h"
#include "Interface/DataModel.h"
#include <stdexcept>
#include <spdlog/spdlog.h>
#include <fstream>
#include "Utils/PaintImage.hpp"
#include "EnvironmentConfig.h"

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
    std::string equipmenName = "DefaultEquipment"; // 设备名
    AntennaType antennaType = AntennaType::OMNI; // 天线类型
	double antenna_height = 25.0; // 天线高度 (m)(设备高度+天线高度)
	double beamWidth_deg = 2.0;     // 波束宽度 (度)
	double antennaPhi_deg = 2.0;     // 天线仰角 (度)
    double centralF_Ghz = 9.4e9;       // 9.4 GHz (X-band)
};  

enum class ModelType {
    PE,
    RayModel
};
using GridMap = std::vector<std::vector<double>>;
using Matrix = std::vector<std::vector<double>>;
using LineMap = std::vector<double>;
using GridMatrix = Eigen::MatrixXd;
class Propagation_Engine;
class EMC_Engine : public QObject {
    Q_OBJECT
public:
    EMC_Engine(ModelType modelType, std::unique_ptr<Fleet> fleet)
        : _modelType(modelType),
          _fleet(std::move(fleet)),
          _dataSnapshot(DataModel::instance()->createSnapshot()),
          _propagationEngine(nullptr) 
    {
        if (!_fleet) {
            spdlog::error("EMC_Engine initialization failed: fleet is null");
        }
    }
	void InitPropagationEngine();
    void do_PE_computing();
    GridMap do_PE_test();
    void do_Validation_TwoRay();
    void do_Validation_Roughness();
    void do_Validation_DuctLeakage();
    std::vector<Transmitter_PE_data> EquipmentConvertToMatrix(Fleet* fleet);

    template <typename... Args>
    static void writeCSVRow(std::ofstream& out, Args... args) {
        // 使用 lambda 和 C++17 折叠表达式来处理逗号分隔
        bool first = true;
        auto print_arg = [&](const auto& val) {
            if (!first) {
                out << ",";
            }
            out << val;
            first = false;
            };

        // 折叠表达式：对 args 参数包中的每一个元素调用 print_arg
        (print_arg(args), ...);

        // 每一行结束后换行
        out << "\n";
    }

private:
    GridMap _LossGrid;
    std::vector<Transmitter_PE_data> _peDataList;
    using DataSnapshot = DataModel::DataSnapshot; 
	std::unique_ptr<Fleet> _fleet;
	DataSnapshot _dataSnapshot;
    ModelType _modelType;
    Propagation_Engine* _propagationEngine;
	EnvironmentConfig _env;
signals:
    void peComputationFinished(const GridMap& lossGrid);
};


class Propagation_Engine {
public:
    Propagation_Engine(ModelType model_type, const Fleet* fleet)
        : _model_type(model_type), _fleet(fleet) {}
    LineMap PEmodel_computing1D(Transmitter_PE_data PEdata, EnvironmentConfig env, double reciever_antenna_height);
    GridMatrix PEmodel_computing2D(Transmitter_PE_data PEdata, EnvironmentConfig env, double reciever_antenna_height);


private:
    const Fleet* _fleet;
    ModelType _model_type;
    Transmitter_PE_data _PEdata;
    GridMap _LossGrid;
    LineMap _LossLine;
	EnvironmentConfig _env;
};