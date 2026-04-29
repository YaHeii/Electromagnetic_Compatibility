#include "EMC_Engine.h"
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include <spdlog/spdlog.h>
#include "Interface/TransferToPEdata.hpp"
#include "Utils/conversions.h"

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
    if (isStopRequested) {
        spdlog::info("Computing aborted by user.");
        return; // 直接退出函数
    }
    if (!_fleet) {
        spdlog::error("Fleet is null, cannot perform PE computing.");
        return;
    }
    // 将所有船只和设备转换为 PE_data 列表
    _peDataList = EquipmentConvertToMatrix(_fleet.get());
    if(_peDataList.size() == 0) {
        spdlog::warn("No equipment data found in fleet, PE computing will be skipped.");
        return;
	}
    // 这里可以根据 pe_data 的参数调整接收天线高度，暂时固定为 25.0
    // TODO: 设置reciever_PE_data, 支持针对不同接收设备来做PE计算
    double receiver_height = 25.0;

    // XXX: 此处使用预计算来分配尺寸，优化使用参数分配
    auto first_loss_mat = _propagationEngine->PEmodel_computing2D(_peDataList[0], _env, receiver_height);
    int rows = static_cast<int>(first_loss_mat.rows());
    int cols = static_cast<int>(first_loss_mat.cols());
    // 初始化线性功率累加矩阵 (单位: mW)
    Eigen::MatrixXd total_power_mw = Eigen::MatrixXd::Zero(rows, cols);
    
    // 计算二维损耗网格
    for (auto& pe_data : _peDataList) {
        if (isStopRequested) return;
        Eigen::MatrixXd current_loss = _propagationEngine->PEmodel_computing2D(pe_data, _env, receiver_height);
        Eigen::MatrixXd current_tx_dbm = pe_data.power_dbm - current_loss.array();
        // [在线性空间 (mW) 进行累加，避免 dBm 直接相加的错误
        accumulatePowerLinear(total_power_mw, current_tx_dbm);

        pe_data.PowerGrid = eigen_to_vector(current_loss);

        //spdlog::info("TESTING output PowerGrid, mustbe deleted while running: {}", pe_data.PowerGrid[5][5]);
    }
    // 将总线性功率转回 dBm 存入 _LossGrid
    Eigen::MatrixXd final_total_dbm = total_power_mw.unaryExpr([](double mw) {
        return mwToDbm(mw);
    });
    _LossGrid = eigen_to_vector(final_total_dbm);

    // 数据落地

    std::ofstream out("PEcomputing_LinearAggregated.csv");
    spdlog::info("PEcomputing result will be saved in PEcomputing_LinearAggregated.csv");
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            // XXX: writeCSV函数对于逗号和换行的处理并不完善，考虑引入第三方库
            writeCSVRow(out, final_total_dbm(i, j));
            writeCSVRow(out, ",");

        }
        writeCSVRow(out, "\n");
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
    _env.maxRange = 20000.0;      // 20km
    _env.ductHeight = 20.0;       // 蒸发波导
    _env.windSpeed = 7.0;

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
    _env.maxRange = 10000.0;   // 10km 足够看清干涉
    _env.ductHeight = 0.0;     // 无波导 (标准大气)
    _env.windSpeed = 0.001;    // 近乎静止的平坦海面

    // 2. 初始化环境
    AtmosphereModel atm(0.0);
    JONSWAPSurfaceGenerator surface(_env.windSpeed);
    std::vector<double> n_profile(_env.nz, 1.0);

    // 3. 运行 PE
    PEModel solver(data.centralF_Ghz, _env.dx, _env.dz, _env.nz);
    //solver.initializeGaussian(data.antenna_height, data.beamWidth_deg, data.antennaPhi_deg);

    // 4. 准备导出数据
    std::ofstream out("validation_data.csv");
    out << "Range_m,PE_Loss_dB,Theory_Loss_dB\n";

    double receiver_h = 15.0; // hr = 15m

    for (double r = _env.dx; r < _env.maxRange; r += _env.dx) {
        // 使用 PLST 步进 (由于 wind=0，PLST 应当退化为标准平坦 PE)
        //solver.step_PLST(r, 0.0, n_profile, surface, 0.0);

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
    _env.maxRange = 20000.0;    // 20 km
    _env.dx = 10.0; _env.dz = 0.1; _env.nz = 2048;
    _env.windSpeed = 10.0;      // 【关键】较大的风速，确保粗糙度效应明显
    _env.ductHeight = 0.0;      // 暂时关闭波导，专注于表面散射验证

    // 2. 初始化环境
    AtmosphereModel atm(_env.ductHeight);
    JONSWAPSurfaceGenerator surface(_env.windSpeed);
    std::vector<double> n_profile(_env.nz, 1.0); // 标准大气 n=1

    PEModel solver_mb(data.centralF_Ghz, _env.dx, _env.dz, _env.nz); // Solver A: Miller-Brown
    PEModel solver_plst(data.centralF_Ghz, _env.dx, _env.dz, _env.nz); // Solver B: PLST

    //// 初始化同样的高斯波束
    //solver_mb.initializeGaussian(data.antenna_height, 2.0, 0.0);
    //solver_plst.initializeGaussian(data.antenna_height, 2.0, 0.0);

    // 3. 运行对比仿真
    std::ofstream out("validation_roughness.csv");
    out << "Range_m,Loss_MillerBrown,Loss_PLST\n";

    double rx_h = 10.0;
    int rx_idx = rx_h / _env.dz;

    for (double r = _env.dx; r < _env.maxRange; r += _env.dx) {
        // --- A. 运行 Miller-Brown (基准) ---
        // 注意：你需要确保 step_Miller_Brown 被正确声明为 public
        solver_mb.step_Miller_Brown(r, _env.windSpeed, n_profile);
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
    _env.maxRange = 50000.0;    // 【关键】距离要足够远 (50km)，累积泄漏才明显
    _env.dx = 10.0;
    _env.dz = 0.1;
    _env.nz = 2048;              // 计算高度约 200m，足以覆盖波导层(20m)和泄漏层(>20m)
    _env.ductHeight = 20.0;     // 【关键】20米强蒸发波导

    // 2. 准备两种场景
    // 场景 A: 平静海面 (Wind = 0) -> 能量应该被死死锁住
    double wind_flat = 0.0;
    // 场景 B: 粗糙海面 (Wind = 15) -> 能量应该被散射出去
    double wind_rough = 15.0;

    // 3. 初始化环境
    // 大气模型是一样的 (都是 20m 波导)
    AtmosphereModel atm(_env.ductHeight);
    std::vector<double> n_profile(_env.nz);
    for (int i = 0; i < _env.nz; ++i) n_profile[i] = atm.getRefractiveIndex(i * _env.dz);

    // 表面生成器
    JONSWAPSurfaceGenerator surf_flat(wind_flat);
    JONSWAPSurfaceGenerator surf_rough(wind_rough);

    // 求解器
    PEModel solver_flat(data.centralF_Ghz, _env.dx, _env.dz, _env.nz);
    PEModel solver_rough(data.centralF_Ghz, _env.dx, _env.dz, _env.nz);

    //solver_flat.initializeGaussian(data.antenna_height, 2.0, 0.0);
    //solver_rough.initializeGaussian(data.antenna_height, 2.0, 0.0);

    // 4. 运行仿真到最大距离
    // 这里我们不需要中间数据，只需要跑到终点看垂直分布
    int steps = static_cast<int>(_env.maxRange / _env.dx);

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
        double loss_flat = solver_flat.getPathLoss(i, _env.maxRange);
        double loss_rough = solver_rough.getPathLoss(i, _env.maxRange);

        // 过滤掉极其微弱的数值(比如高空吸收层)，只输出有效数据
        // 或者直接输出，用Excel筛选
        if (z < 150.0) { // 只关注 150m 以下，太高了是吸收层
            out << z << "," << loss_flat << "," << loss_rough << "\n";
        }
    }
    out.close();
    spdlog::info("Validation data saved to validation_leakage.csv");
}



