#pragma once
#include <complex>
#include <vector>
#include <cmath>
#include <fftw3.h> 
#include <iostream>
#include <algorithm>
#include <Eigen/Dense>
#include <omp.h>
#include "spdlog/spdlog.h"
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
using Complex = std::complex<double>;
using std::vector;
const double GRAVITY = 9.81; // m/s^2
const Complex J(0.0, 1.0);//虚数单位

// 大气环境接口：用于计算大气折射率 n(x, z)
// 蒸发波导或标准大气的建模
class AtmosphereModel {
private:
    double _M0;      // 底部修正折射率，通常为 330
    double _z0;      // 粗糙度长度，通常为 1.5e-4
    double _c0;      // 常数，通常为 0.125
    double _duct_height; // 蒸发波导高度 H0 (m)

public:
    AtmosphereModel(double duct_height, double M0 = 330.0, double z0 = 1.5e-4, double c0 = 0.125)
        : _duct_height(duct_height), _M0(M0), _z0(z0), _c0(c0) {
    }

    // 计算高度 z 处的修正折射率 M(z)
    // 𝑀(𝑧)=𝑀(𝑧0)+𝑐0(𝑧−𝐻0ln𝑧/𝑧0)
    double getModifiedRefractivity(double z) const {
        if (z < _z0) z = _z0; // 防止对数域错误
        return _M0 + _c0 * (z - _duct_height * std::log(z / _z0));
    }

    // 获取折射率 n(z) 用于 PE 计算
    // n = 1 + M * 10^-6 (近似关系)
    double getRefractiveIndex(double z) const {
        double M = getModifiedRefractivity(z);
        return 1.0 + M * 1.0e-6;
    }
};

// Miller-Brown 粗糙度模型
// 基础模型
class MillerBrownModel {
public:
    // 计算均方根波高 h (菲利普斯谱)
    // 公式: h = 0.0051 * U^2
    static double calculateRMSHeight(double wind_speed) {
        return 0.0051 * std::pow(wind_speed, 2);
    }

    // 计算瑞利粗糙度因子 gamma
    // gamma = 2 * k * h * sin(theta)
	//  k 为波常数；θ 为掠射角（弧度）
    static double calculateRayleighParameter(double k, double h_rms, double grazing_angle_rad) {
        return 2.0 * k * h_rms * std::sin(grazing_angle_rad); 
    }

    // 计算 Miller-Brown 粗糙度衰减因子 rho
    // $\ro$ = exp(-0.5*gamma^2) * I0(0.5*gamma^2)
	// I0 为零阶修正贝塞尔函数。
    static double getAttenuationFactor(double wind_speed, double freq_hz, double grazing_angle_rad) {
        double lambda = 299792458.0 / freq_hz;
        double k = 2.0 * M_PI / lambda;

        double h_rms = calculateRMSHeight(wind_speed);
        double gamma = calculateRayleighParameter(k, h_rms, grazing_angle_rad);

        double x = 0.5 * gamma * gamma;

        try {
            double I0 = std::cyl_bessel_i(0.0, x);
            double rho = std::exp(-x) * I0;
            return rho; 
        }
        catch (...) {
            return 1.0; // 出错回退到光滑海面
        }
    }
};

// JONSWAP 双重叠加随机海面模型
// PE边界计算、海面高度生成
class JONSWAPSurfaceGenerator {
private:
    struct _WaveComponent {
        double amplitude; // Zeta_ij 振幅
        double omega;     // 角频率
        double k;         // 波数
        double angle;     // 方向角 theta_j
        double phase;     // 随机相位 Beta_Is [cite: 779]
        double kx;        // 角度x
        double ky;      // 角度y
    };

    std::vector<_WaveComponent> _components;
    double _wind_speed_U10;
    double _wind_fetch_X; // 风区长度 [cite: 798]

public:
    JONSWAPSurfaceGenerator(double wind_speed, double fetch = 20000.0)
        : _wind_speed_U10(wind_speed), _wind_fetch_X(fetch) {

        generateSpectrumComponents();
    }

    // 生成 JONSWAP 谱分量
    void generateSpectrumComponents();
    // 获取特定位置 (x, y) 和时间 t 的海面高度
    // 对应公式 (18)
    double getSurfaceHeight(double x, double y, double t);
};

// 抛物线方程求解器
// 整合：SSFT + 大气折射率 + Miller-Brown 边界
class PEModel {
private:
    double _freq; // 频率
    double _k0; // 真空波数
    double _dx; // 步进距离
    double _dz; // 步进高度
    int _nz;
    int _fft_size;// FFT 计算网格数 (2 * nz_) 用于镜像法

    // FFTW  相关
    fftw_complex* _in_ptr
        , *_out_ptr;
    fftw_plan _plan_fwd
        , _plan_bwd;


