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
		_PEdata.centralF_Ghz, _PEdata.max_range, _PEdata.duct_height, _PEdata.wind_speed);
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

    int last_percent = -1;
    std::cout << "Mapping Polar to Cartesian Grid..." << std::endl;
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
            if (y == grid_dim / 2) {
                // 限制打印频率，每隔 10 个点打一次，防止刷屏
                if (x % 10 == 0) {
                    // 格式化输出：坐标(km) -> 损耗(dB)
                    printf("X: %6.2f km, Y: %6.2f km | Loss: %6.2f dB\n",
                        px / 1000.0, py / 1000.0, coverage_map[y][x]);
                }
            }
        }
        // 进度条逻辑 (仅在主线程打印，避免乱序)
        if (omp_get_thread_num() == 0) {
            int percent = (y * 100) / grid_dim;
            if (percent != last_percent && percent % 10 == 0) { // 每10%提示一次
                std::cout << "[Progress] Grid Mapping: " << percent << "%" << std::endl;
                last_percent = percent;
            }
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
    if (!_fleet) {
        spdlog::error("Fleet is null, cannot perform PE computing.");
        return;
    }

    // 将所有船只和设备转换为 PE_data 列表
    _peDataList = _propagationEngine->EquipmentConvertToMatrix(std::move(_fleet));
    spdlog::info("Starting PE computations for {} equipment items...", _peDataList.size());

    // 对每个 PE_data 进行传播计算
    for (const auto& pe_data : _peDataList) {
        // 这里可以根据 pe_data 的参数调整接收天线高度，暂时固定为 25.0
        double receiver_height = 25.0;

        // 计算二维损耗网格
        _LossGrid = _propagationEngine->PEmodel_computing2D(pe_data, receiver_height);

        // 触发信号，通知UI更新
        emit peComputationFinished(pe_data.shipName, pe_data.equipmenName, _LossGrid);
    }
}
GridMap EMC_Engine::do_PE_test() {
    spdlog::info("Starting PE test computation...");
        PE_data pe_data;
        pe_data.centralF_Ghz = 1;          // 1 GHz (确保单位正确)
        pe_data.sender_antenna_height = 25.0;
        pe_data.beamWidth_deg = 2.0;      // 波束宽度
        pe_data.antennaPhi_deg = 0.0;     // 【关键】仰角改为0，确保照射海面
        pe_data.dx = 10.0;                // 【关键】减小步长以捕捉 7m/s 的海浪
        pe_data.dz = 0.1;                 // 适配 1GHz 波长
        pe_data.nz = 4096;                // 保证计算域高度 ~400m
        pe_data.max_range = 20000.0;      // 20km
        pe_data.duct_height = 20.0;       // 蒸发波导
        pe_data.wind_speed = 7.0;

    _LossGrid = _propagationEngine->PEmodel_computing2D(pe_data, 5.0); // 接收天线高度 25m
    return _LossGrid;
    emit peComputationFinished(pe_data.shipName, pe_data.equipmenName, _LossGrid);
}

