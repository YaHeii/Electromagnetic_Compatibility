#include "EMC_Engine.h"
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include <spdlog/spdlog.h>
// Eigen Matrix -> GridMap
GridMap eigen_to_vector(const Eigen::MatrixXd& mat) {
    // 预分配外层 vector
    std::vector<std::vector<double>> vec(mat.rows());

    // 并行拷贝 (如果矩阵非常大，比如 2000x2000，否则单线程即可)
#pragma omp parallel for
    for (long i = 0; i < mat.rows(); ++i) {
        // resize 内层
        vec[i].resize(mat.cols());
        Eigen::Map<Eigen::VectorXd>(vec[i].data(), mat.cols()) = mat.row(i);
    }
    return vec;
}



void EMC_Engine::do_PE_computing() {
    if (!_fleet) {
        spdlog::error("Fleet is null, cannot perform PE computing.");
        return;
    }

    // 将所有船只和设备转换为 PE_data 列表
    _peDataList = EMC_Engine::EquipmentConvertToMatrix(_fleet.get());
    spdlog::info("Starting PE computations for {} equipment items...", _peDataList.size());
    // 这里可以根据 pe_data 的参数调整接收天线高度，暂时固定为 25.0
    // TODO: 设置reciever_PE_data, 支持针对不同接收设备来做PE计算
    double receiver_height = 25.0;
    // 计算二维损耗网格
    for (auto& pe_data : _peDataList) {
        pe_data.PowerGrid = eigen_to_vector(pe_data.power_dbm - _propagationEngine->PEmodel_computing2D(pe_data, _env, receiver_height).array());
    }
    //REVIEW: 是否使用GridMatrix优化计算
    for (const auto& pe_data : _peDataList) {
        for (size_t i = 0; i < _LossGrid.size(); ++i) {
            for (size_t j = 0; j < _LossGrid[i].size(); ++j) {
                _LossGrid[i][j] += pe_data.PowerGrid[i][j];
            }
        }
    }
    // 触发信号，通知UI更新
    emit peComputationFinished(_LossGrid);
}

GridMap EMC_Engine::do_PE_test() {
    spdlog::info("Starting PE test computation...");
    Transmitter_PE_data pe_data;
    pe_data.centralF_Ghz = 1;          // 1 GHz (确保单位正确)
    pe_data.antenna_height = 25.0;
    pe_data.beamWidth_deg = 2.0;      // 波束宽度
    pe_data.antennaPhi_deg = 0.0;     // 【关键】仰角改为0，确保照射海面
    _env.dx = 10.0;                // 【关键】减小步长以捕捉 7m/s 的海浪
    _env.dz = 0.1;                 // 适配 1GHz 波长
    _env.nz = 4096;                // 保证计算域高度 ~400m
    _env.max_range = 20000.0;      // 20km
    _env.duct_height = 20.0;       // 蒸发波导
    _env.wind_speed = 7.0;

    //_LossGrid = _propagationEngine->PEmodel_computing2D(pe_data, 5.0); // 接收天线高度 25m
    return _LossGrid;
    emit peComputationFinished(_LossGrid);
}

