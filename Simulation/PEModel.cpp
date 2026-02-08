#pragma once
#include "PEModel.h"
#include <random>
#include <chrono>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void JONSWAPSurfaceGenerator::generateSpectrumComponents() {
    int m_freq_bins = 50; // 频率划分数
    int n_angle_bins = 20; // 角度划分数
	_components.reserve(m_freq_bins * n_angle_bins);// 预分配空间
    std::vector<double> rand_vals(m_freq_bins * n_angle_bins);
    std::vector<double> phase_vals(m_freq_bins * n_angle_bins);
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::default_random_engine gen(seed);
    // 计算波峰频率 omega_m 
    // U 10 为海平面以上 10 米处的风速。
    // X_tilde为无量纲区域长度
    double g = GRAVITY;
    double X_tilde = g * _wind_fetch_X / std::pow(_wind_speed_U10, 2);
    double omega_m = 22.0 * (g / _wind_speed_U10) * std::pow(X_tilde, -0.33);
    // delta_omega为分频间隔
    // omega_L 为频率下限
    // omega_H 为频率上限
	// 频率范围通常取为 omega_m 的 0.5 到 3 倍(是否正确？影响如何？)
    double omega_L = 0.5 * omega_m; 
    double omega_H = 3.0 * omega_m; 
    double delta_omega = (omega_H - omega_L) / m_freq_bins;

    double theta_min = -M_PI / 2.0;
    double theta_max = M_PI / 2.0;
    double delta_theta = (theta_max - theta_min) / n_angle_bins;
    // 随机数生成器
    // default_random_engine 默认随机数
    // uniform_real_distribution 均匀分布浮点数
    std::uniform_real_distribution<double> dist_phase(0.0, 2.0 * M_PI);
    std::uniform_real_distribution<double> dist_rand(0.0, 1.0);

    for (int i = 1; i <= m_freq_bins; ++i) {
        // 迭代中心频率 omega_i
        double omega_i = omega_L + (i - 0.5) * delta_omega;

        // JONSWAP S(omega) 计算 
        // σ 为波形因子
        // α_S 为波能量因子
        // gamma_peak为峰值升力因子
        // S_omega为谱函数
        double sigma = (omega_i <= omega_m) ? 0.07 : 0.09;
        double alpha_s = 0.076 * std::pow(X_tilde, -0.22);
        double exponent1 = -1.25 * std::pow(omega_m / omega_i, 4);
        double exponent2 = -std::pow(omega_i - omega_m, 2) / (2 * sigma * sigma * omega_m * omega_m);
        double gamma_peak = 3.3; 

        double S_omega = alpha_s * g * g * std::pow(omega_i, -5) * std::exp(exponent1) * std::pow(gamma_peak, std::exp(exponent2));

        for (int j = 1; j <= n_angle_bins; ++j) {
            double theta_j = theta_min + (j - 0.5) * delta_theta;

            // s_param为方向分布的集中参数
            // G_theta 为 方向扩频函数
            // G0为G_theta 系数
            double s_param = (omega_i <= omega_m) ? 10.0 * std::pow(omega_i / omega_m, 5) : 10.0 * std::pow(omega_i / omega_m, -2.5); 
            double G0 = 1.0 / M_PI * std::pow(2, 2 * s_param - 1) * std::pow(std::tgamma(s_param + 1),2) / std::tgamma(2 * s_param + 1);
            double G_theta = G0 * std::pow(std::cos(theta_j / 2.0), 2.0 * s_param);

            // S_total为三维随机波的方向谱
            double S_total = S_omega * G_theta;

            _WaveComponent wc;
            wc.amplitude = std::sqrt(2.0 * S_total * delta_omega * delta_theta); // [cite: 781]
            // 加入随机扰动到频率
            double rand_val = dist_rand(gen);
            wc.omega = omega_i + (j - 1 + rand_val) * delta_omega / n_angle_bins;
            wc.k = wc.omega * wc.omega / g; // 深水近似 k = w^2/g
            wc.angle = theta_j;
            wc.phase = dist_phase(gen);
            wc.kx = wc.k * std::cos(wc.angle);
            wc.ky = wc.k * std::sin(wc.angle);
            _components.push_back(wc);
        }
    }
}

