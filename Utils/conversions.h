#pragma once
#include <cmath> 
#include <Eigen/Dense>



// dBm to Watts
inline double dbmToWatts(double dbm) {
    return std::pow(10.0, (dbm - 30.0) / 10.0);
}

// Watts to dBm
inline double wattsToDbm(double watts) {
    if (watts <= 0) return -HUGE_VAL; // Or throw an error
    return 10.0 * std::log10(watts) + 30.0;
}
// mW 转 dBm
inline double mwToDbm(double mw) {
    if (mw <= 1e-20) return -200.0; // 防止 log(0)
    return 10.0 * std::log10(mw);
}
// dBm 转 mW 
inline double dbmToMw(double dbm) {
    return std::pow(10.0, dbm / 10.0);
}
// dB to linear
inline double dbToLinear(double db) {
    return std::pow(10.0, db / 10.0);
}

// Linear to dB
inline double linearToDb(double linear_val) {
    if (linear_val <= 0) return -HUGE_VAL; // Or throw an error
    return 10.0 * std::log10(linear_val);
}
// mw + dbm便携函数，注意参数顺序
inline void accumulatePowerLinear(Eigen::MatrixXd& total_mw, const Eigen::MatrixXd& current_dbm) {
    // 使用 Eigen 的 .unaryExpr 进行高效的元素级转换和累加
    total_mw += current_dbm.unaryExpr([](double val) {
        return std::pow(10.0, val / 10.0);
    });
}