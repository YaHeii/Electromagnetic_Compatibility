#include "Simulation.h"
#include <Eigen/Dense>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QMetaObject>
#include <QVBoxLayout>
#include "Utils/PaintImage.hpp"
#include "ElaMessageBar.h"
#include "ElaPushButton.h"
#include "Interface/DataModel.h"
#include "Interface/TransferToEngin.h"
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
    _emcEngine.reset();
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
        spdlog::error("Current model validation failed before simulation: {}", validationResult.second.toStdString());
        QMessageBox::warning(this, QStringLiteral("输入校验失败"), validationResult.second);
        return;
    }

    const DataSnapshot snapshot = model->createSnapshot();
    auto fleet = TransferToEngine::convertDataModelToFleet(snapshot);
    if (!fleet) {
        const QString errorMessage = QStringLiteral("无法从当前快照构建 Fleet，仿真未启动");
        spdlog::error(errorMessage.toStdString());
        setState(TaskState::Failed, errorMessage);
        ElaMessageBar::error(ElaMessageBarType::BottomRight, QStringLiteral("启动失败"), errorMessage, 2000, this);
        return;
    }

    _emcEngine = std::make_unique<EMC_Engine>(ModelType::PE, std::move(fleet), snapshot);
    setState(
        TaskState::Running,
        _hasDirtyInputs ? QStringLiteral("存在未保存草稿，本次仿真只使用当前已保存模型") : QString());
    spdlog::info("Simulation launched with frozen DataModel snapshot.");

    _workerThread = std::thread([this]() {
        EMC_Engine* engine = _emcEngine.get();
        if (!engine) {
            return;
        }

        engine->do_PE_computing();
        QMetaObject::invokeMethod(this, [this]() { onWorkerFinished(); }, Qt::QueuedConnection);
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
    if (_emcEngine) {
        _emcEngine->stop();
    }
}

void Simulation::onWorkerFinished() {
    joinWorkerIfNeeded();
    if (!_emcEngine) {
        setState(TaskState::Idle);
        return;
    }

    if (_emcEngine->completedSuccessfully()) {
        const GridMap& result = _emcEngine->lossGrid();
        if (result.empty() || result.front().empty()) {
            const QString errorMessage = QStringLiteral("仿真返回空结果，已保留上一次成功图像");
            spdlog::warn(errorMessage.toStdString());
            setState(TaskState::Failed, errorMessage);
            ElaMessageBar::warning(ElaMessageBarType::BottomRight, QStringLiteral("结果为空"), errorMessage, 2000, this);
            _emcEngine.reset();
            return;
        }

        try {
            spdlog::info("Painting simulation result to QCustomPlot.");
            PEmodel_Painting2D(result, _plot);
            _lastSuccessfulSnapshot = _emcEngine->inputSnapshot();
            _hasLastSuccessfulSnapshot = true;
            setState(TaskState::Succeeded, QStringLiteral("仿真完成"));
        } catch (const std::exception& e) {
            const QString errorMessage = QStringLiteral("绘图失败：%1").arg(QString::fromUtf8(e.what()));
            spdlog::error("Simulation painting failed: {}", e.what());
            setState(TaskState::Failed, errorMessage);
            QMessageBox::critical(this, QStringLiteral("绘图错误"), errorMessage);
        }
    } else if (_emcEngine->wasCancelled()) {
        setState(TaskState::Cancelled, QStringLiteral("仿真已取消，保留上一次成功结果"));
        ElaMessageBar::warning(
            ElaMessageBarType::BottomRight,
            QStringLiteral("任务已取消"),
            QStringLiteral("取消请求已生效，本次不会覆盖已有结果"),
            2000,
            this);
    } else {
        QString errorMessage = _emcEngine->lastErrorMessage();
        if (errorMessage.isEmpty()) {
            errorMessage = QStringLiteral("仿真失败，已保留上一次成功结果");
        }
        setState(TaskState::Failed, errorMessage);
        ElaMessageBar::error(ElaMessageBarType::BottomRight, QStringLiteral("仿真失败"), errorMessage, 2500, this);
    }

    _emcEngine.reset();
    refreshStatusText();
}

bool Simulation::hasStaleSuccessfulResult() const {
    if (!_hasLastSuccessfulSnapshot) {
        return false;
    }
    return _hasDirtyInputs || !(DataModel::instance()->createSnapshot() == _lastSuccessfulSnapshot);
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