//性能瓶颈，使用openMp优化
double JONSWAPSurfaceGenerator::getSurfaceHeight(double x, double y, double t) {
    double eta = 0.0;
    // 假设传播方向沿 x 轴，Vx=Vy=0 (静止海面快照)
#pragma omp parallel for reduction(+:eta)
    for (int i = 0; i < _components.size(); ++i) {
        const auto& wc = _components[i];
        double phase_term = wc.kx * x + wc.ky * y - wc.omega * t + wc.phase;
        eta += wc.amplitude * std::cos(phase_term);
    }
    return eta;
}

void PEModel::setupAbsorber() {
    _absorber.resize(_nz);
    int layer_thickness = _nz / 8; // 顶部 1/4 作为吸收层

    for (int i = 0; i < _nz; ++i) {
        if (i < _nz - layer_thickness) {
            _absorber[i] = 1.0;
        }
        else {
            // 使用平滑的汉宁窗 (Hanning Window) 衰减至 0
            double ratio = (double)(i - (_nz - layer_thickness)) / layer_thickness;
            _absorber[i] = 0.5 * (1.0 + cos(M_PI * ratio));
        }
    }
}

void PEModel::precomputeDiffraction() {
    // 采用宽角抛物线方程 (Wide-Angle PE) 的衍射项
    // 衍射部分近似: sqrt(k^2 - p^2) - k
    // p = kz
    _diffraction_term.resize(_fft_size);
    double dk_z = 2.0 * M_PI / (_fft_size * _dz);
#pragma omp parallel for
    for (int i = 0; i < _fft_size; ++i) {
        double kz;
        // FFTW 频率排序: [0, 1, ... N/2, -N/2, ... -1]
        if (i <= _fft_size / 2) kz = i * dk_z;
        else kz = (i - _fft_size) * dk_z;
        double p2 = kz * kz;
        double k2 = _k0 * _k0;
        // 窄角衍射因子 exp(-i * kz^2 * dx / 2k) [cite: 669]
        // 宽角近似 (Feit-Fleck) 传播因子 [cite: 276]
        // sqrt(k^2 - p^2) - k
        // 注意：当 p^2 > k^2 时（倏逝波），根号内为负，变为纯虚数衰减
        Complex propagator;
        if (k2 >= p2) {
            double sqrt_val = std::sqrt(k2 - p2);
            propagator = std::exp(J * _dx * (sqrt_val - _k0));
        }
        else {
            // 倏逝波处理：迅速衰减
            double sqrt_val = std::sqrt(p2 - k2);
            propagator = std::exp(-_dx * (sqrt_val)-J * _dx * _k0);
        }
        _diffraction_term[i] = propagator;
    }
}

double PEModel::calculateRoughnessRho(double wind_speed, double grazing_angle_rad) {
    // 均方根波高
    double h_rms = 0.0051 * pow(wind_speed, 2);
    // 瑞利粗糙度参数
    double gamma = 2.0 * _k0 * h_rms * sin(grazing_angle_rad);
    // Miller-Brown 修正因子
    // rho = exp(-0.5 * gamma^2) * I0(0.5 * gamma^2)
    double g2_half = 0.5 * gamma * gamma;
    double I0 = std::cyl_bessel_i(0.0, g2_half);

    return exp(-g2_half) * I0;
}

Complex PEModel::calculateFresnel(double grazing_angle_rad, Complex epsilon_sea) {
    double sin_theta = sin(grazing_angle_rad);
    double cos_theta = cos(grazing_angle_rad);

    // 水平极化公式
    Complex numerator = sin_theta - std::sqrt(epsilon_sea - pow(cos_theta, 2));
    Complex denominator = sin_theta + std::sqrt(epsilon_sea - pow(cos_theta, 2));
    return numerator / denominator;
}

