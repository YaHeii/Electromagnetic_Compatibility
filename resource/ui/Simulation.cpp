#include "Simulation.h"

#include <algorithm>
#include <QAbstractButton>
#include <QHBoxLayout>
#include <QLayoutItem>
#include <QMessageBox>
#include <QMetaObject>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWidget>

#include "ElaFlowLayout.h"
#include "ElaMessageBar.h"
#include "ElaPushButton.h"
#include "ElaScrollPageArea.h"
#include "ElaText.h"
#include "Interface/DataModel.h"
#include "Resource/ui/SimulationPreviewCard.h"
#include "Utils/SimulationChartRenderer.h"
#include "spdlog/spdlog.h"

Simulation::Simulation(QWidget* parent)
    : BasePage(parent) {
    _statusLabel = new QLabel(this);
    _statusLabel->setWordWrap(true);
    _statusLabel->setMinimumHeight(44);

    _startButton = new ElaPushButton(QStringLiteral("开始仿真"), this);
    _cancelButton = new ElaPushButton(QStringLiteral("取消任务"), this);
    _cancelButton->setEnabled(false);

    _gallerySectionTitle = new ElaText(QStringLiteral("结果总览"), this);
    _gallerySectionTitle->setTextPixelSize(18);

    _detailSectionTitle = new ElaText(QStringLiteral("图表详情"), this);
    _detailSectionTitle->setTextPixelSize(18);

    _galleryArea = new ElaScrollPageArea(this);
    auto* galleryAreaLayout = new QVBoxLayout(_galleryArea);
    galleryAreaLayout->setContentsMargins(16, 16, 16, 16);
    galleryAreaLayout->setSpacing(0);
    _galleryLayout = new ElaFlowLayout(0, 12, 12);
    _galleryLayout->setContentsMargins(0, 0, 0, 0);
    _galleryLayout->setIsAnimation(true);
    galleryAreaLayout->addLayout(_galleryLayout);

    _detailArea = new ElaScrollPageArea(this);
    _detailArea->setMinimumHeight(620);
    _detailArea->setMaximumHeight(620);
    auto* detailAreaLayout = new QVBoxLayout(_detailArea);
    detailAreaLayout->setContentsMargins(16, 16, 16, 16);
    detailAreaLayout->setSpacing(8);

    _detailChartTitle = new ElaText(this);
    _detailChartTitle->setTextPixelSize(20);
    _detailChartSubtitle = new ElaText(this);
    _detailChartSubtitle->setTextPixelSize(13);
    _detailChartSummary = new ElaText(this);
    _detailChartSummary->setTextPixelSize(13);
    _detailChartSummary->setIsWrapAnywhere(false);

    _detailStack = new QStackedWidget(this);
    _detailStack->setMinimumHeight(420);

    _detailEmptyPage = new QWidget(this);
    auto* emptyLayout = new QVBoxLayout(_detailEmptyPage);
    emptyLayout->setContentsMargins(0, 32, 0, 32);
    _detailEmptyLabel = new QLabel(_detailEmptyPage);
    _detailEmptyLabel->setAlignment(Qt::AlignCenter);
    _detailEmptyLabel->setWordWrap(true);
    emptyLayout->addStretch();
    emptyLayout->addWidget(_detailEmptyLabel);
    emptyLayout->addStretch();

    _scalarPlot = new QCustomPlot(this);
    _seriesPlot = new QCustomPlot(this);
    _matrixPlot = new QCustomPlot(this);
    _scalarPlot->setMinimumHeight(420);
    _seriesPlot->setMinimumHeight(420);
    _matrixPlot->setMinimumHeight(420);

    _detailStack->addWidget(_detailEmptyPage);
    _detailStack->addWidget(_scalarPlot);
    _detailStack->addWidget(_seriesPlot);
    _detailStack->addWidget(_matrixPlot);

    detailAreaLayout->addWidget(_detailChartTitle);
    detailAreaLayout->addWidget(_detailChartSubtitle);
    detailAreaLayout->addWidget(_detailChartSummary);
    detailAreaLayout->addWidget(_detailStack);

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(_startButton);
    buttonLayout->addWidget(_cancelButton);
    buttonLayout->addStretch();

    auto* centralWidget = new QWidget(this);
    centralWidget->setWindowTitle(QStringLiteral("仿真"));
    auto* centerVLayout = new QVBoxLayout(centralWidget);
    centerVLayout->setContentsMargins(0, 0, 0, 0);
    centerVLayout->setSpacing(12);
    centerVLayout->addWidget(_statusLabel);
    centerVLayout->addLayout(buttonLayout);
    centerVLayout->addWidget(_gallerySectionTitle);
    centerVLayout->addWidget(_galleryArea);
    centerVLayout->addWidget(_detailSectionTitle);
    centerVLayout->addWidget(_detailArea);
    addCentralWidget(centralWidget, true, false, 0);

    connect(_startButton, &ElaPushButton::clicked, this, &Simulation::on_StartSimulate_clicked);
    connect(_cancelButton, &ElaPushButton::clicked, this, &Simulation::on_CancelSimulate_clicked);

    refreshGalleryAreaHeight();
    resetDetailView();
    refreshStatusText();
}

