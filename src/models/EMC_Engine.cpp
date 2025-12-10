#include "../../include/models/EMC_Engine.h"
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


void Propagation_Engine::initializePEmodel(PE_data _PEdata,double reciever_antenna_height) {
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
    PEModel solver(_PEdata.freq, _PEdata.dx, _PEdata.dz, _PEdata.nz);

    // 初始化高斯波束：天线高度 25m
    solver.initializeGaussian(_PEdata.sender_antenna_height,_PEdata.beam_width_deg
        , _PEdata.elevation_deg);

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
        }
    }
}


//std::vector<InterferenceResult> EMC_Engine::EMC_computing(const Fleet& fleet) {//返回受扰计算结果数组
//
//}