// 退化参数为平面，以双径模型验证结果
void EMC_Engine::do_Validation_TwoRay() {
    spdlog::info("Starting Level 1 Validation: Flat Sea & Standard Atmosphere...");

    // 1. 设置退化参数
    Transmitter_PE_data data;
    data.centralF_Ghz = 1;       // 1 GHz
    data.antenna_height = 25.0; // ht = 25m
    data.beamWidth_deg = 20.0;  // 设宽波束，确保能照亮海面反射点
    data.antennaPhi_deg = 0.0;  // 水平发射
    _env.dx = 10.0;
    _env.dz = 0.1;
    _env.nz = 4096;
    _env.max_range = 10000.0;   // 10km 足够看清干涉
    _env.duct_height = 0.0;     // 无波导 (标准大气)
    _env.wind_speed = 0.001;    // 近乎静止的平坦海面

    // 2. 初始化环境
    AtmosphereModel atm(0.0);
    JONSWAPSurfaceGenerator surface(_env.wind_speed);
    std::vector<double> n_profile(_env.nz, 1.0);

    // 3. 运行 PE
    PEModel solver(data.centralF_Ghz, _env.dx, _env.dz, _env.nz);
    solver.initializeGaussian(data.antenna_height, data.beamWidth_deg, data.antennaPhi_deg);

    // 4. 准备导出数据
    std::ofstream out("validation_data.csv");
    out << "Range_m,PE_Loss_dB,Theory_Loss_dB\n";

    double receiver_h = 15.0; // hr = 15m

    for (double r = _env.dx; r < _env.max_range; r += _env.dx) {
        // 使用 PLST 步进 (由于 wind=0，PLST 应当退化为标准平坦 PE)
        solver.step_PLST(r, 0.0, n_profile, surface, 0.0);

        // --- A. 获取 PE 结果 ---
        int rx_idx = static_cast<int>(receiver_h / _env.dz);
        double pe_loss = solver.getPathLoss(rx_idx, r);

        // --- B. 计算理论双径结果 (Two-Ray Analytical) ---
        // 自由空间损耗
        double lambda = 299792458.0 / (data.centralF_Ghz * 1.0e9);
        double fspl = 20 * std::log10(4 * M_PI * r / lambda);

        // 传播因子 F (考虑地面的反射)
        // 路径差 delta_R approx 2*ht*hr / r
        double delta_R = 2.0 * data.antenna_height * receiver_h / r;
        double phase_diff = (2.0 * M_PI / lambda) * delta_R;

        // 理想导体反射系数 Gamma = -1 (即相移 PI)
        // E_total = E_direct + E_reflected = 1 + (-1)*exp(-j*k*delta_R)
        Complex E_total = 1.0 - std::exp(Complex(0, -phase_diff));
        double F_linear = std::abs(E_total); // 线性幅值

        // 加上天线方向图修正 (因为理论公式假设全向，但我们是高斯)
        // 计算直射波角度，看它在高斯波束的哪个位置
        double direct_angle = std::atan((receiver_h - data.antenna_height) / r);
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
    Transmitter_PE_data data;
    data.centralF_Ghz = 10;       // 10 GHz
    data.antenna_height = 15.0;
    _env.max_range = 20000.0;    // 20 km
    _env.dx = 10.0; _env.dz = 0.1; _env.nz = 2048;
    _env.wind_speed = 10.0;      // 【关键】较大的风速，确保粗糙度效应明显
    _env.duct_height = 0.0;      // 暂时关闭波导，专注于表面散射验证

    // 2. 初始化环境
    AtmosphereModel atm(_env.duct_height);
    JONSWAPSurfaceGenerator surface(_env.wind_speed);
    std::vector<double> n_profile(_env.nz, 1.0); // 标准大气 n=1

    PEModel solver_mb(data.centralF_Ghz, _env.dx, _env.dz, _env.nz); // Solver A: Miller-Brown
    PEModel solver_plst(data.centralF_Ghz, _env.dx, _env.dz, _env.nz); // Solver B: PLST

    // 初始化同样的高斯波束
    solver_mb.initializeGaussian(data.antenna_height, 2.0, 0.0);
    solver_plst.initializeGaussian(data.antenna_height, 2.0, 0.0);

    // 3. 运行对比仿真
    std::ofstream out("validation_roughness.csv");
    out << "Range_m,Loss_MillerBrown,Loss_PLST\n";

    double rx_h = 10.0;
    int rx_idx = rx_h / _env.dz;

    for (double r = _env.dx; r < _env.max_range; r += _env.dx) {
        // --- A. 运行 Miller-Brown (基准) ---
        // 注意：你需要确保 step_Miller_Brown 被正确声明为 public
        solver_mb.step_Miller_Brown(r, _env.wind_speed, n_profile);
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
    Transmitter_PE_data data;
    data.centralF_Ghz = 10;       // 10 GHz
    data.antenna_height = 10.0; // 发射机位于波导内部 (H0=20m, Ht=10m)
    _env.max_range = 50000.0;    // 【关键】距离要足够远 (50km)，累积泄漏才明显
    _env.dx = 10.0;
    _env.dz = 0.1;
    _env.nz = 2048;              // 计算高度约 200m，足以覆盖波导层(20m)和泄漏层(>20m)
    _env.duct_height = 20.0;     // 【关键】20米强蒸发波导

    // 2. 准备两种场景
    // 场景 A: 平静海面 (Wind = 0) -> 能量应该被死死锁住
    double wind_flat = 0.0;
    // 场景 B: 粗糙海面 (Wind = 15) -> 能量应该被散射出去
    double wind_rough = 15.0;

    // 3. 初始化环境
    // 大气模型是一样的 (都是 20m 波导)
    AtmosphereModel atm(_env.duct_height);
    std::vector<double> n_profile(_env.nz);
    for (int i = 0; i < _env.nz; ++i) n_profile[i] = atm.getRefractiveIndex(i * _env.dz);

    // 表面生成器
    JONSWAPSurfaceGenerator surf_flat(wind_flat);
    JONSWAPSurfaceGenerator surf_rough(wind_rough);

    // 求解器
    PEModel solver_flat(data.centralF_Ghz, _env.dx, _env.dz, _env.nz);
    PEModel solver_rough(data.centralF_Ghz, _env.dx, _env.dz, _env.nz);

    solver_flat.initializeGaussian(data.antenna_height, 2.0, 0.0);
    solver_rough.initializeGaussian(data.antenna_height, 2.0, 0.0);

    // 4. 运行仿真到最大距离
    // 这里我们不需要中间数据，只需要跑到终点看垂直分布
    int steps = static_cast<int>(_env.max_range / _env.dx);

    spdlog::info("Computing Flat Surface Case...");
    for (int s = 0; s < steps; ++s) {
        double r = (s + 1) * _env.dx;
        solver_flat.step_PLST(r, n_profile, surf_flat, 0.0);
    }

    spdlog::info("Computing Rough Surface Case...");
    for (int s = 0; s < steps; ++s) {
        double r = (s + 1) * _env.dx;
        solver_rough.step_PLST(r, n_profile, surf_rough, 0.0);
    }

    // 5. 提取终点处 (Range = 50km) 的垂直剖面数据
    std::ofstream out("validation_leakage.csv");
    out << "Height_m,Loss_Flat_dB,Loss_Rough_dB\n";

    // 遍历所有高度层
    for (int i = 0; i < _env.nz; ++i) {
        double z = i * _env.dz;

        // 获取损耗
        double loss_flat = solver_flat.getPathLoss(i, _env.max_range);
        double loss_rough = solver_rough.getPathLoss(i, _env.max_range);

        // 过滤掉极其微弱的数值(比如高空吸收层)，只输出有效数据
        // 或者直接输出，用Excel筛选
        if (z < 150.0) { // 只关注 150m 以下，太高了是吸收层
            out << z << "," << loss_flat << "," << loss_rough << "\n";
        }
    }
    out.close();
    spdlog::info("Validation data saved to validation_leakage.csv");
}

void EMC_Engine::InitPropagationEngine() {
    _propagationEngine = new Propagation_Engine(_modelType, _fleet.get());
}

LineMap Propagation_Engine::PEmodel_computing1D(Transmitter_PE_data PEdata, EnvironmentConfig env, double reciever_antenna_height) {
    // 初始化大气模型：蒸发波导高度 20m
    // 对应 Paper 2 Fig. 6(d) 和 Eq. (35)
    AtmosphereModel atm(env.duct_height);
    JONSWAPSurfaceGenerator surface(env.wind_speed);
    // 预计算折射率剖面 (Profile)
    // 这一步非常重要，避免在 step 循环中重复计算 log 函数，提升效率
    std::vector<double> n_profile(env.nz);
    for (int i = 0; i < env.nz; ++i) {
        double z = i * env.dz;
        n_profile[i] = atm.getRefractiveIndex(z);
    }

    // 初始化求解器
    PEModel solver(PEdata.centralF_Ghz, env.dx, env.dz, env.nz);

    // 初始化高斯波束：天线高度 25m
    solver.initializeGaussian(PEdata.antenna_height, PEdata.beamWidth_deg
        , PEdata.antennaPhi_deg);

    // 开始步进仿真
    //std::cout << "Range(km) \t Loss(dB) \t (Atmosphere: Evaporation Duct 20m)" << std::endl;
    std::ofstream out("PEmodel_computing1D.csv");
    EMC_Engine::writeCSVRow(out, "Range", "Loss");
    //r为仿真距离（剖面）
    for (double r = env.dx; r < env.max_range; r += env.dx) {
        // 将预计算好的大气剖面传递给求解器
        //solver.step_Miller_Brown(r, PEdata.wind_speed, n_profile);
        solver.step_PLST(r, n_profile, surface, 0);

        // 输出数据
        if (std::abs(fmod(r, 1000.0)) < 0.1) {
            // 获取接收天线高度 15m 处的损耗
            int rx_idx = static_cast<int>(reciever_antenna_height / env.dz);
            double loss = solver.getPathLoss(rx_idx, r);
            EMC_Engine::writeCSVRow(out, r, loss);
            _LossLine.push_back(loss);
        }
    }
    return _LossLine;
}

GridMatrix Propagation_Engine::PEmodel_computing2D(Transmitter_PE_data PEdata, EnvironmentConfig env, double reciever_antenna_height) {
    spdlog::info("Starting 2D PE model computation for equipment: {}", PEdata.equipmenName);
    spdlog::info("Parameters: Frequency = {} GHz, Max Range = {} m, Duct Height = {} m, Wind Speed = {} m/s",
        PEdata.centralF_Ghz, env.max_range, env.duct_height, env.wind_speed);
    // 1. 定义地图网格参数
    double map_size_m = env.max_range;
    double grid_res_m = env.dx; 
    const int grid_dim = static_cast<int>(map_size_m / grid_res_m);

    // 角度与距离参数
    const int num_angles = 360 / env.angle_step_deg;
    const int num_ranges = static_cast<int>(env.max_range / env.dx);

    // 默认底噪值 (dB)
    const double noise_floor = -200.0;

    // 2. 准备环境
    AtmosphereModel atm(env.duct_height);
    JONSWAPSurfaceGenerator surface(env.wind_speed);
    // 预计算 n_profile 折射率
    std::vector<double> n_profile(env.nz);
#pragma omp parallel for
    for (int i = 0; i < env.nz; ++i) {
        n_profile[i] = atm.getRefractiveIndex(i * env.dz);
    }
    Eigen::MatrixXd polar_matrix(num_angles, num_ranges);
    polar_matrix.setConstant(noise_floor); // 初始化

    spdlog::info("Phase 1: Computing Polar Scan...");
    // 创建每个线程专用的 solver 池，避免在并行循环中反复创建/销毁 FFTW 计划
    int max_threads = omp_get_max_threads();
    std::vector<std::unique_ptr<PEModel>> solvers;
    solvers.reserve(max_threads);
    for (int t = 0; t < max_threads; ++t) {
        auto s = std::make_unique<PEModel>(PEdata.centralF_Ghz, env.dx, env.dz, env.nz);
        // 预初始化高斯波束
        s->initializeGaussian(PEdata.antenna_height, PEdata.beamWidth_deg, PEdata.antennaPhi_deg);
        solvers.push_back(std::move(s));
    }
    
#pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < num_angles; ++i) {
        int tid = omp_get_thread_num();
        PEModel* solver = solvers[tid].get();

        // 每个角度必须重置初始场
        solver->initializeGaussian(PEdata.antenna_height, PEdata.beamWidth_deg, PEdata.antennaPhi_deg);

        double az_deg = i * env.angle_step_deg;
        double az_rad = az_deg * M_PI / 180.0;
        int rx_z_idx = static_cast<int>(reciever_antenna_height / env.dz);

        // 沿径向步进 (Marching)
        int range_idx = 0;
        for (double r = env.dx; r < env.max_range && range_idx < num_ranges; r += env.dx) {

            // 执行一步 PE 运算
            solver->step_PLST(r, az_rad, n_profile, surface, 0.0);

            // 获取损耗并存入 Eigen 矩阵
            // 注意：Eigen 默认是 (row, col)
            polar_matrix(i, range_idx) = solver->getPathLoss(rx_z_idx, r);

            range_idx++;
        }
    }
    //std::ofstream out_polar("PE_Raw_Polar.csv");
    //EMC_Engine::writeCSVRow(out_polar, "Angle_Deg", "Range_m", "Loss_dB");

    //for (int i = 0; i < num_angles; ++i) {
    //    double az_deg = i * angle_step_deg;
    //    for (int j = 0; j < num_ranges; ++j) {
    //        double r_m = (j + 1) * PEdata.dx;
    //        double loss = polar_matrix(i, j);
    //        EMC_Engine::writeCSVRow(out_polar, az_deg, r_m, loss);
    //    }
    //}
    //out_polar.close();
    // 4. 坐标映射 (填满车轮空隙)
    GridMatrix cartesian_grid = GridMatrix::Constant(grid_dim, grid_dim, static_cast<int>(noise_floor));
    double center_idx = grid_dim / 2.0;
    const double deg_to_idx = 1.0 / env.angle_step_deg;
    const double inv_dx = 1.0 / env.dx;
    const double two_pi = 2.0 * M_PI;
#pragma omp parallel for collapse(2)
    for (int y = 0; y < grid_dim; ++y) {
        for (int x = 0; x < grid_dim; ++x) {
            // 1. 像素 -> 物理坐标 (m)
            double px = (x - center_idx) * grid_res_m;
            double py = (y - center_idx) * grid_res_m; // 假设 y 向上为正，若绘图库相反需调整

            // 2. 物理坐标 -> 极坐标 (r, theta)
            double r = std::hypot(px, py); // 更快更安全的 sqrt(x^2+y^2)

            // 超出最大射程直接跳过 (保留默认值)
            if (r >= env.max_range) continue;

            double theta = std::atan2(py, px); // (-PI, PI]
            if (theta < 0) theta += two_pi;    // [0, 2PI)

            // 3. 极坐标 -> 索引 (最近邻插值 Nearest Neighbor)
            // 角度索引
            double az_deg = theta * 180.0 / M_PI;
            int az_idx = static_cast<int>(std::round(az_deg * deg_to_idx));
            if (az_idx >= num_angles) az_idx = 0; // 处理 360度

            // 距离索引
            int r_idx = static_cast<int>(r * inv_dx);

            // 边界检查并赋值
            if (r_idx >= 0 && r_idx < num_ranges) {
                // 读取 double，转为 int 存入结果矩阵
                cartesian_grid(y, x) = static_cast<int>(polar_matrix(az_idx, r_idx));
            }
        }
    }

    return cartesian_grid; 
}


std::vector<Transmitter_PE_data> EMC_Engine::EquipmentConvertToMatrix(Fleet* fleet) {
    std::vector<Transmitter_PE_data> pe_data_list;
    spdlog::info("Converting Fleet to PE_data list...");
    for (const auto& ship_ptr : fleet->getShips()) {
        for (const auto& equip_ptr : ship_ptr->getEquipmentList()) {
            Transmitter_PE_data data;
            if(equip_ptr->getType() == EquipmentType::TRANSMITTER || equip_ptr->getType() == EquipmentType::TRANSCEIVER) {
                Transmitter* transmitter_ptr = dynamic_cast<Transmitter*>(equip_ptr.get());
                data.equipmenName = transmitter_ptr->getID();
                data.antennaType = transmitter_ptr->getAntennaType();
                data.power_dbm = transmitter_ptr->getPowerDBm();
                data.antenna_height = transmitter_ptr->getHeight() + ship_ptr->getHeight();
                data.beamWidth_deg = transmitter_ptr->getBeamWidth();
                data.antennaPhi_deg = transmitter_ptr->getAntennaPhi();
                data.centralF_Ghz = transmitter_ptr->getFrequencyGHz();
                pe_data_list.push_back(data);
                spdlog::info("Added Transmitter data for equipment: {}", data.equipmenName);
            } else {
                spdlog::warn("Skipping Reciever(or else) equipment: {}", equip_ptr->getID());
            }
        }
    }
    return pe_data_list;
}
