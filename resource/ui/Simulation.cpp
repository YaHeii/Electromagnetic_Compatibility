#include "Simulation.h"
#include "models/DataModel.h"
#include "utils/TransferToEngin.h"
#include <QMessageBox>
#include <thread>
#include <future>
#include "utils/JsonLoader.hpp"
Simulation::Simulation(QWidget* parent)
	: QWidget(parent), _emcEngine(nullptr) {
    setupUI();
	// 连接仿真完成信号到槽
	connect(this, &Simulation::simulationDone, this, &Simulation::onSimulationFinished);
}

Simulation::~Simulation() {
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
    if (JsonLoader::LoadFile("D:/code/Electromagnetic_compatibility/tests/Test.json")) {

    }
    // 准备数据快照 (深拷贝以保证线程安全)
    auto dataSnapshot = DataModel::instance()->createSnapshot();

    // UI 状态更新
    StartSimulate->setEnabled(false);
    spdlog::info("正在进行电磁仿真计算...");

    // 清空旧的绘图
    PEmodel2Dplot->clearPlottables();
    PEmodel2Dplot->replot();

    // 阻塞实现
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
    connect(_emcEngine, &EMC_Engine::peComputationFinished, this, &Simulation::onSingleGridMapReady);
    
    spdlog::info("Engine computing 2D loss map...");
    

    _emcEngine->do_PE_computing();
    
    StartSimulate->setEnabled(true);
    spdlog::info("Simulation finished.");
}

void Simulation::simulationWaiter(std::future<GridMap> future) {
    try {
        GridMap result = future.get(); // 阻塞直到计算完成
        emit simulationDone(result);   // 发射信号，将结果传递给UI线程
    }
    catch (const std::exception& e) {
        spdlog::error("Exception in simulation thread: {}", e.what());
        emit simulationDone(GridMap()); // 发射空结果表示失败
    }
}

void Simulation::onSimulationFinished(const GridMap& result) {

    StartSimulate->setEnabled(true);
    // statusbar->showMessage("仿真完成", 5000);

    if (result.empty() || (result.size() > 0 && result[0].empty())) {
        spdlog::warn("Simulation returned empty or invalid result.");
        QMessageBox::warning(this, "仿真警告", "仿真结果为空或无效，无法绘图。");
        return;
    }

    // 3. 调用 PaintImage.hpp 中的函数进行绘图
    try {
        spdlog::info("Painting results to QCustomPlot...");

        // 直接调用提供的内联函数
        PEmodel_Painting2D(result, PEmodel2Dplot);

        // 强制刷新显示
        PEmodel2Dplot->replot();

    }
    catch (const std::exception& e) {
        spdlog::error("Error during painting: {}", e.what());
        QMessageBox::critical(this, "绘图错误", QString("绘图时发生异常: %1").arg(e.what()));
    }
}

void Simulation::onSingleGridMapReady(const GridMap& lossGrid) {
    //spdlog::debug("Received single GridMap for ship: {}, equipment: {}", shipID, equipmentName);
    

    try {
         PEmodel_Painting2D(lossGrid, PEmodel2Dplot);
         PEmodel2Dplot->replot();
    } catch (const std::exception& e) {
        spdlog::error("Error painting single grid map: {}", e.what());
    }
}