#include "Utils/SimulationChartRenderer.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <limits>

#include <QBrush>
#include <QColor>
#include <QFont>
#include <QPainter>
#include <QPen>
#include <QSharedPointer>
#include <QVector>

namespace {

enum class ColorMapPalette {
    Jet,
    Inferno,
    Magma,
    DesenseRisk
};

struct DataRange {
    double minValue{0.0};
    double maxValue{1.0};
};

QCPColorScale* ensureColorScale(QCustomPlot* plot) {
    if (!plot) {
        return nullptr;
    }

    if (plot->plotLayout()->elementCount() > 1) {
        if (auto* colorScale = qobject_cast<QCPColorScale*>(plot->plotLayout()->element(0, 1))) {
            return colorScale;
        }
    }

    auto* colorScale = new QCPColorScale(plot);
    plot->plotLayout()->addElement(0, 1, colorScale);
    return colorScale;
}

bool isInvalidValue(double value, const std::optional<double>& noDataValue) {
    if (std::isnan(value)) {
        return true;
    }
    if (!noDataValue.has_value()) {
        return false;
    }
    const double expected = noDataValue.value();
    const double tolerance = std::max(1e-9, std::abs(expected) * 1e-9);
    return std::abs(value - expected) <= tolerance;
}

DataRange computeFieldRange(const ScalarField2D& field) {
    double minValue = std::numeric_limits<double>::max();
    double maxValue = std::numeric_limits<double>::lowest();

    for (double value : field.values) {
        if (isInvalidValue(value, field.noDataValue)) {
            continue;
        }
        minValue = std::min(minValue, value);
        maxValue = std::max(maxValue, value);
    }

    if (minValue == std::numeric_limits<double>::max() ||
        maxValue == std::numeric_limits<double>::lowest()) {
        return {};
    }

    if (std::abs(maxValue - minValue) < 1e-9) {
        return {minValue - 1.0, maxValue + 1.0};
    }

    return {minValue, maxValue};
}

DataRange computeMatrixRange(const LabeledMatrix2D& matrix) {
    double minValue = std::numeric_limits<double>::max();
    double maxValue = std::numeric_limits<double>::lowest();

    for (double value : matrix.values) {
        if (std::isnan(value)) {
            continue;
        }
        minValue = std::min(minValue, value);
        maxValue = std::max(maxValue, value);
    }

    if (minValue == std::numeric_limits<double>::max() ||
        maxValue == std::numeric_limits<double>::lowest()) {
        return {};
    }

    if (std::abs(maxValue - minValue) < 1e-9) {
        return {minValue - 1.0, maxValue + 1.0};
    }

    return {minValue, maxValue};
}

QCPColorGradient makeGradient(ColorMapPalette palette) {
    QCPColorGradient gradient;
    gradient.setLevelCount(300);

    switch (palette) {
    case ColorMapPalette::Jet:
        gradient.loadPreset(QCPColorGradient::gpJet);
        break;

    case ColorMapPalette::Inferno:
        gradient.setColorStopAt(0.00, QColor("#000004"));
        gradient.setColorStopAt(0.15, QColor("#1f0c48"));
        gradient.setColorStopAt(0.35, QColor("#550f6d"));
        gradient.setColorStopAt(0.55, QColor("#b5367a"));
        gradient.setColorStopAt(0.75, QColor("#f98e09"));
        gradient.setColorStopAt(1.00, QColor("#fcffa4"));
        break;

    case ColorMapPalette::Magma:
        gradient.setColorStopAt(0.00, QColor("#000004"));
        gradient.setColorStopAt(0.20, QColor("#3b0f70"));
        gradient.setColorStopAt(0.40, QColor("#8c2981"));
        gradient.setColorStopAt(0.65, QColor("#de4968"));
        gradient.setColorStopAt(0.85, QColor("#fe9f6d"));
        gradient.setColorStopAt(1.00, QColor("#fcfdbf"));
        break;

    case ColorMapPalette::DesenseRisk:
        gradient.setLevelCount(7);
        gradient.setColorStopAt(0.00, QColor("#e9f5e9"));
        gradient.setColorStopAt(0.12, QColor("#fff3e0"));
        gradient.setColorStopAt(0.28, QColor("#ffcc80"));
        gradient.setColorStopAt(0.45, QColor("#ffab91"));
        gradient.setColorStopAt(0.65, QColor("#f44336"));
        gradient.setColorStopAt(0.82, QColor("#b71c1c"));
        gradient.setColorStopAt(1.00, QColor("#4a148c"));
        break;
    }

    return gradient;
}

ColorMapPalette paletteForScalarField(SimulationChartKey key) {
    switch (key) {
    case SimulationChartKey::TElevField:
        return ColorMapPalette::Inferno;
    case SimulationChartKey::DDesenseField:
        return ColorMapPalette::DesenseRisk;
    case SimulationChartKey::AggregatedField:
    case SimulationChartKey::ReferenceEmitterPathLoss:
    case SimulationChartKey::ScfMatrix:
    case SimulationChartKey::S3iCurve:
        return ColorMapPalette::Jet;
    }
    return ColorMapPalette::Jet;
}

void resetPlot(QCustomPlot* plot, bool preview) {
    if (!plot) {
        return;
    }

    plot->clearPlottables();
    plot->clearItems();
    plot->legend->clearItems();
    plot->legend->setVisible(false);
    plot->setInteractions(QCP::iNone);
    plot->setBackground(QBrush(preview ? QColor("#f8f8f8") : QColor("#ffffff")));
    plot->xAxis->setRangeReversed(false);
    plot->yAxis->setRangeReversed(false);
    plot->xAxis->grid()->setVisible(!preview);
    plot->yAxis->grid()->setVisible(!preview);
    plot->xAxis->setTickLabels(!preview);
    plot->yAxis->setTickLabels(!preview);
    plot->xAxis->setTicks(!preview);
    plot->yAxis->setTicks(!preview);
    plot->xAxis->setSubTicks(false);
    plot->yAxis->setSubTicks(false);
}

void configureScalarAxes(QCustomPlot* plot, const ScalarField2D& field, bool preview) {
    plot->xAxis->setLabel(preview ? QString() : field.axisXUnit);
    plot->yAxis->setLabel(preview ? QString() : field.axisYUnit);
    plot->xAxis->setRange(field.originX, field.originX + field.stepX * field.cols);
    plot->yAxis->setRange(field.originY, field.originY + field.stepY * field.rows);
    plot->xAxis->setVisible(true);
    plot->yAxis->setVisible(true);
}

void configureSeriesAxes(QCustomPlot* plot, const Series1D& series, bool preview) {
    plot->xAxis->setLabel(preview ? QString() : series.xUnit);
    plot->yAxis->setLabel(preview ? QString() : series.yUnit);
    plot->xAxis->setVisible(true);
    plot->yAxis->setVisible(true);
}

void populateColorMap(
    QCPColorMap* colorMap,
    const ScalarField2D& field) {
    colorMap->data()->setSize(field.cols, field.rows);
    colorMap->data()->setRange(
        QCPRange(field.originX, field.originX + field.cols * field.stepX),
        QCPRange(field.originY, field.originY + field.rows * field.stepY));

    for (int row = 0; row < field.rows; ++row) {
        for (int col = 0; col < field.cols; ++col) {
            const std::size_t index = static_cast<std::size_t>(row * field.cols + col);
            const double value = field.values[index];
            if (isInvalidValue(value, field.noDataValue)) {
                colorMap->data()->setCell(col, row, 0.0);
                colorMap->data()->setAlpha(col, row, 0);
                continue;
            }
            colorMap->data()->setCell(col, row, value);
            colorMap->data()->setAlpha(col, row, 255);
        }
    }
}

QPixmap renderUnavailablePreview(const SimulationChartPayload& payload, const QSize& size) {
    QPixmap pixmap(size);
    pixmap.fill(QColor("#f4f4f4"));

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QColor("#9a9a9a"));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(pixmap.rect().adjusted(3, 3, -3, -3), 10, 10);

