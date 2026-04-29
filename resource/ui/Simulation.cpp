#include "Simulation.h"
#include "Interface/DataModel.h"
#include "Interface/TransferToEngin.h"
#include <QMessageBox>
#include <thread>
Simulation::Simulation(QWidget* parent)
	: BasePage(parent), _emcEngine(nullptr) {

    // 2D Power Distribution
    PEmodel2Dplot = new QCustomPlot();
    StartSimulate = new ElaPushButton("开始仿真",this);

    QWidget* centralWidget = new QWidget(this);
    centralWidget->setWindowTitle("仿真");
    QVBoxLayout* centerVLayout = new QVBoxLayout(centralWidget);
    centerVLayout->setContentsMargins(0, 0, 0, 0);
    centerVLayout->addWidget(PEmodel2Dplot);
    centerVLayout->addWidget(StartSimulate);
    //centerVLayout->addStretch();
    addCentralWidget(centralWidget, true, false, 0);

    // Connect signals
    connect(StartSimulate, &ElaPushButton::clicked, this, &Simulation::on_StartSimulate_clicked);

}

Simulation::~Simulation() {
    if (_emcEngine) {
        _emcEngine->stop(); 
    }

    if (_workerThread.joinable()) {
        _workerThread.join(); // 阻塞主线程，直到子线程 return
    }
    if (_emcEngine) {
        delete _emcEngine;
        _emcEngine = nullptr;
    }
}

void Simulation::on_StartSimulate_clicked() {
    spdlog::info("Simulation requested...");
    if (_workerThread.joinable()) {
        _workerThread.join();
    }

    auto* model = DataModel::instance();
    const auto validationResult = model->validateCurrentModel();
    if (!validationResult.first) {
        spdlog::error("当前输入快照未通过核心校验: {}", validationResult.second.toStdString());
        QMessageBox::warning(this, "输入校验失败", validationResult.second);
        return;
    }
    const auto dataSnapshot = model->createSnapshot();
    
    StartSimulate->setEnabled(false); // 禁用按钮防止重复点击
    PEmodel2Dplot->clearPlottables();
    PEmodel2Dplot->replot();

    auto fleet = TransferToEngine::convertDataModelToFleet(dataSnapshot);
    if (!fleet) {
        spdlog::error("Fleet conversion failed (nullptr).");
        StartSimulate->setEnabled(true);
        return;
    }

    if (_emcEngine) {
        delete _emcEngine;
        _emcEngine = nullptr;
    }

    _emcEngine = new EMC_Engine(ModelType::PE, std::move(fleet));
    connect(_emcEngine, &EMC_Engine::peComputationFinished, this, &Simulation::onSimulationFinished);
    
    spdlog::info("Engine computing 2D loss map");
    _workerThread = std::thread([this]() {
        if (this->_emcEngine) {
            this->_emcEngine->do_PE_computing();
        }
       
        QMetaObject::invokeMethod(this, [this]() {
            // 只有当 this 还存在时才会执行这里
            this->StartSimulate->setEnabled(true);
            spdlog::info("Simulation finished.");
        }, Qt::QueuedConnection);
    });
}

void Simulation::onSimulationFinished(const GridMap& result) {
    // statusbar->showMessage("仿真完成", 5000);
    if (result.empty() || (result.size() > 0 && result[0].empty())) {
        spdlog::warn("Simulation returned empty or invalid result.");
        QMessageBox::warning(this, "仿真警告", "仿真结果为空或无效，无法绘图。");
        return;
    }

    try {
        spdlog::info("Painting results to QCustomPlot...");
        PEmodel_Painting2D(result, PEmodel2Dplot);

    }
    catch (const std::exception& e) {
        spdlog::error("Error during painting: {}", e.what());
        QMessageBox::critical(this, "绘图错误", QString("绘图时发生异常: %1").arg(e.what()));
    }
}
