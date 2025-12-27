#include "../../include/models/EMC_Engine.h"
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include <spdlog/spdlog.h>

LineMap Propagation_Engine::PEmodel_computing1D(PE_data _PEdata,double reciever_antenna_height) {
    // 初始化大气模型：蒸发波导高度 20m
    // 对应 Paper 2 Fig. 6(d) 和 Eq. (35)
    AtmosphereModel atm(_PEdata.duct_height);
    JONSWAPSurfaceGenerator surface(_PEdata.wind_speed);
    // 预计算折射率剖面 (Profile)
    // 这一步非常重要，避免在 step 循环中重复计算 log 函数，提升效率
    std::vector<double> n_profile(_PEdata.nz);
    for (int i = 0; i < _PEdata.nz; ++i) {
        double z = i * _PEdata.dz;
        n_profile[i] = atm.getRefractiveIndex(z);
    }

    // 初始化求解器
    PEModel solver(_PEdata.centralF_Ghz, _PEdata.dx, _PEdata.dz, _PEdata.nz);

    // 初始化高斯波束：天线高度 25m
    solver.initializeGaussian(_PEdata.sender_antenna_height,_PEdata.beamWidth_deg
        , _PEdata.antennaPhi_deg);

    // 开始步进仿真
    std::cout << "Range(km) \t Loss(dB) \t (Atmosphere: Evaporation Duct 20m)" << std::endl;
    //r为仿真距离（剖面）
    for (double r = _PEdata.dx; r < _PEdata.max_range; r += _PEdata.dx) {
        // 将预计算好的大气剖面传递给求解器
        //solver.step_Miller_Brown(r, _PEdata.wind_speed, n_profile);
        solver.step_PLST(r, n_profile, surface, 0);

        // 输出数据
        if (std::abs(fmod(r, 1000.0)) < 0.1) {
            // 获取接收天线高度 15m 处的损耗
            int rx_idx = static_cast<int>(reciever_antenna_height / _PEdata.dz);
            double loss = solver.getPathLoss(rx_idx, r);
            std::cout << r / 1000.0 << " \t\t " << loss << std::endl;
            _LossLine.push_back(loss);
        }
    }
    return _LossLine;
}

GridMap Propagation_Engine::PEmodel_computing2D(PE_data _PEdata, double reciever_antenna_height) {
    spdlog::info("Starting 2D PE model computation for equipment: {}", _PEdata.equipmenName);
    spdlog::info("Parameters: Frequency = {} GHz, Max Range = {} m, Duct Height = {} m, Wind Speed = {} m/s",
		_PEdata.centralF_Ghz / 1e9, _PEdata.max_range, _PEdata.duct_height, _PEdata.wind_speed);
    // 1. 定义地图网格参数
    double map_size_km = 20.0;
    double grid_res_m = 50.0; // 50米一个像素
    int grid_dim = static_cast<int>((map_size_km * 1000) / grid_res_m); // 400x400

    // 初始化地图为极小值 (代表无信号/底噪)
    GridMap coverage_map(grid_dim, std::vector<double>(grid_dim, -200.0));

    // 2. 准备环境
    AtmosphereModel atm(_PEdata.duct_height);
    JONSWAPSurfaceGenerator surface(_PEdata.wind_speed);
    // 预计算 n_profile
    std::vector<double> n_profile(_PEdata.nz);
    for (int i = 0; i < _PEdata.nz; ++i) n_profile[i] = atm.getRefractiveIndex(i * _PEdata.dz);

    // 3. 存储极坐标扫描数据 [角度][距离索引] -> 损耗值
    // 角度步长 5度
    int angle_step_deg = 5;
    int num_angles = 360 / angle_step_deg;
    int num_ranges = static_cast<int>(_PEdata.max_range / _PEdata.dx);

    // polar_data[angle_idx][range_idx]
    std::vector<std::vector<double>> polar_data(num_angles, std::vector<double>(num_ranges));

    spdlog::info("Starting 360-degree scan...");
    // 创建每个线程专用的 solver 池，避免在并行循环中反复创建/销毁 FFTW 计划
    int num_threads = omp_get_max_threads();
    std::vector<std::unique_ptr<PEModel>> solvers;
    solvers.reserve(num_threads);
    for (int t = 0; t < num_threads; ++t) {
        solvers.emplace_back(std::make_unique<PEModel>(_PEdata.centralF_Ghz, _PEdata.dx, _PEdata.dz, _PEdata.nz));
        solvers.back()->initializeGaussian(_PEdata.sender_antenna_height, _PEdata.beamWidth_deg, _PEdata.antennaPhi_deg);
    }
    // --- 核心循环：旋转扫描 ---
#pragma omp parallel for // 并行计算各个角度 (OpenMP)
    for (int i = 0; i < num_angles; ++i) {
        double az_deg = i * angle_step_deg;
        double az_rad = az_deg * M_PI / 180.0;

        int tid = omp_get_thread_num();
        PEModel* solver = solvers[tid].get();

        solver->initializeGaussian(_PEdata.sender_antenna_height, _PEdata.beamWidth_deg, _PEdata.antennaPhi_deg);

        int range_idx = 0;
        for (double r = _PEdata.dx; r < _PEdata.max_range; r += _PEdata.dx) {
            // 调用修改后的 PLST，传入方位角
            solver->step_PLST(r, az_rad, n_profile, surface, 0.0);

            // 获取特定高度的损耗并存储
            int rx_z_idx = static_cast<int>(reciever_antenna_height / _PEdata.dz);
            polar_data[i][range_idx] = solver->getPathLoss(rx_z_idx, r);
            range_idx++;
        }
    }

    std::cout << "Scan complete. Mapping to Cartesian grid..." << std::endl;
    std::cout << "Range(km) \t Loss(dB) \t (Atmosphere: Evaporation Duct 20m)" << std::endl;
    // 4. 坐标映射 (填满车轮空隙)
    // 遍历地图上的每一个像素点 (x, y)
    double center_idx = grid_dim / 2.0;

#pragma omp parallel for collapse(2)
    for (int y = 0; y < grid_dim; ++y) {
        for (int x = 0; x < grid_dim; ++x) {
            // 像素坐标 -> 物理坐标 (相对于中心，单位 m)
            double px = (x - center_idx) * grid_res_m;
            double py = (y - center_idx) * grid_res_m; // 注意图像坐标系 y 可能相反，这里暂按数学坐标

            // 计算极坐标 (r, theta)
            double r = std::sqrt(px * px + py * py);
            double theta = std::atan2(py, px); // (-PI, PI)
            if (theta < 0) theta += 2.0 * M_PI; // (0, 2PI)

            // 如果超出最大射程，跳过
            if (r >= _PEdata.max_range) continue;

            // 查找最近的数据点 (Nearest Neighbor 插值)
            // 1. 找角度索引
            double az_deg = theta * 180.0 / M_PI;
            int az_idx = static_cast<int>(std::round(az_deg / angle_step_deg)) % num_angles;

            // 2. 找距离索引
            int r_idx = static_cast<int>(r / _PEdata.dx);
            if (r_idx >= num_ranges) r_idx = num_ranges - 1;

            // 填值
            coverage_map[y][x] = polar_data[az_idx][r_idx];
            std::cout << x / 1000.0 << " \t\t " << coverage_map[y][x];
        }
    }

    return coverage_map;
}
//std::vector<InterferenceResult> EMC_Engine::EMC_computing(const Fleet& fleet) {//返回受扰计算结果数组
//
//}