    QFont titleFont = painter.font();
    titleFont.setBold(true);
    titleFont.setPixelSize(16);
    painter.setFont(titleFont);
    painter.setPen(QColor("#505050"));
    painter.drawText(
        QRect(16, 24, size.width() - 32, 26),
        Qt::AlignLeft | Qt::AlignVCenter,
        payload.title);

    QFont subtitleFont = painter.font();
    subtitleFont.setBold(false);
    subtitleFont.setPixelSize(12);
    painter.setFont(subtitleFont);
    painter.setPen(QColor("#7a7a7a"));
    painter.drawText(
        QRect(16, 56, size.width() - 32, size.height() - 72),
        Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
        payload.subtitle);

    return pixmap;
}

void setSeriesData(QCPGraph* graph, const Series1D& series) {
    QVector<double> xValues;
    QVector<double> yValues;
    xValues.reserve(static_cast<int>(series.xValues.size()));
    yValues.reserve(static_cast<int>(series.yValues.size()));

    for (double value : series.xValues) {
        xValues.push_back(value);
    }
    for (double value : series.yValues) {
        yValues.push_back(value);
    }

    graph->setData(xValues, yValues);
}

void configurePreviewMargins(QCustomPlot* plot) {
    plot->axisRect()->setAutoMargins(QCP::msNone);
    plot->axisRect()->setMargins(QMargins(8, 8, 8, 8));
}

