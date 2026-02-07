#include "Simulation.h"
#include "Interface/DataModel.h"
#include "Interface/TransferToEngin.h"
#include <QMessageBox>
#include <thread>
#include <future>
#include "Utils/JsonLoader.hpp"
Simulation::Simulation(QWidget* parent)
	: QWidget(parent), _emcEngine(nullptr) {
    setupUI();

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
void Simulation::setupUI()
{
    // Main layout
    mainLayout = new QVBoxLayout(this);
    
    // Tab widget
    tabWidget = new QTabWidget(this);
    tabWidget->setCurrentIndex(0);
    
    // 2D Power Distribution Tab
    simulateTab = new QWidget();
    simulateTabLayout = new QVBoxLayout(simulateTab);
    
    PEmodel2Dplot = new QCustomPlot(simulateTab);
    simulateTabLayout->addWidget(PEmodel2Dplot);
    
    StartSimulate = new ElaPushButton("仿真", simulateTab);
    simulateTabLayout->addWidget(StartSimulate);
    
    tabWidget->addTab(simulateTab, "二维功率损耗分布");
    
    // Tab 2 (placeholder)
    tab2 = new QWidget();
    tabWidget->addTab(tab2, "Tab 2");
    
    mainLayout->addWidget(tabWidget);
    
    // Connect signals
    connect(StartSimulate, &ElaPushButton::clicked, this, &Simulation::on_StartSimulate_clicked);
}

void Simulation::on_StartSimulate_clicked() {
    spdlog::info("Simulation requested...");
    if (_workerThread.joinable()) {
        _workerThread.join();
    }

    JsonLoader::LoadFile("D:/code/Electromagnetic_compatibility/tests/Test.json");
    auto dataSnapshot = DataModel::instance()->createSnapshot();
    
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
        // 强制刷新显示
        PEmodel2Dplot->replot();

    }
    catch (const std::exception& e) {
        spdlog::error("Error during painting: {}", e.what());
        QMessageBox::critical(this, "绘图错误", QString("绘图时发生异常: %1").arg(e.what()));
    }
}

