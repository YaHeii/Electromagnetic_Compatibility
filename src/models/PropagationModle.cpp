#include "../include/models/PropagationModle.h"
#include <cmath> // For sqrt, pow, log10
#include <complex>
#include <limits> // For std::numeric_limits
#include "../include/utils/point_2D.h"
#include "models/shortlist.h"
#define C_LIGHT 300000000
namespace Electromagnetic_compatibility {
namespace models {

double FreeSpaceModel::getPathLossDb(const utils::Point2D& pos_tx,
                                     const utils::Point2D& pos_rx,
                                     double frequency_mhz) const {
    // 计算距离
    double dx = pos_tx.x - pos_rx.x;
    double dy = pos_tx.y - pos_rx.y;
    double distance = std::sqrt(dx * dx + dy * dy);

    if (distance == 0) {
        return 0.0; // 或者返回一个很大的损耗值
    }

    // Friis公式
    double path_loss = 20.0 * std::log10(distance) + 20.0 * std::log10(frequency_mhz) - 27.55;
    return path_loss;
}
// --- TwoRayModel 实现 ---
TwoRayModel::TwoRayModel(double h_t_m, double h_r_m) : m_h_t_m(h_t_m), m_h_r_m(h_r_m) {}

double TwoRayModel::getPathLossDb(const utils::Point2D& pos_tx,
                                  const utils::Point2D& pos_rx,
                                  double frequency_mhz) const {
    if (frequency_mhz <= 0) {
        throw std::invalid_argument("Frequency must be positive.");
    }

    // 步骤1: 计算几何距离
    double d = TwoRayModel::calculate_distance(pos_tx, pos_rx); // 水平距离
    if (d <= 0) {
        return 0.0; // 同一位置，损耗为0
    }
    // 直射路径长度
    double d_los = std::sqrt(std::pow(d, 2) + std::pow(m_h_t_m - m_h_r_m, 2));
    // 反射路径长度 (使用镜像法)
    double d_ref = std::sqrt(std::pow(d, 2) + std::pow(m_h_t_m + m_h_r_m, 2));

    // 步骤2: 计算相位差
    double frequency_hz = frequency_mhz * 1e6;
    double lambda = C_LIGHT / frequency_hz; // 波长
    double path_difference = d_ref - d_los; // 路程差
    double phase_diff_rad = (2.0 * M_PI * path_difference) / lambda; // 由路程差引起的相位差

    // 步骤3: 确定反射系数 Gamma
    // 这是一个复杂的参数，取决于极化、掠射角和海面介电常数。
    // 为简化模型，我们做一个工程上常用的假设：对于水平极化和小掠射角，
    // 反射系数近似为 -1，即幅值不变，相位反转180度(pi)。
    std::complex<double> gamma = {-1.0, 0.0};

    // 步骤4: 矢量叠加计算总场强增益/损失
    // 总场强是直射波和反射波的复数叠加。
    // E_total ∝ ( E_los + E_ref )
    // E_los ∝ (1/d_los) * exp(-j*k*d_los)
    // E_ref ∝ Gamma * (1/d_ref) * exp(-j*k*d_ref)
    // 路径损耗是发射功率与接收功率的比值，与总场强的平方成反比。
    // L = (P_t / P_r) ∝ 1 / |E_total|^2
    
    double k = 2.0 * M_PI / lambda; // 波数
    
    // 创建代表直射和反射波的复数（相位器）
    // 幅度按 1/d 衰减
    std::complex<double> field_los = (1.0 / d_los) * std::exp(std::complex<double>(0, -k * d_los));
    std::complex<double> field_ref = gamma * (1.0 / d_ref) * std::exp(std::complex<double>(0, -k * d_ref));
    
    // 矢量叠加
    std::complex<double> field_total = field_los + field_ref;
    
    // 计算接收功率。P_r = P_t * G_t * G_r * (lambda / 4*pi)^2 * |E_total_normalized|^2
    // 路径损耗 L = P_t / P_r (假设 G_t=G_r=1)
    // L = (4*pi / lambda)^2 / |(1/d_los) + Gamma*(1/d_ref)*exp(-j*k*path_diff)|^2 ... 比较复杂

    // 采用更直观的方式计算：与自由空间损耗进行比较
    // 自由空间损耗（以d_los为准）对应的场强幅值（归一化后）
    double field_los_magnitude = 1.0 / d_los;
    
    // 总场强的幅值
    double field_total_magnitude = std::abs(field_total);

    // 场强增益/损失因子（相对于自由空间）
    double gain_factor = field_total_magnitude / field_los_magnitude;
    
    // 转换为dB
    double gain_factor_db = 20.0 * std::log10(gain_factor);

    // 计算以直射路径d_los为基准的自由空间损耗
    double fsl_db = 20.0 * std::log10(d_los) + 20.0 * std::log10(frequency_mhz) - 27.55;
    
    // 总路径损耗 = 自由空间损耗 - 双径干涉带来的增益
    double total_path_loss_db = fsl_db - gain_factor_db;

    return total_path_loss_db > 0 ? total_path_loss_db : 0;
}


double TwoRayModel::calculate_distance(const utils::Point2D &pos_tx, const utils::Point2D &pos_rx){
    return std::sqrt(std::pow(pos_tx.x - pos_rx.x, 2) + std::pow(pos_tx.y - pos_rx.y, 2));
}
} // namespace models
} // namespace Electromagnetic_compatibility