void configureDetailMargins(QCustomPlot* plot) {
    plot->axisRect()->setAutoMargins(QCP::msAll);
}

void configureMatrixAxes(
    QCustomPlot* plot,
    const LabeledMatrix2D& matrix,
    bool preview) {
    plot->xAxis->setVisible(true);
    plot->yAxis->setVisible(true);
    plot->xAxis->setRange(-0.5, matrix.cols - 0.5);
    plot->yAxis->setRange(-0.5, matrix.rows - 0.5);
    plot->yAxis->setRangeReversed(true);

    if (preview) {
        plot->xAxis->setTicks(false);
        plot->yAxis->setTicks(false);
        plot->xAxis->setTickLabels(false);
        plot->yAxis->setTickLabels(false);
        return;
    }

    auto xTicker = QSharedPointer<QCPAxisTickerText>(new QCPAxisTickerText());
    auto yTicker = QSharedPointer<QCPAxisTickerText>(new QCPAxisTickerText());

    for (int col = 0; col < matrix.cols; ++col) {
        xTicker->addTick(col, matrix.colLabels[static_cast<std::size_t>(col)]);
    }
    for (int row = 0; row < matrix.rows; ++row) {
        yTicker->addTick(row, matrix.rowLabels[static_cast<std::size_t>(row)]);
    }

    plot->xAxis->setTicker(xTicker);
    plot->yAxis->setTicker(yTicker);
    plot->xAxis->setTickLabelRotation(35.0);
    plot->xAxis->setLabel(QStringLiteral("发射机"));
    plot->yAxis->setLabel(QStringLiteral("接收机"));
}

void addMatrixLabels(
    QCustomPlot* plot,
    const LabeledMatrix2D& matrix,
    const DataRange& range) {
    const double threshold = range.minValue + (range.maxValue - range.minValue) * 0.65;

    for (int row = 0; row < matrix.rows; ++row) {
        for (int col = 0; col < matrix.cols; ++col) {
            const std::size_t index = static_cast<std::size_t>(row * matrix.cols + col);
            const double value = matrix.values[index];
            if (std::isnan(value)) {
                continue;
            }

            auto* textItem = new QCPItemText(plot);
            textItem->position->setType(QCPItemPosition::ptPlotCoords);
            textItem->position->setCoords(col, row);
            textItem->setPadding(QMargins(2, 2, 2, 2));
            textItem->setText(QString::number(value, 'f', 1));
            textItem->setPen(Qt::NoPen);
            textItem->setBrush(Qt::NoBrush);

            QFont font = textItem->font();
            font.setBold(true);
            font.setPixelSize(9);
            textItem->setFont(font);
            textItem->setColor(value >= threshold ? QColor("#111111") : QColor("#ffffff"));
        }
    }
}

