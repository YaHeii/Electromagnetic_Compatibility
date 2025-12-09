//#include "../include/mainwindow.h"
//#include <QApplication>
//
//int main(int argc, char *argv[])
//{
//    QApplication a(argc, argv);
//    MainWindow w;
//    w.show();
//    return a.exec();
//}

#include <iostream>
#include <vector>
#include <cmath>
#include <fftw3.h> // 核心头文件
#define M_PI 3.1415926535897932384626433832795

// 简单的复数输出辅助函数
void print_complex(const char* tag, fftw_complex* arr, int n) {
    std::cout << tag << ":" << std::endl;
    for (int i = 0; i < n; ++i) {
        // arr[i][0] 是实部, arr[i][1] 是虚部
        std::cout << "  [" << i << "] " << arr[i][0] << " + " << arr[i][1] << "i" << std::endl;
    }
}

int main() {
    int N = 8; // 采样点数

    // 1. 分配输入和输出数组
    // fftw_malloc 能够保证内存对齐，对 SIMD 指令集优化至关重要
    fftw_complex* in = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * N);
    fftw_complex* out = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * N);

    // 2. 创建变换计划 (Plan)
    // FFTW_FORWARD 表示正变换 (时域->频域)
    // FFTW_ESTIMATE 表示快速估算最佳算法 (不实际测量)
    fftw_plan plan = fftw_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_ESTIMATE);

    // 3. 填充模拟数据 (例如一个简单的直流分量 + 交流分量)
    for (int i = 0; i < N; ++i) {
        in[i][0] = 1.0 + cos(2 * M_PI * i / N); // 实部
        in[i][1] = 0.0;                        // 虚部
    }

    // 4. 执行变换
    fftw_execute(plan);

    // 5. 输出结果
    print_complex("Input (Time Domain)", in, N);
    print_complex("Output (Freq Domain)", out, N);

    // 6. 清理资源
    fftw_destroy_plan(plan);
    fftw_free(in);
    fftw_free(out);

    return 0;
}