LineMap Propagation_Engine::PEmodel_computing1D(Transmitter_PE_data PEdata, EnvironmentData env, double reciever_antenna_height) {
    // 初始化大气模型：蒸发波导高度 20m
    // 对应 Paper 2 Fig. 6(d) 和 Eq. (35)
    AtmosphereModel atm(env.ductHeight);
    JONSWAPSurfaceGenerator surface(env.windSpeed);
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
    //solver.initializeGaussian(PEdata.antenna_height, PEdata.beamWidth_deg
    //    , PEdata.antennaPhi_deg);

    // 开始步进仿真
    //std::cout << "Range(km) \t Loss(dB) \t (Atmosphere: Evaporation Duct 20m)" << std::endl;
    std::ofstream out("PEmodel_computing1D.csv");
    writeCSVRow(out, "Range", "Loss");
    //r为仿真距离（剖面）
    for (double r = env.dx; r < env.maxRange; r += env.dx) {
        // 将预计算好的大气剖面传递给求解器
        //solver.step_Miller_Brown(r, PEdata.wind_speed, n_profile);
        solver.step_PLST(r, n_profile, surface, 0);

        // 输出数据
        if (std::abs(fmod(r, 1000.0)) < 0.1) {
            // 获取接收天线高度 15m 处的损耗
            int rx_idx = static_cast<int>(reciever_antenna_height / env.dz);
            double loss = solver.getPathLoss(rx_idx, r);
            writeCSVRow(out, r, loss);
            _LossLine.push_back(loss);
        }
    }
    return _LossLine;
}