QCPRange seriesRange(const Series1D& primarySeries, const Series1D* secondarySeries, bool useX) {
    double minValue = std::numeric_limits<double>::max();
    double maxValue = std::numeric_limits<double>::lowest();

    auto collect = [&](const Series1D& series) {
        const std::vector<double>& values = useX ? series.xValues : series.yValues;
        for (double value : values) {
            if (std::isnan(value)) {
                continue;
            }
            minValue = std::min(minValue, value);
            maxValue = std::max(maxValue, value);
        }
    };

    collect(primarySeries);
    if (secondarySeries) {
        collect(*secondarySeries);
    }

    if (minValue == std::numeric_limits<double>::max() ||
        maxValue == std::numeric_limits<double>::lowest()) {
        return QCPRange(0.0, 1.0);
    }
    if (std::abs(maxValue - minValue) < 1e-9) {
        return QCPRange(minValue - 1.0, maxValue + 1.0);
    }

    const double padding = (maxValue - minValue) * 0.08;
    return QCPRange(minValue - padding, maxValue + padding);
}

}  // namespace

void SimulationChartRenderer::renderScalarFieldDetail(
    const ScalarField2D& field,
    SimulationChartKey key,
    QCustomPlot* plot,
    bool preview) {
    if (!plot || field.rows <= 0 || field.cols <= 0 || field.values.empty()) {
        return;
    }

    resetPlot(plot, preview);
    preview ? configurePreviewMargins(plot) : configureDetailMargins(plot);
    configureScalarAxes(plot, field, preview);

    auto* colorMap = new QCPColorMap(plot->xAxis, plot->yAxis);
    populateColorMap(colorMap, field);
    colorMap->setInterpolate(true);
    colorMap->setTightBoundary(true);
    colorMap->setGradient(makeGradient(paletteForScalarField(key)));

    QCPRange dataRange;
    if (key == SimulationChartKey::DDesenseField) {
        dataRange = QCPRange(0.0, 120.0);
    } else {
        const DataRange range = computeFieldRange(field);
        dataRange = QCPRange(range.minValue, range.maxValue);
    }
    colorMap->setDataRange(dataRange);

    if (preview) {
        plot->axisRect()->setBackground(QBrush(QColor("#fafafa")));
        plot->replot();
        return;
    }

    auto* colorScale = ensureColorScale(plot);
    if (colorScale) {
        colorMap->setColorScale(colorScale);
        colorScale->setVisible(true);
        colorScale->setLabel(field.valueUnit);
    }

    plot->rescaleAxes();
    plot->replot();
}