void PEModel::step_Miller_Brown(double current_range, double wind_speed, const std::vector<double>& n_profile) {
    // 使用 Eigen Map 直接操作 FFTW 的内存，代码更简洁且利于编译器优化
    using Eigen::VectorXcd;
    using Eigen::Map;

    // 映射 FFTW 输入/输出缓冲区为 Eigen 向量
    Map<VectorXcd> u_space(reinterpret_cast<Complex*>(_in_ptr), _fft_size);
    Map<VectorXcd> u_kspace(reinterpret_cast<Complex*>(_out_ptr), _fft_size);
    // --- 步骤 A: 空间域处理 (折射 + 吸收 + 边界构建) ---

    // 1. 估算掠射角 (简单的几何近似，实际应参考 Paper 2 使用谱估计)
    double grazing_approx = atan(25.0 / (current_range + 1000.0)); // 假设天线高25m

    // 2. 计算底部边界参数
    double rho = calculateRoughnessRho(wind_speed, grazing_approx); // 粗糙度 
    Complex eps_sea(80.0, -4.0 * M_PI * 4.0 / (_k0 * 299792458.0)); // 海水介电常数示例
    Complex Gamma_f = calculateFresnel(grazing_approx, eps_sea);    // 菲涅尔

    // Paper 2 Eq. (93): 有效反射系数 Gamma_eff = rho * Gamma_Fresnel
    Complex Gamma_eff = rho * Gamma_f;

    // 3. 填充物理空间 (0 ~ nz-1)
#pragma omp parallel for
    for (int i = 0; i < _nz; ++i) {
        double n = n_profile[i];
        // 论文公式 (33) 折射项: sqrt(n^2 - sin^2(beta)) - 1
        // 在 Miller-Brown (平坦假设) 下，beta = 0，退化为 n - 1
        Complex refraction = std::exp(J * _k0 * (n - 1.0) * _dx);
        // 直接操作 Eigen 对象对应的内存
        u_space[i] *= refraction * _absorber[i];
    }

    // 4. [关键] 构建镜像空间 (nz ~ 2*nz-1) 实现海面边界
    // 逻辑: u(-z) = Gamma * u(z)
    // 在 FFTW 数组中，后半部分对应负坐标（由于周期性）
#pragma omp parallel for
    for (int i = 1; i < _nz; ++i) {
        // 镜像点赋值
        u_space[_fft_size - i] = Gamma_eff * u_space[i];
    }
    u_space[0] *= (1.0 + Gamma_eff);

    // --- 步骤 B: 变换到波数域 (FFT) ---
    fftw_execute(_plan_fwd);

    // --- 步骤 C: 波数域衍射 (Eigen 写法) ---
        // 这种写法会自动调用向量化指令 (SIMD)
        // Map<VectorXcd> diff_vec(_diffraction_term.data(), _fft_size);
        // u_kspace.cwiseProduct(diff_vec); // 这样写如果有别名问题需小心

        // 安全且并行的写法：
#pragma omp parallel for
    for (int i = 0; i < _fft_size; ++i) {
        u_kspace[i] *= _diffraction_term[i];
    }


    // --- 步骤 D: 变回空间域 (IFFT) ---
    fftw_execute(_plan_bwd);

    // --- 步骤 E: 归一化 ---
    u_space /= (double)_fft_size;
}