Simulation::~Simulation() {
    requestStop();
    joinWorkerIfNeeded();
    _scheduler.reset();
}

bool Simulation::isBusy() const {
    return _state == TaskState::Running || _state == TaskState::Cancelling;
}

void Simulation::on_StartSimulate_clicked() {
    if (isBusy()) {
        return;
    }

    joinWorkerIfNeeded();

    DataModel* model = DataModel::instance();
    const auto validationResult = model->validateCurrentModel();
    if (!validationResult.first) {
        spdlog::error(
            "Current model validation failed before simulation: {}",
            validationResult.second.toStdString());
        QMessageBox::warning(this, QStringLiteral("输入校验失败"), validationResult.second);
        return;
    }

    const DataModel::DataSnapshot snapshot = model->createSnapshot();
    _scheduler = std::make_unique<simSchedulerCtx>(
        ModelType::PE,
        snapshot,
        FormationSource::ManualInput,
        std::nullopt);
    setState(
        TaskState::Running,
        _hasDirtyInputs ? QStringLiteral("存在未保存草稿，本次仿真只使用当前已保存模型") : QString());
    spdlog::info("Simulation launched with frozen DataModel snapshot.");

    _workerThread = std::thread([this]() {
        simSchedulerCtx* scheduler = _scheduler.get();
        if (!scheduler) {
            return;
        }

        SimulationTaskResult taskResult = scheduler->run();
        QMetaObject::invokeMethod(
            this,
            [this, taskResult = std::move(taskResult)]() mutable {
                onWorkerFinished(std::move(taskResult));
            },
            Qt::QueuedConnection);
    });
}

void Simulation::on_CancelSimulate_clicked() {
    if (_state != TaskState::Running) {
        return;
    }

    requestStop();
    setState(TaskState::Cancelling, QStringLiteral("已请求取消，等待当前计算步骤结束"));
    spdlog::info("Simulation cancellation requested.");
}

void Simulation::onInputDraftStateChanged(bool hasDirtyInputs) {
    _hasDirtyInputs = hasDirtyInputs;
    refreshStatusText();
}

void Simulation::onInputModelCommitted() {
    refreshStatusText();
}

void Simulation::clearPreviewCards() {
    while (_galleryLayout && _galleryLayout->count() > 0) {
        QLayoutItem* item = _galleryLayout->takeAt(0);
        if (!item) {
            break;
        }
        if (QWidget* widget = item->widget()) {
            delete widget;
        }
        delete item;
    }
    _previewCards.clear();
    _selectedChartKey.reset();
    refreshGalleryAreaHeight();
}

void Simulation::refreshGalleryAreaHeight() {
    if (!_galleryArea) {
        return;
    }

    constexpr int cardWidth = 360;
    constexpr int cardHeight = 128;
    constexpr int spacing = 12;
    constexpr int horizontalMargins = 32;
    constexpr int verticalMargins = 32;
    constexpr int emptyHeight = 120;

    const int cardCount = static_cast<int>(_previewCards.size());
    if (cardCount <= 0) {
        _galleryArea->setMinimumHeight(emptyHeight);
        _galleryArea->setMaximumHeight(emptyHeight);
        return;
    }

    const int availableWidth = std::max(1, _galleryArea->width() - horizontalMargins);
    const int columns = std::max(1, (availableWidth + spacing) / (cardWidth + spacing));
    const int rows = (cardCount + columns - 1) / columns;
    const int contentHeight = verticalMargins + rows * cardHeight + std::max(0, rows - 1) * spacing;

    _galleryArea->setMinimumHeight(contentHeight);
    _galleryArea->setMaximumHeight(contentHeight);
}

