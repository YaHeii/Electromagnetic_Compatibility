#pragma once

#include <memory>
#include <thread>

#include <QLabel>

#include "BasePage.h"
#include "Simulation/EMC_Engine.h"

#include "qcustomplot.h"

class ElaPushButton;

class Simulation : public BasePage {
    Q_OBJECT

public:
    enum class TaskState {
        Idle,
        Running,
        Cancelling,
        Succeeded,
        Failed,
        Cancelled
    };

    explicit Simulation(QWidget* parent = nullptr);
    ~Simulation() override;

    bool isBusy() const;

signals:
    void busyStateChanged(bool busy);

public slots:
    void on_StartSimulate_clicked();
    void on_CancelSimulate_clicked();
    void onInputDraftStateChanged(bool hasDirtyInputs);
    void onInputModelCommitted();

private:
    using DataSnapshot = DataModel::DataSnapshot;

    void setState(TaskState state, const QString& detail = QString());
    void refreshStatusText();
    void joinWorkerIfNeeded();
    void requestStop();
    void onWorkerFinished();
    bool hasStaleSuccessfulResult() const;
    QString stateText(TaskState state) const;

    std::unique_ptr<EMC_Engine> _emcEngine;
    std::thread _workerThread;
    QCustomPlot* _plot{nullptr};
    ElaPushButton* _startButton{nullptr};
    ElaPushButton* _cancelButton{nullptr};
    QLabel* _statusLabel{nullptr};
    TaskState _state{TaskState::Idle};
    QString _stateDetail;
    bool _hasDirtyInputs{false};
    bool _hasLastSuccessfulSnapshot{false};
    DataSnapshot _lastSuccessfulSnapshot;
};