void PEModel::step_PLST(double current_range, const std::vector<double>& n_profile, JONSWAPSurfaceGenerator& surface_gen, double time_sec) {
    // 使用 Eigen Map 直接操作 FFTW 的内存
    using Eigen::VectorXcd;
    using Eigen::Map;

    // 1. 获取地形几何信息 (计算斜率 beta)
    // 论文方法：分段线性位移变换 [cite: 265]
    // 需要计算当前位置 x 和下一步 x+dx 之间的高差来确定斜率
    // 注意：这里的 y 设为 0，假设我们在 x-z 剖面计算
    double z_curr = surface_gen.getSurfaceHeight(current_range, 0.0, time_sec);
    double z_next = surface_gen.getSurfaceHeight(current_range + _dx, 0.0, time_sec);

    // T' = tan(beta) = dz/dx
    double slope = (z_next - z_curr) / _dx;
    double beta = std::atan(slope); // 地形倾角
    double sin_beta = std::sin(beta);
    double cos_beta = std::cos(beta);
    double sin_sq_beta = sin_beta * sin_beta;
    double cos_sq_beta = cos_beta * cos_beta;

    // 映射 FFTW 内存
    Map<VectorXcd> u_space(reinterpret_cast<Complex*>(_in_ptr), _fft_size);
    Map<VectorXcd> u_kspace(reinterpret_cast<Complex*>(_out_ptr), _fft_size);

    // --- 步骤 A: 空间域处理 (修正后的折射 + 修正后的边界) ---

    // 2. 修正边界条件 (PLST Modified Impedance/Reflection)
    // 估算基础掠射角 (几何近似)
    double grazing_geom = std::atan(25.0 / (current_range + 1000.0)); // 假设高度25m

    // [关键] 局部掠射角修正
    // 论文指出 Leontovich 边界条件随 beta 改变 
    // 工程实现：局部入射角 = 几何掠射角 + 地形倾角
    double grazing_local = grazing_geom + beta;

    // 防止掠射角过小或反向 (背坡遮挡)
    if (grazing_local < 1e-6) grazing_local = 1e-6;

    // 计算修正后的有效反射系数
    Complex eps_sea(80.0, -4.0 * M_PI * 4.0 / (_k0 * 299792458.0));
    Complex Gamma_PLST = calculateFresnel(grazing_local, eps_sea);
    // 注意：PLST 已经通过网格变形处理了粗糙度带来的相位偏移，
    // 所以这里通常不再乘 Miller-Brown 的 rho，除非模拟亚网格尺度的微粗糙度。
    // 如果遵循纯 PLST 几何光学逻辑，这里仅使用 Fresnel。

    // 3. 应用 PLST 修正后的折射因子
    // 公式 (33) 折射部分: exp(i * k * dx * (sqrt(n^2 - sin^2(beta)) - 1)) 
#pragma omp parallel for
    for (int i = 0; i < _nz; ++i) {
        double n = n_profile[i];
        double n2 = n * n;

        Complex refraction_term;
        double val = n2 - sin_sq_beta;

        // 处理根号内正负 (虽然大气中 n~1, beta 通常很小，val > 0)
        if (val >= 0) {
            // 标准传播相位
            refraction_term = std::exp(J * _k0 * _dx * (std::sqrt(val) - 1.0));
        }
        else {
            // 倏逝波衰减 (理论上不应发生在大气波导计算中)
            refraction_term = std::exp(-_k0 * _dx * std::sqrt(-val) - J * _k0 * _dx);
        }

        u_space[i] *= refraction_term * _absorber[i];
    }

    // 4. 应用镜像法 (使用修正后的反射系数)
#pragma omp parallel for
    for (int i = 1; i < _nz; ++i) {
        u_space[_fft_size - i] = Gamma_PLST * u_space[i];
    }
    u_space[0] *= (1.0 + Gamma_PLST);

    // --- 步骤 B: 变换到波数域 (FFT) ---
    fftw_execute(_plan_fwd);

    // --- 步骤 C: 波数域处理 (PLST 修正后的衍射) ---
    // 公式 (33) 衍射部分: exp(i * dx * (sqrt(k^2 * cos^2(beta) - p^2) - k)) 
    // 这一步必须动态计算，不能使用预计算的 _diffraction_term

    double dk_z = 2.0 * M_PI / (_fft_size * _dz);
    double k_eff_sq = _k0 * _k0 * cos_sq_beta; // k^2 * cos^2(beta)

#pragma omp parallel for
    for (int i = 0; i < _fft_size; ++i) {
        double kz;
        if (i <= _fft_size / 2) kz = i * dk_z;
        else kz = (i - _fft_size) * dk_z;

        double p2 = kz * kz;
        Complex diff_prop;

        // 宽角传播算子计算
        double val = k_eff_sq - p2;

        if (val >= 0) {
            // 传播波: sqrt(k_eff^2 - p^2) - k0
            // 注意：减去的是参考波数 k0，因为 SSFT 的 split 算子定义
            diff_prop = std::exp(J * _dx * (std::sqrt(val) - _k0));
        }
        else {
            // 倏逝波: i * (i * sqrt(p^2 - k_eff^2)) - i*k0
            // = -sqrt(...) - i*k0
            diff_prop = std::exp(-_dx * std::sqrt(-val) - J * _dx * _k0);
        }

        u_kspace[i] *= diff_prop;
    }

    // --- 步骤 D: 变回空间域 (IFFT) ---
    fftw_execute(_plan_bwd);

    // --- 步骤 E: 归一化 ---
    u_space /= (double)_fft_size;
}

