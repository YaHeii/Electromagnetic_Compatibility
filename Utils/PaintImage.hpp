#pragma once
#include "Resource/ui/qcustomplot.h"
#include <QWidget>
using GridMap = std::vector<std::vector<double>>;
using Matrix = std::vector<std::vector<double>>;
using LineMap = std::vector<double>;
inline void PEmodel_Painting2D(const GridMap& Loss_2D, QCustomPlot* PEmodel_2Dplot) {
    // 1. 检查数据有效性
    if (Loss_2D.empty() || Loss_2D[0].empty()) return;
    int nx = Loss_2D.size();       // X轴方向的数据量
    int ny = Loss_2D[0].size();    // Y轴方向的数据量

    // 2. 清除之前的图层（如果多次调用此函数，需要防止图层叠加）
    PEmodel_2Dplot->clearPlottables();
    // 3. 创建颜色图对象 (ColorMap)
    QCPColorMap* colorMap = new QCPColorMap(PEmodel_2Dplot->xAxis, PEmodel_2Dplot->yAxis);

    // 4. 设置数据尺寸和坐标范围
    colorMap->data()->setSize(nx, ny);
    colorMap->data()->setRange(QCPRange(0, nx), QCPRange(0, ny));

    // 5. 填充数据
    for (int x = 0; x < nx; ++x) {
        for (int y = 0; y < ny; ++y) {
            // 注意：要确保数据索引不越界，且逻辑对应
            // setCell(x, y, value)
            colorMap->data()->setCell(x, y, Loss_2D[x][y]);
        }
    }

    // 6. 设置颜色条 (Color Scale) - 显示在右侧的色带
    QCPColorScale* colorScale = nullptr;
    if (PEmodel_2Dplot->plotLayout()->elementCount() > 1) {
        colorScale = qobject_cast<QCPColorScale*>(PEmodel_2Dplot->plotLayout()->element(0, 1));
    }

    if (!colorScale) {
        colorScale = new QCPColorScale(PEmodel_2Dplot);
        PEmodel_2Dplot->plotLayout()->addElement(0, 1, colorScale); // 添加到右侧
    }

    colorMap->setColorScale(colorScale); // 关联 map 和 scale

    // 7. 设置颜色梯度（配色方案）
    // gpJet 是经典的彩虹色，gpThermal 是热成像色，gpGrayscale 是灰度
    colorMap->setGradient(QCPColorGradient::gpJet);

    // 8. 设置颜色数据范围，增强梯度对比
    colorMap->setDataRange(QCPRange(60.0, 140.0));
    colorMap->rescaleAxes();//放缩坐标轴
    // 9. 重新绘制
    PEmodel_2Dplot->replot();
}

//TODO: 结合双重叠加法结果绘制不同海况下海面情况。
void PaintMarineSurface_3D();