// XXX:目前使用笛卡尔坐标系反向映射极坐标数据
GridMatrix Propagation_Engine::PEmodel_computing2D(Transmitter_PE_data PEdata, EnvironmentData env, double reciever_antenna_height) {
    spdlog::info("Starting 2D PE model computation for equipment: {}, on {}", PEdata.equipmenName, PEdata.shipName);
    // 1. 定义地图网格参数
    // XXX: 是否解耦最大辐射半径和地图半径
    double map_width_m = env.maxRange; // 示例：生成足够大的地图
    double map_height_m = env.maxRange;

    int grid_w = static_cast<int>(map_width_m / env.dx);
    int grid_h = static_cast<int>(map_height_m / env.dx);

    // 角度与距离参数
    const int num_angles = 360 / env.angleStepDeg;
    const int num_ranges = static_cast<int>(env.maxRange / env.dx);

    // 默认底噪值 (dB)
    const double noise_floor = 200;
	// 坐标原点 (0,0) 对应 output_grid(0,0),位于左上角，向右是X正方向，向下是Y正方向
    GridMatrix output_grid = GridMatrix::Constant(grid_h, grid_w, noise_floor);

    // 预计算常量以加速循环
    const double inv_dx = 1.0 / env.dx;
    const double deg_to_idx = 1.0 / env.angleStepDeg;
    const double two_pi = 2.0 * M_PI;

    // 发射机绝对坐标
    const double tx_x = PEdata.X_offset;
    const double tx_y = PEdata.Y_offset;

    // 2. 准备环境
    AtmosphereModel atm(env.ductHeight);
    JONSWAPSurfaceGenerator surface(env.windSpeed);

    Eigen::MatrixXd polar_matrix(num_angles, num_ranges);
    polar_matrix.setConstant(noise_floor); // 初始化

    // 3. 创建每个线程专用的 solver 池，避免在并行循环中反复创建/销毁 FFTW 计划
    int max_threads = omp_get_max_threads();
    std::vector<std::unique_ptr<PEModel>> solvers;
    solvers.reserve(max_threads);
    static std::mutex fftw_mutex;
    for (int t = 0; t < max_threads; ++t) {
        std::lock_guard<std::mutex> lock(fftw_mutex); // 保护 FFTW 计划创建
        auto s = std::make_unique<PEModel>(PEdata.centralF_Ghz, env.dx, env.dz, env.nz);
        solvers.push_back(std::move(s));
    }
    // 4. 角度并行计算
#pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < num_angles; ++i) {
        int tid = omp_get_thread_num();
        PEModel* solver = solvers[tid].get();

        double az_deg = i * env.angleStepDeg;
        double az_rad = az_deg * M_PI / 180.0;
        double cos_az = std::cos(az_rad);
        double sin_az = std::sin(az_rad);

        // 每个角度必须重置初始场
        // 计算起始点 (r=0) 的海面高度，修正初始场高度
        double tx_h_surface = surface.getSurfaceHeight(PEdata.X_offset, PEdata.Y_offset, 0.0);
        solver->initializeGaussian(PEdata.antenna_height, tx_h_surface, PEdata.beamWidth_deg, PEdata.antennaPhi_deg);

        // 沿径向步进 (Marching)
        int range_idx = 0;
        for (double r = env.dx; r < env.maxRange && range_idx < num_ranges; r += env.dx) {

            // 执行一步 PE 运算
            solver->step_PLST(r, az_rad, atm, surface, 0.0);
            // 计算当前点 (相对于Tx距离r) 的绝对物理坐标
            double current_x_world = PEdata.X_offset + r * cos_az;
            double current_y_world = PEdata.Y_offset + r * sin_az;
            // 动态高度跟踪
            // 计算当前距离 r 处的海面高度 h(r)
            double h_r = surface.getSurfaceHeight(current_x_world, current_y_world, 0.0);
            // 接收机在 PLST 网格中的相对高度 zeta = z_phys - h(r)
            double zeta_rx = reciever_antenna_height - h_r;
            int rx_z_idx = static_cast<int>(zeta_rx / env.dz);
            // 边界检查：如果接收机被海浪淹没
            if (rx_z_idx >= 0 && rx_z_idx < env.nz) {
                polar_matrix(i, range_idx) = solver->getPathLoss(rx_z_idx, r);
            }
            else {
                polar_matrix(i, range_idx) = noise_floor;
            }
            range_idx++;

        }
    }

	// 5. 极坐标 -> 笛卡尔坐标映射
