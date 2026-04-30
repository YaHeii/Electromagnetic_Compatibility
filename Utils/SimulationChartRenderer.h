#pragma once

#include <QPixmap>
#include <QSize>

#include "Resource/ui/SimulationResultCatalog.h"
#include "Resource/ui/qcustomplot.h"

class SimulationChartRenderer {
public:
    static void renderScalarFieldDetail(
        const ScalarField2D& field,
        SimulationChartKey key,
        QCustomPlot* plot,
        bool preview = false);

    static void renderSeriesDetail(
        const Series1D& primarySeries,
        const Series1D* secondarySeries,
        SimulationChartKey key,
        QCustomPlot* plot,
        bool preview = false);

    static void renderMatrixDetail(
        const LabeledMatrix2D& matrix,
        SimulationChartKey key,
        QCustomPlot* plot,
        bool preview = false);

    static QPixmap renderPreviewPixmap(
        const SimulationChartPayload& payload,
        const QSize& size = QSize(320, 180));
};
