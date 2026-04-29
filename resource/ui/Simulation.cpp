#include "Simulation.h"

#include <QHBoxLayout>
#include <QMessageBox>
#include <QMetaObject>
#include <QVBoxLayout>

#include "ElaMessageBar.h"
#include "ElaPushButton.h"
#include "Interface/DataModel.h"
#include "Utils/PaintImage.hpp"
#include "spdlog/spdlog.h"

Simulation::Simulation(QWidget* parent)
    : BasePage(parent) {
    _plot = new QCustomPlot(this);
    _statusLabel = new QLabel(this);
    _statusLabel->setWordWrap(true);
    _statusLabel->setMinimumHeight(44);

    _startButton = new ElaPushButton(QStringLiteral("开始仿真"), this);
    _cancelButton = new ElaPushButton(QStringLiteral("取消任务"), this);
    _cancelButton->setEnabled(false);

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(_startButton);
    buttonLayout->addWidget(_cancelButton);
    buttonLayout->addStretch();

    auto* centralWidget = new QWidget(this);
    centralWidget->setWindowTitle(QStringLiteral("仿真"));
    auto* centerVLayout = new QVBoxLayout(centralWidget);
    centerVLayout->setContentsMargins(0, 0, 0, 0);
    centerVLayout->addWidget(_plot);
    centerVLayout->addWidget(_statusLabel);
    centerVLayout->addLayout(buttonLayout);
    addCentralWidget(centralWidget, true, false, 0);

    connect(_startButton, &ElaPushButton::clicked, this, &Simulation::on_StartSimulate_clicked);
    connect(_cancelButton, &ElaPushButton::clicked, this, &Simulation::on_CancelSimulate_clicked);

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
    setState(TaskState::Cancelling, QStringLiteral("已请求取消，等待当前计算步结束"));
    spdlog::info("Simulation cancellation requested.");
}

void Simulation::onInputDraftStateChanged(bool hasDirtyInputs) {
    _hasDirtyInputs = hasDirtyInputs;
    refreshStatusText();
}

void Simulation::onInputModelCommitted() {
    refreshStatusText();
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
            QStringLiteral("任务结果非法：%1，已保留上一张成功结果").arg(validation.second);
        spdlog::error("Simulation task result validation failed: {}", validation.second.toStdString());
        setState(TaskState::Failed, errorMessage);
        ElaMessageBar::error(ElaMessageBarType::BottomRight, QStringLiteral("结果非法"), errorMessage, 2500, this);
        _scheduler.reset();
        refreshStatusText();
        return;
    }

    if (taskResult.status == SimulationResultStatus::Succeeded) {
        if (taskResult.aggregatedField.values.empty()) {
            const QString errorMessage = QStringLiteral("仿真返回空结果，已保留上一张成功结果");
            spdlog::warn(errorMessage.toStdString());
            setState(TaskState::Failed, errorMessage);
            ElaMessageBar::warning(ElaMessageBarType::BottomRight, QStringLiteral("结果为空"), errorMessage, 2000, this);
            _scheduler.reset();
            refreshStatusText();
            return;
        }

        try {
            spdlog::info("Painting simulation result to QCustomPlot.");
            PEmodel_Painting2D(taskResult.aggregatedField, _plot);
            _lastSuccessfulResult = taskResult;
            setState(TaskState::Succeeded, taskResult.summaryText);
        } catch (const std::exception& e) {
            const QString errorMessage = QStringLiteral("绘图失败：%1").arg(QString::fromUtf8(e.what()));
            spdlog::error("Simulation painting failed: {}", e.what());
            setState(TaskState::Failed, errorMessage);
            QMessageBox::critical(this, QStringLiteral("绘图错误"), errorMessage);
        }
    } else if (taskResult.status == SimulationResultStatus::Cancelled) {
        setState(TaskState::Cancelled, QStringLiteral("仿真已取消，保留上一张成功结果"));
        ElaMessageBar::warning(
            ElaMessageBarType::BottomRight,
            QStringLiteral("任务已取消"),
            QStringLiteral("取消请求已生效，本次不会覆盖已有结果"),
            2000,
            this);
    } else {
        QString errorMessage = taskResult.errorMessage;
        if (errorMessage.isEmpty()) {
            errorMessage = QStringLiteral("仿真失败，已保留上一张成功结果");
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