    std::vector<Complex> _diffraction_term; // 衍射因子 (k-space)
    std::vector<double> _absorber;          // 顶部吸收窗
public:
    PEModel(double centralF_Ghz, double dx, double dz, int nz)
        : _freq(centralF_Ghz), _dx(dx), _dz(dz), _nz(nz) {
        _freq *= 1000000000;
        _k0 = 2.0 * M_PI * _freq / 299792458.0; // 转换为 Hz 后计算波数
        spdlog::info("Frequency = {} Hz", _freq);
		spdlog::info("Wave number k0 = {} 1/m", _k0);
        // 使用镜像法：FFT 大小为 2 * nz
        // 索引 0 ~ nz-1 : 物理空间 (z > 0)
        // 索引 nz ~ 2*nz-1 : 镜像空间 (z < 0)
        _fft_size = 2 * _nz;

        // 分配 FFTW 内存
        _in_ptr = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * _fft_size);
        _out_ptr = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * _fft_size);

        // 创建 FFTW 计划 (Forward: z -> kz, Backward: kz -> z)
        _plan_fwd = fftw_plan_dft_1d(_fft_size, _in_ptr, _out_ptr, FFTW_FORWARD, FFTW_ESTIMATE);
        _plan_bwd = fftw_plan_dft_1d(_fft_size, _out_ptr, _in_ptr, FFTW_BACKWARD, FFTW_ESTIMATE);

        // 初始化功能模块
        precomputeDiffraction();
        setupAbsorber();
    }

    ~PEModel() {
        fftw_destroy_plan(_plan_fwd);
        fftw_destroy_plan(_plan_bwd);
        fftw_free(_in_ptr);
        fftw_free(_out_ptr);
    }


    //顶部吸收层 (Absorber) 实现
    void setupAbsorber();

    // 计算衍射因子
    // ssft中逆傅里叶变换的指数项因子
    void precomputeDiffraction();

    // 计算 Miller - Brown 粗糙度衰减因子 rho
    double calculateRoughnessRho(double wind_speed, double grazing_angle_rad);

    // 计算菲涅尔反射系数 (水平极化)
    Complex calculateFresnel(double grazing_angle_rad, Complex epsilon_sea);

    // SSFT计算
    // 包含大气折射效应和 Miller-Brown 边界处理
    void step_Miller_Brown(double current_range, double wind_speed, const std::vector<double>& n_profile);
    //一维PLST计算
    //得到当前高度下损耗随着x的变化
	void step_PLST(double current_range, const std::vector<double>& n_profile, JONSWAPSurfaceGenerator& surface_gen, double time_sec = 0.0);
	//二维PLST计算，加入方位角影响
    //得到当前高度下二维平面的损耗分布
    void step_PLST(double current_range, double azimuth_rad, const std::vector<double>& n_profile, JONSWAPSurfaceGenerator& surface_gen, double time_sec = 0.0);
    // PE要求垂直场分布u(0,z)
    // 初始化高斯波束 
    // $$u(0, z) = \exp\left( -\frac{(z - z_a)^2}{w_0^2} \right) \cdot \exp(i k z \sin \theta_{tilt})$$
    // $z_a$：天线高度（Antenna Height）。$w_0$：波束宽度参数（Beam Width Parameter），控制波束的胖瘦。
    // $\theta_{tilt}$：天线仰角（Elevation/Tilt Angle），控制波束的初始投射方向。$k$：波数。
    // 还可以使用sinc函数，模拟矩形孔径天线，具备明显旁瓣，模拟雷达多径效应
    // $$u(0, z) = \text{sinc}\left( \frac{z - z_a}{w} \right) \cdot e^{i k z \sin\theta_{tilt}}$$
    // 阵列因子
    // $$u(0, z) = \sum_{n=0}^{N-1} A_n \cdot \exp\left(-\frac{(z - z_n)^2}{w^2}\right) \cdot e^{i \phi_n}$$
    void initializeGaussian(double antenna_height, double beamWidth_deg, double antennaPhi_deg);
    


    double getPathLoss(int z_idx, double range) {
        if (z_idx >= _nz) return 0.0;
        Complex u = reinterpret_cast<Complex*>(_in_ptr)[z_idx];
        double mag = std::abs(u);
        // PE 计算的是 u，真实场 E = u * exp(ikx) / sqrt(x)
        double F = mag * std::sqrt(range); // 补上 2D->3D 的扩散因子 sqrt(r)
        double lambda = 299792458.0 / _freq;
        double L_fspl = 20.0 * log10(4.0 * M_PI * range / lambda);
        double loss = L_fspl - 20.0 * log10(F + 1e-12);
        return loss;
    }
};