//为了避免高性能计算中出现错误，所以选择复制粘贴，加入方位角影响
void PEModel::step_PLST(double current_range, double azimuth_rad, const AtmosphereModel& atm_model, JONSWAPSurfaceGenerator& surface_gen, double time_sec) {
    using Eigen::VectorXcd;
    using Eigen::Map;
	// 1. 获取地形几何信息 (计算斜率 beta)
    double cos_az = std::cos(azimuth_rad);
    double sin_az = std::sin(azimuth_rad);
    // 计算当前和下一步物理坐标
    double x_curr = current_range * cos_az;
    double y_curr = current_range * sin_az;
    double x_next = (current_range + _dx) * cos_az;
    double y_next = (current_range + _dx) * sin_az;
    // 获取本地海面高度 z(x)
    double z_curr = surface_gen.getSurfaceHeight(x_curr, y_curr, time_sec);
    double z_next = surface_gen.getSurfaceHeight(x_next, y_next, time_sec);
    // 计算斜率 beta: tan(beta) = (h_next - h_curr) / dx
    double slope = (z_next - z_curr) / _dx;
    double beta = std::atan(slope);
    double sin_beta = std::sin(beta);
    double cos_beta = std::cos(beta);
    double sin_sq_beta = sin_beta * sin_beta;
    double cos_sq_beta = cos_beta * cos_beta;
    // 映射 FFTW 内存为 Eigen 向量以利用 SIMD 优化
    Map<VectorXcd> u_space(reinterpret_cast<Complex*>(_in_ptr), _fft_size);
    Map<VectorXcd> u_kspace(reinterpret_cast<Complex*>(_out_ptr), _fft_size);
    // --- 步骤 A: 空间域处理 (修正后的折射项) ---
    // 估算局部掠射角（用于计算反射系数 Gamma）
    double grazing_geom = std::atan(25.0 / (current_range + 1000.0)); //TODO:优化假设值使用外部参数 假设高度25m
    double grazing_local = grazing_geom + beta;
    if (grazing_local < 1e-6) grazing_local = 1e-6; // 避免背坡数值异常

    Complex eps_sea(80.0, -4.0 * M_PI * 4.0 / (_k0 * 299792458.0));
    Complex Gamma_PLST = calculateFresnel(grazing_local, eps_sea);

#pragma omp parallel for
    for (int i = 0; i < _nz; ++i) {
        //double n = n_profile[i];
        //double n2 = n * n;
        // 物理高度 z = 网格高度 zeta + 当前海面高度 z_curr
        double zeta = i * _dz;
        double z_phys = zeta + z_curr;
        // 采样物理高度对应的大气折射率
        double n = atm_model.getRefractiveIndex(z_phys);
        double n2 = n * n;
        Complex refraction_term;
        // 计算 PLST 修正后的空间步进因子
        // Formula: exp(i * k0 * dx * (sqrt(n^2 - sin^2(beta)) - 1))
        double val = n2 - sin_sq_beta;
        if (val >= 0) {
            refraction_term = std::exp(J * _k0 * _dx * (std::sqrt(val) - 1.0));
        }
        else {
            refraction_term = std::exp(-_k0 * _dx * std::sqrt(-val) - J * _k0 * _dx);
        }

        u_space[i] *= refraction_term * _absorber[i];
    }
    // --- 步骤 B: 镜像法实现海面边界 (u(-zeta) = Gamma * u(zeta)) ---
#pragma omp parallel for
    for (int i = 1; i < _nz; ++i) {
        u_space[_fft_size - i] = Gamma_PLST * u_space[i];
    }
    u_space[0] *= (1.0 + Gamma_PLST); //表面交接处理
    // --- 步骤 C: 变换到波数域 (FFT) ---
    fftw_execute(_plan_fwd);
    // --- 步骤 D: 波数域衍射 (PLST 修正后的衍射算子) ---
    // Formula: exp(i * dx * (sqrt(k0^2 * cos^2(beta) - kz^2) - k0))
    double dk_z = 2.0 * M_PI / (_fft_size * _dz);
    double k_eff_sq = _k0 * _k0 * cos_sq_beta; // k^2 * cos^2(beta) 有效波数项

#pragma omp parallel for
    for (int i = 0; i < _fft_size; ++i) {
        double kz;
        if (i <= _fft_size / 2) kz = i * dk_z;
        else kz = (i - _fft_size) * dk_z;

        double p2 = kz * kz;
        Complex diff_prop;

        double val = k_eff_sq - p2;
        if (val >= 0) {
            diff_prop = std::exp(J * _dx * (std::sqrt(val) - _k0));
        }
        else {
            diff_prop = std::exp(-_dx * std::sqrt(-val) - J * _dx * _k0);
        }
        u_kspace[i] *= diff_prop;
    }
    // --- 步骤 E: 变回空间域并归一化 (IFFT) ---
    fftw_execute(_plan_bwd);
    u_space /= (double)_fft_size;
}