void Simulation::rebuildResultGallery(const SimulationTaskResult& taskResult) {
    clearPreviewCards();

    const std::vector<SimulationChartCardDescriptor> cards = SimulationResultCatalog::buildCards(taskResult);
    for (const SimulationChartCardDescriptor& cardDescriptor : cards) {
        const SimulationChartPayload payload = SimulationResultCatalog::payloadForKey(taskResult, cardDescriptor.key);

        auto* card = new SimulationPreviewCard(_galleryArea);
        card->setTitle(cardDescriptor.title);
        card->setSubTitle(cardDescriptor.subtitle);
        card->setCardPixmap(SimulationChartRenderer::renderPreviewPixmap(payload));
        card->setSelected(false);
        card->setEnabled(cardDescriptor.available);

        connect(card, &QAbstractButton::clicked, this, [this, key = cardDescriptor.key]() {
            selectChart(key);
        });

        _galleryLayout->addWidget(card);
        _previewCards.emplace_back(cardDescriptor.key, card);
    }

    refreshGalleryAreaHeight();
}

void Simulation::resetDetailView() {
    _detailChartTitle->setText(QStringLiteral("暂无仿真结果"));
    _detailChartSubtitle->setText(QStringLiteral("运行仿真后查看六类主图"));
    _detailChartSummary->setText(QStringLiteral("详情区将展示场图、曲线图或矩阵热图。"));
    _detailEmptyLabel->setText(QStringLiteral("运行一次成功仿真后，这里会显示图表详情。"));
    _detailStack->setCurrentWidget(_detailEmptyPage);
}

void Simulation::selectChart(SimulationChartKey key) {
    _selectedChartKey = key;
    updateSelectedCardState();
    renderSelectedChart();
}

void Simulation::renderSelectedChart() {
    if (!_lastSuccessfulResult.has_value() || !_selectedChartKey.has_value()) {
        resetDetailView();
        return;
    }

    const SimulationChartPayload payload = SimulationResultCatalog::payloadForKey(
        _lastSuccessfulResult.value(),
        _selectedChartKey.value());

    _detailChartTitle->setText(payload.title);
    _detailChartSubtitle->setText(payload.subtitle);

    if (!payload.available) {
        _detailChartSummary->setText(
            payload.detailSummary.isEmpty()
                ? QStringLiteral("当前结果缺少该图数据。")
                : QStringLiteral("当前结果缺少该图数据。\n%1").arg(payload.detailSummary));
        _detailEmptyLabel->setText(QStringLiteral("该图在当前结果中不可用。"));
        _detailStack->setCurrentWidget(_detailEmptyPage);
        return;
    }

    _detailChartSummary->setText(payload.detailSummary);

    switch (payload.payloadType) {
    case SimulationChartPayloadType::ScalarField2D:
        if (payload.scalarField) {
            SimulationChartRenderer::renderScalarFieldDetail(
                *payload.scalarField,
                payload.key,
                _scalarPlot,
                false);
            _detailStack->setCurrentWidget(_scalarPlot);
            return;
        }
        break;

    case SimulationChartPayloadType::Series1D:
        if (payload.primarySeries) {
            SimulationChartRenderer::renderSeriesDetail(
                *payload.primarySeries,
                payload.secondarySeries,
                payload.key,
                _seriesPlot,
                false);
            _detailStack->setCurrentWidget(_seriesPlot);
            return;
        }
        break;

    case SimulationChartPayloadType::LabeledMatrix2D:
        if (payload.matrix) {
            SimulationChartRenderer::renderMatrixDetail(
                *payload.matrix,
                payload.key,
                _matrixPlot,
                false);
            _detailStack->setCurrentWidget(_matrixPlot);
            return;
        }
        break;
    }

    _detailEmptyLabel->setText(QStringLiteral("未能渲染当前图表。"));
    _detailStack->setCurrentWidget(_detailEmptyPage);
}

void Simulation::updateSelectedCardState() {
    for (const auto& previewCard : _previewCards) {
        previewCard.second->setSelected(
            _selectedChartKey.has_value() &&
            previewCard.first == _selectedChartKey.value());
    }
}

void Simulation::setState(TaskState state, const QString& detail) {
    const bool previousBusy = isBusy();
    _state = state;
    _stateDetail = detail;
    refreshStatusText();

    _startButton->setEnabled(!isBusy());
    _cancelButton->setEnabled(_state == TaskState::Running);

    const bool currentBusy = isBusy();
    if (previousBusy != currentBusy) {
        emit busyStateChanged(currentBusy);
    }
}

void Simulation::refreshStatusText() {
    QString text = stateText(_state);
    if (!_stateDetail.isEmpty()) {
        text += QStringLiteral("：") + _stateDetail;
    }
    if (!isBusy() && hasStaleSuccessfulResult()) {
        text += QStringLiteral("\n当前结果对应旧输入。");
    }
    _statusLabel->setText(text);
}

void Simulation::joinWorkerIfNeeded() {
    if (_workerThread.joinable()) {
        _workerThread.join();
    }
}

