#pragma once

#include <vector>

#include <QWidget>

#include "Interface/SimulationResult.h"
#include "Resource/ui/qcustomplot.h"

using GridMap = std::vector<std::vector<double>>;
using Matrix = std::vector<std::vector<double>>;
using LineMap = std::vector<double>;

inline void PEmodel_Painting2D(const ScalarField2D& field, QCustomPlot* plot) {
    if (!plot) {
        return;
    }
    if (field.rows <= 0 || field.cols <= 0 || field.values.empty()) {
        return;
    }

    plot->clearPlottables();
    QCPColorMap* colorMap = new QCPColorMap(plot->xAxis, plot->yAxis);
    colorMap->data()->setSize(field.cols, field.rows);
    colorMap->data()->setRange(
        QCPRange(field.originX, field.originX + field.cols * field.stepX),
        QCPRange(field.originY, field.originY + field.rows * field.stepY));

    for (int row = 0; row < field.rows; ++row) {
        for (int col = 0; col < field.cols; ++col) {
            const std::size_t index = static_cast<std::size_t>(row * field.cols + col);
            colorMap->data()->setCell(col, row, field.values[index]);
        }
    }

    QCPColorScale* colorScale = nullptr;
    if (plot->plotLayout()->elementCount() > 1) {
        colorScale = qobject_cast<QCPColorScale*>(plot->plotLayout()->element(0, 1));
    }

    if (!colorScale) {
        colorScale = new QCPColorScale(plot);
        plot->plotLayout()->addElement(0, 1, colorScale);
    }

    colorMap->setColorScale(colorScale);
    colorMap->setGradient(QCPColorGradient::gpJet);
    colorMap->setDataRange(QCPRange(-120, 0));
    colorMap->rescaleAxes();
    plot->replot();
}

inline void PEmodel_Painting2D(const GridMap& loss2D, QCustomPlot* plot) {
    if (loss2D.empty() || loss2D.front().empty()) {
        return;
    }

    ScalarField2D field;
    field.fieldId = QStringLiteral("legacy-gridmap");
    field.displayName = QStringLiteral("Legacy GridMap");
    field.quantity = ScalarFieldQuantity::AggregatedPowerDbm;
    field.valueUnit = QStringLiteral("dBm");
    field.axisXUnit = QStringLiteral("index");
    field.axisYUnit = QStringLiteral("index");
    field.rows = static_cast<int>(loss2D.size());
    field.cols = static_cast<int>(loss2D.front().size());
    field.originX = 0.0;
    field.originY = 0.0;
    field.stepX = 1.0;
    field.stepY = 1.0;
    field.values.reserve(static_cast<std::size_t>(field.rows * field.cols));

    for (const auto& row : loss2D) {
        for (double value : row) {
            field.values.push_back(value);
        }
    }

    PEmodel_Painting2D(field, plot);
}
