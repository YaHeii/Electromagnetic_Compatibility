#pragma once
#include "qcustomplot.h"
#include <QWidget>
using GridMap = std::vector<std::vector<double>>;
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
    // 假设坐标就是 0 到 nx, 0 到 ny。如果你的物理坐标不同，在这里修改 setRange
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
    // 检查是否已经有名为 "colorScale" 的布局元素，避免重复添加
    QCPColorScale* colorScale = nullptr;
    if (PEmodel_2Dplot->plotLayout()->elementCount() > 1) {
        // 尝试获取现有的 colorScale（稍微高级一点的处理，防止内存泄漏）
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
    colorMap->setDataRange(QCPRange(-30, -10));
    // 保持纵横比（可选，如果是地图或物理场通常需要）
    // customPlot->rescaleAxes(); 
    //colorMap->rescaleDataRange(true);
    PEmodel_2Dplot->replot();
}