#pragma omp parallel for collapse(2)
    for (int y = 0; y < grid_h; ++y) {
        for (int x = 0; x < grid_w; ++x) {
            // A. 当前像素的绝对物理坐标 (World Coordinates)
             // 假设 (0,0) 在左上角，X向右，Y向下 (图像坐标系)
             // 如果是地理坐标系 Y向上，则 py = (grid_h - y) * env.dx
            double px_world = x * env.dx;
            double py_world = y * env.dx;

            // B. 计算相对于发射机的矢量 (Relative Vector)
            double dx = px_world - tx_x;
            double dy = py_world - tx_y; // 注意坐标系方向一致性即可

            // C. 转换为极坐标 (r, theta)
            double r = std::hypot(dx, dy);

            // D. 范围检查 (超出射频范围直接跳过)
            if (r >= env.maxRange || r < env.dx) continue;

            // E. 计算角度
            // atan2 返回 (-PI, PI]，0 指向 X 正轴
            double theta = std::atan2(dy, dx);
            if (theta < 0) theta += two_pi; // 归一化到 [0, 2PI)

            // F. 采样 (Nearest Neighbor)
            // --- 双线性插值 (Bilinear Interpolation) 开始 ---

            // 计算浮点索引
            double az_float = theta * 180.0 / M_PI * deg_to_idx;
            double r_float = r * inv_dx;

            // 获取四个相邻点的整数索引
            int a0 = static_cast<int>(std::floor(az_float));
            int a1 = (a0 + 1) % num_angles; // 角度循环衔接 359 -> 0
            int r0 = static_cast<int>(std::floor(r_float));
            int r1 = r0 + 1;

            // 边界保护
            if (r1 >= num_ranges) r1 = r0;

            // 计算权重
            double wa = az_float - a0; // 角度方向权重
            double wr = r_float - r0;  // 距离方向权重

            // 采样四个点
            double v00 = polar_matrix(a0, r0);
            double v10 = polar_matrix(a1, r0);
            double v01 = polar_matrix(a0, r1);
            double v11 = polar_matrix(a1, r1);

            // 双线性插值公式
            // 先在角度方向插值
            double v_r0 = v00 * (1 - wa) + v10 * wa;
            double v_r1 = v01 * (1 - wa) + v11 * wa;
            // 再在距离方向插值
            double val = v_r0 * (1 - wr) + v_r1 * wr;

            output_grid(y, x) = val;
            // --- 双线性插值 结束 ---
        }
    }

    return output_grid;
}


