#pragma once
#include "Utils/PaintImage.hpp"
#include "Simulation/EMC_Engine.h"
#include "spdlog/spdlog.h"
#include <QWidget>
#include <QTabWidget>
#include <QVBoxLayout>
#include "qcustomplot.h"
#include <ElaPushButton.h>
#include <future>
#include "BasePage.h"

class Simulation : public BasePage {
	Q_OBJECT

public:
	explicit Simulation(QWidget* parent = nullptr);
	~Simulation();

signals:
	// 用于从工作线程安全地将结果传递到UI线程
	void simulationDone(const GridMap& result);

private:
	EMC_Engine* _emcEngine;
	std::thread _workerThread;

	// 2D Power Distribution Tab
	QCustomPlot* PEmodel2Dplot;
	ElaPushButton* StartSimulate;
public slots:
	void on_StartSimulate_clicked();
	// 槽函数现在接收GridMap作为参数
	void onSimulationFinished(const GridMap& result);
};