// 退化参数为平面，以双径模型验证结果
void EMC_Engine::do_Validation_TwoRay() {
    spdlog::info("Starting Level 1 Validation: Flat Sea & Standard Atmosphere...");

    // 1. 设置退化参数
    PE_data data;
    data.centralF_Ghz = 1;       // 1 GHz
    data.sender_antenna_height = 25.0; // ht = 25m
    data.beamWidth_deg = 20.0;  // 设宽波束，确保能照亮海面反射点
    data.antennaPhi_deg = 0.0;  // 水平发射
    data.dx = 10.0;             
    data.dz = 0.1;
    data.nz = 4096;
    data.max_range = 10000.0;   // 10km 足够看清干涉
    data.duct_height = 0.0;     // 无波导 (标准大气)
    data.wind_speed = 0.001;    // 近乎静止的平坦海面

    // 2. 初始化环境
    AtmosphereModel atm(0.0); 
    JONSWAPSurfaceGenerator surface(data.wind_speed); 
    std::vector<double> n_profile(data.nz, 1.0); 

    // 3. 运行 PE
    PEModel solver(data.centralF_Ghz, data.dx, data.dz, data.nz);
    solver.initializeGaussian(data.sender_antenna_height, data.beamWidth_deg, data.antennaPhi_deg);

    // 4. 准备导出数据
    std::ofstream out("validation_data.csv");
    out << "Range_m,PE_Loss_dB,Theory_Loss_dB\n";

    double receiver_h = 15.0; // hr = 15m

    for (double r = data.dx; r < data.max_range; r += data.dx) {
        // 使用 PLST 步进 (由于 wind=0，PLST 应当退化为标准平坦 PE)
        solver.step_PLST(r, 0.0, n_profile, surface, 0.0);

        // --- A. 获取 PE 结果 ---
        int rx_idx = static_cast<int>(receiver_h / data.dz);
        double pe_loss = solver.getPathLoss(rx_idx, r);

        // --- B. 计算理论双径结果 (Two-Ray Analytical) ---
        // 自由空间损耗
        double lambda = 299792458.0 / (data.centralF_Ghz * 1.0e9);
        double fspl = 20 * std::log10(4 * M_PI * r / lambda);
        
        // 传播因子 F (考虑地面的反射)
        // 路径差 delta_R approx 2*ht*hr / r
        double delta_R = 2.0 * data.sender_antenna_height * receiver_h / r;
        double phase_diff = (2.0 * M_PI / lambda) * delta_R;
        
        // 理想导体反射系数 Gamma = -1 (即相移 PI)
        // E_total = E_direct + E_reflected = 1 + (-1)*exp(-j*k*delta_R)
        Complex E_total = 1.0 - std::exp(Complex(0, -phase_diff));
        double F_linear = std::abs(E_total); // 线性幅值
        
        // 加上天线方向图修正 (因为理论公式假设全向，但我们是高斯)
        // 计算直射波角度，看它在高斯波束的哪个位置
        double direct_angle = std::atan((receiver_h - data.sender_antenna_height) / r);
        // 高斯方向图因子 G(theta)
        // 注意：这里仅仅是粗略修正，为了完美重合，建议理论公式只对比“波峰/波谷的位置”，不强求幅值绝对一致
        
        double theory_loss = fspl - 20 * std::log10(F_linear + 1e-10);

        out << r << "," << pe_loss << "," << theory_loss << "\n";
    }
    out.close();
    spdlog::info("Validation data saved to validation_data.csv");
}

void EMC_Engine::do_Validation_Roughness() {
    spdlog::info("Starting Level 2 Validation: PLST vs Miller-Brown...");

    // 1. 设置测试参数 (典型的 X 波段海面场景)
    PE_data data;
    data.centralF_Ghz = 10;       // 10 GHz
    data.sender_antenna_height = 15.0;
    data.max_range = 20000.0;    // 20 km
    data.dx = 10.0; data.dz = 0.1; data.nz = 2048;
    data.wind_speed = 10.0;      // 【关键】较大的风速，确保粗糙度效应明显
    data.duct_height = 0.0;      // 暂时关闭波导，专注于表面散射验证

    // 2. 初始化环境
    AtmosphereModel atm(data.duct_height);
    JONSWAPSurfaceGenerator surface(data.wind_speed);
    std::vector<double> n_profile(data.nz, 1.0); // 标准大气 n=1

    PEModel solver_mb(data.centralF_Ghz, data.dx, data.dz, data.nz); // Solver A: Miller-Brown
    PEModel solver_plst(data.centralF_Ghz, data.dx, data.dz, data.nz); // Solver B: PLST

    // 初始化同样的高斯波束
    solver_mb.initializeGaussian(data.sender_antenna_height, 2.0, 0.0);
    solver_plst.initializeGaussian(data.sender_antenna_height, 2.0, 0.0);

    // 3. 运行对比仿真
    std::ofstream out("validation_roughness.csv");
    out << "Range_m,Loss_MillerBrown,Loss_PLST\n";

    double rx_h = 10.0;
    int rx_idx = rx_h / data.dz;

    for (double r = data.dx; r < data.max_range; r += data.dx) {
        // --- A. 运行 Miller-Brown (基准) ---
        // 注意：你需要确保 step_Miller_Brown 被正确声明为 public
        solver_mb.step_Miller_Brown(r, data.wind_speed, n_profile);
        double loss_mb = solver_mb.getPathLoss(rx_idx, r);

        // --- B. 运行 PLST (待验证对象) ---
        solver_plst.step_PLST(r, n_profile, surface, 0.0);
        double loss_plst = solver_plst.getPathLoss(rx_idx, r);

        out << r << "," << loss_mb << "," << loss_plst << "\n";
    }
    out.close();
}

