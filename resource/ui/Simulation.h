#pragma once
#include "utils/PaintImage.hpp"
#include "models/EMC_Engine.h"
#include "spdlog/spdlog.h"
#include <QWidget>
#include <QTabWidget>
#include <QVBoxLayout>
#include "qcustomplot.h"
#include <ElaPushButton.h>
#include <future>

class Simulation : public QWidget {
	Q_OBJECT

public:
	explicit Simulation(QWidget* parent = nullptr);
	~Simulation();

signals:
	// 用于从工作线程安全地将结果传递到UI线程
	void simulationDone(const GridMap& result);

private:
	void setupUI();
	EMC_Engine* _emcEngine;
	void simulationWaiter(std::future<GridMap> future);
	
	// UI Components
	QVBoxLayout* mainLayout;
	QTabWidget* tabWidget;
	
	// 2D Power Distribution Tab
	QWidget* simulateTab;
	QVBoxLayout* simulateTabLayout;
	QCustomPlot* PEmodel2Dplot;
	ElaPushButton* StartSimulate;
	
	// Tab 2 (placeholder)
	QWidget* tab2;

public slots:
	void on_StartSimulate_clicked();
	// 槽函数现在接收GridMap作为参数
	void onSimulationFinished(const GridMap& result);
	//单张图返回
	void onSingleGridMapReady(const std::string& shipName, const std::string& equipmentName, const GridMap& lossGrid);

};
