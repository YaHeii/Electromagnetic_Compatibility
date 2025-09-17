#include "../include/core/TransmitterSource.h"
#include <limits> // for std::numeric_limits


Electromagnetic_compatibility::core::TransmitterSource::TransmitterSource(std::string id, double centerFrequencyHz, double bandwidthHz, double averagePowerDbm)
    : m_id(std::move(id)),
      m_centerFrequencyHz(centerFrequencyHz),
      m_bandwidthHz(bandwidthHz),
      m_averagePowerDbm(averagePowerDbm),
      m_signalType(SignalType::CONTINUOUS_WAVE),
      m_associatedAntenna(nullptr) 
{
    if (centerFrequencyHz <= 0 || bandwidthHz <= 0) {
        throw std::invalid_argument("Frequency and bandwidth must be positive.");
    }
}

void Electromagnetic_compatibility::core::TransmitterSource::setAsPulsed(double pulseWidthS, double prfHz) {
    if (pulseWidthS <= 0 || prfHz <= 0) {
        throw std::invalid_argument("Pulse width and PRF must be positive.");
    }
    m_signalType = SignalType::PULSED;
    m_pulseInfo.pulseWidthS = pulseWidthS;
    m_pulseInfo.prfHz = prfHz;
    calculatePeakPower();
}

void Electromagnetic_compatibility::core::TransmitterSource::setAsCW() {
    m_signalType = SignalType::CONTINUOUS_WAVE;
}

void Electromagnetic_compatibility::core::TransmitterSource::setSpectralMask(const std::map<double, double>& mask) {
    m_spectralMaskDbc = mask;
}

void Electromagnetic_compatibility::core::TransmitterSource::addHarmonic(int order, double relativePowerDb) {
    if (order < 2) {
        throw std::invalid_argument("Harmonic order must be 2 or greater.");
    }
    m_harmonics.push_back({order, relativePowerDb});
}

void Electromagnetic_compatibility::core::TransmitterSource::associateAntenna(Antenna* antenna) {
    m_associatedAntenna = antenna;
}

const Electromagnetic_compatibility::core::PulseInfo& Electromagnetic_compatibility::core::TransmitterSource::getPulseInfo() const {
    if (m_signalType != SignalType::PULSED) {
        throw std::logic_error("Pulse info is only available for PULSED signal type.");
    }
    return m_pulseInfo;
}

double Electromagnetic_compatibility::core::TransmitterSource::getPowerAtFrequency(double queryFrequencyHz) const {
    // 步骤1: 检查查询频率是否落在某个谐波的频带内
    for (const auto& harmonic : m_harmonics) {
        double harmonicCenterFreq = m_centerFrequencyHz * harmonic.order;
        double halfBandwidth = m_bandwidthHz / 2.0;

        if (std::abs(queryFrequencyHz - harmonicCenterFreq) <= halfBandwidth) {
            // 落在谐波频带内，返回谐波功率
            // 简化假设：谐波功率等于基波功率加上其相对值
            return m_averagePowerDbm + harmonic.relativePowerDb;
        }
    }

    // 步骤2: 检查是否落在基波的频带内
    double halfBandwidth = m_bandwidthHz / 2.0;
    double frequencyOffset = std::abs(queryFrequencyHz - m_centerFrequencyHz);

    if (frequencyOffset <= halfBandwidth) {
        // 简化假设：带内功率处处等于平均功率
        return m_averagePowerDbm;
    }

    // 步骤3: 如果是带外，则使用频谱模板计算
    double suppressionDb = getSuppressionFromMask(frequencyOffset);
    return m_averagePowerDbm + suppressionDb;
}


void Electromagnetic_compatibility::core::TransmitterSource::calculatePeakPower() {
    if (m_signalType != SignalType::PULSED) return;

    double dutyCycle = m_pulseInfo.pulseWidthS * m_pulseInfo.prfHz;

    if (dutyCycle <= 0.0 || dutyCycle > 1.0) {
        // 占空比无效，或者可能导致对数计算错误，重置峰值功率
        m_pulseInfo.peakPowerDbm = m_averagePowerDbm; // 或抛出异常
        return;
    }
    
    // P_avg = P_peak * duty_cycle  (线性域)
    // P_avg_dBm = P_peak_dBm + 10*log10(duty_cycle)
    // P_peak_dBm = P_avg_dBm - 10*log10(duty_cycle)
    m_pulseInfo.peakPowerDbm = m_averagePowerDbm - 10.0 * std::log10(dutyCycle);
}

double Electromagnetic_compatibility::core::TransmitterSource::getSuppressionFromMask(double frequencyOffsetHz) const {
    if (m_spectralMaskDbc.empty()) {
        // 如果没有定义模板，返回一个极低的默认值，表示极强的抑制
        return -std::numeric_limits<double>::infinity();
    }
    
    // 查找第一个大于给定偏移量的点
    auto it_upper = m_spectralMaskDbc.upper_bound(frequencyOffsetHz);

    if (it_upper == m_spectralMaskDbc.begin()) {
        // 偏移量比模板中最小的点还小，返回第一个点的抑制值
        return it_upper->second;
    }

    if (it_upper == m_spectralMaskDbc.end()) {
        // 偏移量比模板中最大的点还大，返回最后一个点的抑制值
        return m_spectralMaskDbc.rbegin()->second;
    }

    // 找到了两个点，it_upper是右边的点，it_lower是左边的点
    auto it_lower = std::prev(it_upper);

    // 在对数-对数坐标系下进行线性插值 (dB vs log(freq))
    double x1 = std::log10(it_lower->first);
    double y1 = it_lower->second;
    double x2 = std::log10(it_upper->first);
    double y2 = it_upper->second;
    double x = std::log10(frequencyOffsetHz);

    // 线性插值公式: y = y1 + (x - x1) * (y2 - y1) / (x2 - x1)
    double suppression = y1 + (x - x1) * (y2 - y1) / (x2 - x1);

    return suppression;
}