void SimulationChartRenderer::renderSeriesDetail(
    const Series1D& primarySeries,
    const Series1D* secondarySeries,
    SimulationChartKey key,
    QCustomPlot* plot,
    bool preview) {
    Q_UNUSED(key);

    if (!plot || primarySeries.xValues.empty() || primarySeries.yValues.empty()) {
        return;
    }

    resetPlot(plot, preview);
    preview ? configurePreviewMargins(plot) : configureDetailMargins(plot);
    configureSeriesAxes(plot, primarySeries, preview);

    auto* primaryGraph = plot->addGraph();
    primaryGraph->setName(primarySeries.displayName);
    primaryGraph->setLineStyle(QCPGraph::lsLine);
    primaryGraph->setPen(QPen(QColor("#1f77b4"), preview ? 1.4 : 2.0));
    primaryGraph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssNone, 0.0));
    setSeriesData(primaryGraph, primarySeries);

    if (secondarySeries) {
        auto* secondaryGraph = plot->addGraph();
        secondaryGraph->setName(secondarySeries->displayName);
        QPen secondaryPen(QColor("#d62728"), preview ? 1.4 : 2.0);
        secondaryPen.setStyle(Qt::DashLine);
        secondaryGraph->setPen(secondaryPen);
        secondaryGraph->setLineStyle(QCPGraph::lsLine);
        secondaryGraph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssNone, 0.0));
        setSeriesData(secondaryGraph, *secondarySeries);

        primaryGraph->setBrush(QBrush(QColor(128, 128, 128, preview ? 32 : 48)));
        primaryGraph->setChannelFillGraph(secondaryGraph);

        if (!preview) {
            plot->legend->setVisible(true);
        }
    }

    plot->xAxis->setRange(seriesRange(primarySeries, secondarySeries, true));
    plot->yAxis->setRange(seriesRange(primarySeries, secondarySeries, false));
    plot->xAxis->grid()->setVisible(!preview);
    plot->yAxis->grid()->setVisible(!preview);
    plot->replot();
}

void SimulationChartRenderer::renderMatrixDetail(
    const LabeledMatrix2D& matrix,
    SimulationChartKey key,
    QCustomPlot* plot,
    bool preview) {
    Q_UNUSED(key);

    if (!plot || matrix.rows <= 0 || matrix.cols <= 0 || matrix.values.empty()) {
        return;
    }

    resetPlot(plot, preview);
    preview ? configurePreviewMargins(plot) : configureDetailMargins(plot);
    configureMatrixAxes(plot, matrix, preview);

    auto* colorMap = new QCPColorMap(plot->xAxis, plot->yAxis);
    colorMap->data()->setSize(matrix.cols, matrix.rows);
    colorMap->data()->setRange(QCPRange(-0.5, matrix.cols - 0.5), QCPRange(-0.5, matrix.rows - 0.5));

    for (int row = 0; row < matrix.rows; ++row) {
        for (int col = 0; col < matrix.cols; ++col) {
            const std::size_t index = static_cast<std::size_t>(row * matrix.cols + col);
            colorMap->data()->setCell(col, row, matrix.values[index]);
            colorMap->data()->setAlpha(col, row, std::isnan(matrix.values[index]) ? 0 : 255);
        }
    }

    const DataRange range = computeMatrixRange(matrix);
    colorMap->setDataRange(QCPRange(range.minValue, range.maxValue));
    colorMap->setGradient(makeGradient(ColorMapPalette::Magma));
    colorMap->setInterpolate(false);
    colorMap->setTightBoundary(true);

    if (!preview) {
        auto* colorScale = ensureColorScale(plot);
        if (colorScale) {
            colorMap->setColorScale(colorScale);
            colorScale->setVisible(true);
            colorScale->setLabel(matrix.valueUnit);
        }
        addMatrixLabels(plot, matrix, range);
    }

    plot->replot();
}

QPixmap SimulationChartRenderer::renderPreviewPixmap(
    const SimulationChartPayload& payload,
    const QSize& size) {
    if (!payload.available) {
        return renderUnavailablePreview(payload, size);
    }

    QCustomPlot plot;
    plot.resize(size);
    plot.setMinimumSize(size);
    plot.setMaximumSize(size);

    switch (payload.payloadType) {
    case SimulationChartPayloadType::ScalarField2D:
        if (payload.scalarField) {
            renderScalarFieldDetail(*payload.scalarField, payload.key, &plot, true);
        }
        break;

    case SimulationChartPayloadType::Series1D:
        if (payload.primarySeries) {
            renderSeriesDetail(*payload.primarySeries, payload.secondarySeries, payload.key, &plot, true);
        }
        break;

    case SimulationChartPayloadType::LabeledMatrix2D:
        if (payload.matrix) {
            renderMatrixDetail(*payload.matrix, payload.key, &plot, true);
        }
        break;
    }

    return plot.toPixmap(size.width(), size.height(), 1.0);
}
