//#include "../include/mainwindow.h"
//#include <QApplication>
//#ifndef M_PI
//#define M_PI 3.14159265358979323846
//#endif
//int main(int argc, char *argv[])
//{
//    QApplication a(argc, argv);
//    MainWindow w;
//    w.show();
//    return a.exec();
//}
#include "../include/models/PEModel.h"
int main() {
    // 1. 仿真参数设置
    double freq = 9.4e9;       // 9.4 GHz (X-band)
    double dx = 50.0;          // 步进 50m
    double dz = 0.2;           // 垂直分辨率 0.2m (越高越好，建议 <= lambda/2)
    int nz = 2048;             // 物理高度网格 (总高度 ~400m)
    double max_range = 50000.0;// 50 km

    // 2. 初始化大气模型：蒸发波导高度 20m
    // 对应 Paper 2 Fig. 6(d) 和 Eq. (35)
    AtmosphereModel atm(20.0);

    // 3. 预计算折射率剖面 (Profile)
    // 这一步非常重要，避免在 step 循环中重复计算 log 函数，提升效率
    std::vector<double> n_profile(nz);
    for (int i = 0; i < nz; ++i) {
        double z = i * dz;
        n_profile[i] = atm.getRefractiveIndex(z);
    }

    // 4. 初始化求解器 (包含 FFTW3 和 镜像法)
    PEModel solver(freq, dx, dz, nz);

    // 初始化高斯波束：天线高度 25m
    solver.initializeGaussian(25.0, 2.0, 0.0);

    // 5. 开始步进仿真
    std::cout << "Range(km) \t Loss(dB) \t (Atmosphere: Evaporation Duct 20m)" << std::endl;

    for (double r = 0; r < max_range; r += dx) {
        // [关键]：将预计算好的大气剖面传递给求解器
        // 7.0 是风速 (m/s)，用于计算 Miller-Brown 粗糙度
        solver.step_Miller_Brown(r, 7.0, n_profile);

        // 输出数据
        if (std::abs(fmod(r, 1000.0)) < 0.1) {
            // 获取接收天线高度 15m 处的损耗
            int rx_idx = static_cast<int>(15.0 / dz);
            double loss = solver.getPathLoss(rx_idx, r);
            std::cout << r / 1000.0 << " \t\t " << loss << std::endl;
        }
    }

    return 0;
}