void EMC_Engine::do_Validation_DuctLeakage() {
    spdlog::info("Starting Level 3 Validation: Duct Leakage Effect...");

    // 1. 设置通用参数 (X波段，强波导)
    PE_data data;
    data.centralF_Ghz = 10;       // 10 GHz
    data.sender_antenna_height = 10.0; // 发射机位于波导内部 (H0=20m, Ht=10m)
    data.max_range = 50000.0;    // 【关键】距离要足够远 (50km)，累积泄漏才明显
    data.dx = 10.0;
    data.dz = 0.1;
    data.nz = 2048;              // 计算高度约 200m，足以覆盖波导层(20m)和泄漏层(>20m)
    data.duct_height = 20.0;     // 【关键】20米强蒸发波导

    // 2. 准备两种场景
    // 场景 A: 平静海面 (Wind = 0) -> 能量应该被死死锁住
    double wind_flat = 0.0;
    // 场景 B: 粗糙海面 (Wind = 15) -> 能量应该被散射出去
    double wind_rough = 15.0;

    // 3. 初始化环境
    // 大气模型是一样的 (都是 20m 波导)
    AtmosphereModel atm(data.duct_height);
    std::vector<double> n_profile(data.nz);
    for (int i = 0; i < data.nz; ++i) n_profile[i] = atm.getRefractiveIndex(i * data.dz);

    // 表面生成器
    JONSWAPSurfaceGenerator surf_flat(wind_flat);
    JONSWAPSurfaceGenerator surf_rough(wind_rough);

    // 求解器
    PEModel solver_flat(data.centralF_Ghz, data.dx, data.dz, data.nz);
    PEModel solver_rough(data.centralF_Ghz, data.dx, data.dz, data.nz);

    solver_flat.initializeGaussian(data.sender_antenna_height, 2.0, 0.0);
    solver_rough.initializeGaussian(data.sender_antenna_height, 2.0, 0.0);

    // 4. 运行仿真到最大距离
    // 这里我们不需要中间数据，只需要跑到终点看垂直分布
    int steps = static_cast<int>(data.max_range / data.dx);

    spdlog::info("Computing Flat Surface Case...");
    for (int s = 0; s < steps; ++s) {
        double r = (s + 1) * data.dx;
        solver_flat.step_PLST(r, n_profile, surf_flat, 0.0);
    }

    spdlog::info("Computing Rough Surface Case...");
    for (int s = 0; s < steps; ++s) {
        double r = (s + 1) * data.dx;
        solver_rough.step_PLST(r, n_profile, surf_rough, 0.0);
    }

    // 5. 提取终点处 (Range = 50km) 的垂直剖面数据
    std::ofstream out("validation_leakage.csv");
    out << "Height_m,Loss_Flat_dB,Loss_Rough_dB\n";

    // 遍历所有高度层
    for (int i = 0; i < data.nz; ++i) {
        double z = i * data.dz;

        // 获取损耗
        double loss_flat = solver_flat.getPathLoss(i, data.max_range);
        double loss_rough = solver_rough.getPathLoss(i, data.max_range);

        // 过滤掉极其微弱的数值(比如高空吸收层)，只输出有效数据
        // 或者直接输出，用Excel筛选
        if (z < 150.0) { // 只关注 150m 以下，太高了是吸收层
            out << z << "," << loss_flat << "," << loss_rough << "\n";
        }
    }
    out.close();
    spdlog::info("Validation data saved to validation_leakage.csv");
}