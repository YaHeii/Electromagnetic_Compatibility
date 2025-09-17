#pragma once
#include "../utils/point_2D.h"
#include "../core/antenna.h" // For Antenna (though not directly used in this simplified Friis)
namespace Electromagnetic_compatibility {
namespace models {

class PropagationModel {
public:
    virtual ~PropagationModel() = default;
    /**
     * @brief Calculates path loss between two points.
     * @param pos_tx Transmitter position.
     * @param pos_rx Receiver position.
     * @param frequency_mhz Frequency in MHz.
     * @return Path loss in dB (a positive value).
     */
    virtual double getPathLossDb(const utils::Point2D& pos_tx,
                                 const utils::Point2D& pos_rx,
                                 double frequency_mhz) const = 0;
};

class FreeSpaceModel : public PropagationModel{
public:
    double getPathLossDb(const utils::Point2D& pos_tx,
                         const utils::Point2D& pos_rx,
                         double frequency_mhz) const override;//const over
};
//海面双径模型
class TwoRayModel : public PropagationModel {
public:
    /**
     * @brief 构造函数，接收并存储天线高度。
     * @param h_t_m 发射天线高度 (米)
     * @param h_r_m 接收天线高度 (米)
     */
    TwoRayModel(double h_t_m, double h_r_m);

    double getPathLossDb(const utils::Point2D& pos_tx,
                         const utils::Point2D& pos_rx,
                         double frequency_mhz) const override;

    static double calculate_distance(const utils::Point2D& pos_tx, const utils::Point2D& pos_rx);

private:
    double m_h_t_m; // 将高度作为成员变量存储
    double m_h_r_m;
};
} // namespace models
} // namespace Electromagnetic_compatibility