void PEModel::initializeGaussian(double antenna_phys_height, double h_start, double beamWidth_deg, double tilt_deg) {
    // 将波束宽度（角度）转换为高斯函数的空间宽度参数 w0
    // 公式推导：高斯波束的半功率波束宽度 (HPBW) 与 w0 的关系近似为 w0 = 2 / (k * sin(HPBW/2))
    double w0 = 2.0 / (_k0 * std::sin(beamWidth_deg * M_PI / 360.0));
    // 将仰角转换为弧度，用于计算相位
    double tilt_rad = std::sin(tilt_deg * M_PI / 180.0);
    // 计算天线在变换后的网格高度 zeta_a
    double zeta_a = antenna_phys_height - h_start;
    // 计算归一化系数
    // 使得 integral(|u|^2) = 1
    // 高斯积分公式: int exp(-2*(z-za)^2/w0^2) dz = w0 * sqrt(pi/2)
    double total_energy = w0 * std::sqrt(M_PI / 2.0);
    double norm_factor = 1.0 / std::sqrt(total_energy);

    for (int i = 0; i < _fft_size; ++i) {
        reinterpret_cast<Complex*>(_in_ptr)[i] = 0.0;
    }
    for (int i = 0; i < _nz; ++i) {
        double zeta = i * _dz;
        // 幅度部分 (Amplitude): 高斯分布
        // 对应公式：exp(-(z - za)^2 / w0^2)
        // 物理含义：能量集中在天线高度 antenna_height 附近
        // 在网格高度 zeta 上应用高斯分布
        double amp = norm_factor * std::exp(-std::pow(zeta - zeta_a, 2) / std::pow(w0, 2));        // 相位部分 (Phase): 线性相位倾斜
        // 对应公式：exp(i * k * z * sin(theta))
        // 物理含义：通过改变相位梯度来控制波束的传播方向（仰角）
        // 如果 antennaPhi_deg = 0，则相位为 0，波束水平传播
        Complex phase = std::exp(J * _k0 * zeta * tilt_rad);
        Complex val = amp * phase;
        // 填充物理空间
        reinterpret_cast<Complex*>(_in_ptr)[i] = val;
    }
}