void Simulation::requestStop() {
    if (_scheduler) {
        _scheduler->requestStop();
    }
}

void Simulation::onWorkerFinished(SimulationTaskResult taskResult) {
    joinWorkerIfNeeded();
    _lastFinishedResult = taskResult;

    const auto validation = taskResult.validate();
    if (!validation.first) {
        const QString errorMessage =
            QStringLiteral("任务结果非法：%1，已保留上一张成功结果。").arg(validation.second);
        spdlog::error("Simulation task result validation failed: {}", validation.second.toStdString());
        setState(TaskState::Failed, errorMessage);
        ElaMessageBar::error(ElaMessageBarType::BottomRight, QStringLiteral("结果非法"), errorMessage, 2500, this);
        _scheduler.reset();
        refreshStatusText();
        return;
    }

    if (taskResult.status == SimulationResultStatus::Succeeded) {
        if (taskResult.aggregatedField.values.empty()) {
            const QString errorMessage = QStringLiteral("仿真返回空结果，已保留上一张成功结果。");
            spdlog::warn(errorMessage.toStdString());
            setState(TaskState::Failed, errorMessage);
            ElaMessageBar::warning(ElaMessageBarType::BottomRight, QStringLiteral("结果为空"), errorMessage, 2000, this);
            _scheduler.reset();
            refreshStatusText();
            return;
        }

        const std::optional<SimulationTaskResult> previousSuccessfulResult = _lastSuccessfulResult;
        const std::optional<SimulationChartKey> previousSelectedChart = _selectedChartKey;

        try {
            spdlog::info("Building simulation result gallery from SimulationTaskResult.");
            _lastSuccessfulResult = taskResult;
            rebuildResultGallery(taskResult);
            selectChart(SimulationChartKey::AggregatedField);
            setState(TaskState::Succeeded, taskResult.summaryText);
        } catch (const std::exception& e) {
            _lastSuccessfulResult = previousSuccessfulResult;
            if (_lastSuccessfulResult.has_value()) {
                rebuildResultGallery(_lastSuccessfulResult.value());
                if (previousSelectedChart.has_value()) {
                    selectChart(previousSelectedChart.value());
                } else {
                    selectChart(SimulationChartKey::AggregatedField);
                }
            } else {
                clearPreviewCards();
                resetDetailView();
            }

            const QString errorMessage = QStringLiteral("绘图失败：%1").arg(QString::fromUtf8(e.what()));
            spdlog::error("Simulation painting failed: {}", e.what());
            setState(TaskState::Failed, errorMessage);
            QMessageBox::critical(this, QStringLiteral("绘图错误"), errorMessage);
        }
    } else if (taskResult.status == SimulationResultStatus::Cancelled) {
        setState(TaskState::Cancelled, QStringLiteral("仿真已取消，保留上一张成功结果。"));
        ElaMessageBar::warning(
            ElaMessageBarType::BottomRight,
            QStringLiteral("任务已取消"),
            QStringLiteral("取消请求已生效，本次不会覆盖已有结果"),
            2000,
            this);
    } else {
        QString errorMessage = taskResult.errorMessage;
        if (errorMessage.isEmpty()) {
            errorMessage = QStringLiteral("仿真失败，已保留上一张成功结果。");
        }
        setState(TaskState::Failed, errorMessage);
        ElaMessageBar::error(ElaMessageBarType::BottomRight, QStringLiteral("仿真失败"), errorMessage, 2500, this);
    }

    _scheduler.reset();
    refreshStatusText();
}

bool Simulation::hasStaleSuccessfulResult() const {
    if (!_lastSuccessfulResult.has_value()) {
        return false;
    }
    return _hasDirtyInputs ||
           (DataModel::instance()->createSnapshot() != _lastSuccessfulResult->inputSnapshot);
}

void Simulation::resizeEvent(QResizeEvent* event) {
    BasePage::resizeEvent(event);
    refreshGalleryAreaHeight();
}

QString Simulation::stateText(TaskState state) const {
    switch (state) {
    case TaskState::Idle:
        return QStringLiteral("空闲，尚未执行仿真");
    case TaskState::Running:
        return QStringLiteral("仿真运行中");
    case TaskState::Cancelling:
        return QStringLiteral("正在取消仿真");
    case TaskState::Succeeded:
        return QStringLiteral("最近一次仿真成功");
    case TaskState::Failed:
        return QStringLiteral("最近一次仿真失败");
    case TaskState::Cancelled:
        return QStringLiteral("最近一次仿真已取消");
    }
    return QStringLiteral("未知状态");
}