std::vector<PE_data> Propagation_Engine::EquipmentConvertToMatrix(std::unique_ptr<Fleet> fleet) {
    std::vector<PE_data> pe_data_list;
    spdlog::info("Converting Fleet to PE_data list...");
    for (const auto& ship_ptr : fleet->getShips()) {
        const ship& current_ship = *ship_ptr;

        for (const auto& equip_ptr : current_ship.getEquipmentList()) {
            Equipment* equipment = equip_ptr.get();
            
            // 尝试将 Equipment* 动态转换为 Transmitter* 或 Transceiver*
            Transmitter* tx = dynamic_cast<Transmitter*>(equipment);
            Transceiver* trx = dynamic_cast<Transceiver*>(equipment);

            if (tx || trx) {
                PE_data pe_data;

                pe_data.shipName = current_ship.getID();
                pe_data.equipmenName = equipment->getID();

                // 获取天线和位置信息
                Point3D ship_pos = current_ship.getLocation();
                Point3D equip_pos = equipment->getRelativePosition();
                pe_data.sender_antenna_height = ship_pos._z + equip_pos._z;

                if (tx) {
                    pe_data.antennaType = tx->getAntennaType_string();
                    pe_data.beamWidth_deg = tx->getBeamWidth();
                    pe_data.antennaPhi_deg = tx->getAntennaPhi();
                    pe_data.centralF_Ghz = tx->getFrequencyMHz() / 1000.0;
                } else { // trx
                    pe_data.antennaType = trx->getAntennaType_string();
                    pe_data.beamWidth_deg = trx->getBeamWidth();
                    pe_data.antennaPhi_deg = trx->getAntennaPhi();
                    pe_data.centralF_Ghz = trx->getTXFrequencyMHz() / 1000.0;
                }
                
                pe_data_list.push_back(pe_data);
            }
        }
    }

    return pe_data_list;
}

void EMC_Engine::do_PE_computing() {
    // 将所有船只和设备转换为 PE_data 列表
    _peDataList = _propagationEngine->EquipmentConvertToMatrix(std::move(_fleet));
	spdlog::info("Starting PE computations for {} equipment items...", _peDataList.size());
    // 对每个 PE_data 进行传播计算
    for (const auto& pe_data : _peDataList) {
        _LossGrid = _propagationEngine->PEmodel_computing2D(pe_data, 25.0); // 假设接收天线高度为 25m
        // 这里可以存储或处理 loss_line 数据
        emit peComputationFinished(pe_data.shipName, pe_data.equipmenName, _LossGrid);
    }
}