#pragma once

#include <string>
#include <vector>
#include <map>
#include <cmath>
#include <stdexcept> // 用于抛出异常
#include "Antenna.h"



//定义信号类型
enum class SignalType {
    CONTINUOUS_WAVE, // 连续波
    PULSED           // 脉冲
};

//脉冲信号参数
struct PulseInfo {
    double pulseWidthS = 0.0; // 脉冲宽度 (秒)
    double prfHz = 0.0;       // 脉冲重复频率 (赫兹)
    double peakPowerDbm = 0.0;// 峰值功率 (dBm)
};

//谐波类
struct HarmonicInfo {
    int order = 2;              // 谐波阶次 (例如 2, 3, 4...)
    double relativePowerDb = -20.0; // 相对于基波功率的强度 (dB), 例如 -20dBc
};

/**
类描述了一个发射信号源的多个关键特性，包括功率、频率、带宽、
时间特性（连续波/脉冲）、以及频率域的非理想特性（频谱模板和谐波）。
 **/
class TransmitterSource {
public:
    TransmitterSource(std::string id, double centerFrequencyHz, double bandwidthHz, double averagePowerDbm);
    void setAsPulsed(double pulseWidthS, double prfHz);//设置为脉冲
    void setAsCW();//设置为连续


//一个map，key为偏离中心频率的频率(Hz, 正值)，value为功率抑制值(dBc, 负值)。
    void setSpectralMask(const std::map<double, double>& mask);


     // 添加一个谐波的信息
     // order 谐波阶次 (必须 >= 2)
     // relativePowerDb 相对于基波的功率 (dBc, 通常为负值)
    void addHarmonic(int order, double relativePowerDb);

    // 关联一个天线对象
    // antenna 指向一个已存在的天线对象的指针。
    void associateAntenna(Antenna* antenna);

    // 获取在指定查询频率点的等效发射功率
    // 这是本类的核心功能，它会综合考虑带内、带外(频谱模板)和谐波的影响
    // queryFrequencyHz 您想查询的频率点 (Hz)。
    // 该频率点上的等效发射功率 (dBm)。
    double getPowerAtFrequency(double queryFrequencyHz) const;

    // --- Getters ---
    std::string getID() const { return m_id; }
    double getCenterFrequencyHz() const { return m_centerFrequencyHz; }
    double getBandwidthHz() const { return m_bandwidthHz; }
    double getAveragePowerDbm() const { return m_averagePowerDbm; }
    SignalType getSignalType() const { return m_signalType; }
    const PulseInfo& getPulseInfo() const;
    const std::map<double, double>& getSpectralMask() const { return m_spectralMaskDbc; }
    const std::vector<HarmonicInfo>& getHarmonics() const { return m_harmonics; }
    Antenna* getAssociatedAntenna() const { return m_associatedAntenna; }

private:
    // --- 私有成员变量 ---
    std::string m_id;
    double m_centerFrequencyHz;
    double m_bandwidthHz;
    double m_averagePowerDbm;

    SignalType m_signalType;
    PulseInfo m_pulseInfo;
    std::map<double, double> m_spectralMaskDbc; // key: Freq Offset (Hz), value: Suppression (dBc)
    std::vector<HarmonicInfo> m_harmonics;
    Antenna* m_associatedAntenna;

    // --- 私有辅助函数 ---
    // 根据占空比和平均功率计算峰值功率。
    void calculatePeakPower();

    // 从频谱模板中获取带外抑制值。
    // frequencyOffsetHz 偏离中心频率的绝对值 (Hz)。
    // 抑制值 (dBc, 负值)。
    double getSuppressionFromMask(double frequencyOffsetHz) const;
};
