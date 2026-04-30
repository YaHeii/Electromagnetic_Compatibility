#pragma once

#include <vector>

#include <QWidget>

#include "Interface/SimulationResult.h"
#include "Resource/ui/SimulationResultCatalog.h"
#include "Resource/ui/qcustomplot.h"
#include "Utils/SimulationChartRenderer.h"

using GridMap = std::vector<std::vector<double>>;
using Matrix = std::vector<std::vector<double>>;
using LineMap = std::vector<double>;

inline void PEmodel_Painting2D(const ScalarField2D& field, QCustomPlot* plot) {
    SimulationChartRenderer::renderScalarFieldDetail(
        field,
        SimulationChartKey::AggregatedField,
        plot,
        false);
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
