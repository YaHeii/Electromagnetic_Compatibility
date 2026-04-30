#pragma once

#include <memory>
#include <optional>
#include <thread>
#include <utility>
#include <vector>

#include <QLabel>

#include "BasePage.h"
#include "Resource/ui/SimulationResultCatalog.h"
#include "Simulation/simSchedulerCtx.h"

class ElaPushButton;
class ElaScrollPageArea;
class ElaText;
class QStackedWidget;
class QCustomPlot;
class ElaFlowLayout;
class SimulationPreviewCard;
class QResizeEvent;

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
    void clearPreviewCards();
    void refreshGalleryAreaHeight();
    void rebuildResultGallery(const SimulationTaskResult& taskResult);
    void resetDetailView();
    void selectChart(SimulationChartKey key);
    void renderSelectedChart();
    void updateSelectedCardState();
    void setState(TaskState state, const QString& detail = QString());
    void refreshStatusText();
    void joinWorkerIfNeeded();
    void requestStop();
    void onWorkerFinished(SimulationTaskResult taskResult);
    bool hasStaleSuccessfulResult() const;
    QString stateText(TaskState state) const;

    std::unique_ptr<simSchedulerCtx> _scheduler;
    std::thread _workerThread;
    ElaPushButton* _startButton{nullptr};
    ElaPushButton* _cancelButton{nullptr};
    QLabel* _statusLabel{nullptr};
    ElaText* _gallerySectionTitle{nullptr};
    ElaText* _detailSectionTitle{nullptr};
    ElaScrollPageArea* _galleryArea{nullptr};
    ElaScrollPageArea* _detailArea{nullptr};
    ElaFlowLayout* _galleryLayout{nullptr};
    ElaText* _detailChartTitle{nullptr};
    ElaText* _detailChartSubtitle{nullptr};
    ElaText* _detailChartSummary{nullptr};
    QStackedWidget* _detailStack{nullptr};
    QWidget* _detailEmptyPage{nullptr};
    QLabel* _detailEmptyLabel{nullptr};
    QCustomPlot* _scalarPlot{nullptr};
    QCustomPlot* _seriesPlot{nullptr};
    QCustomPlot* _matrixPlot{nullptr};
    TaskState _state{TaskState::Idle};
    QString _stateDetail;
    bool _hasDirtyInputs{false};
    std::optional<SimulationTaskResult> _lastSuccessfulResult;
    std::optional<SimulationTaskResult> _lastFinishedResult;
    std::optional<SimulationChartKey> _selectedChartKey;
    std::vector<std::pair<SimulationChartKey, SimulationPreviewCard*>> _previewCards;

protected:
    void resizeEvent(QResizeEvent* event) override;
};
