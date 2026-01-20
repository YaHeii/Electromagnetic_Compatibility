#include "ui_mainwindow.h"
#include "ui_devicewidget.h"
#include "../include/mainwindow.h"
#include <QMessageBox>
#include "spdlog/spdlog.h"
#include <QMetaType> 
#include "Simulation.h"
#include "FleetInput.h"
#include "LogWidget.h"
#include "ElaWindow.h"
#include "ElaDockWidget.h"

MainWindow::MainWindow(QWidget *parent)
    : ElaWindow(parent),
    ui(new Ui::MainWindow) {
    //创建面板
    resize(1200, 800); 
    setWindowTitle("无人船舰队电磁预测系统");
    
    FleetInput* FleetWidget = new FleetInput(this);
    Simulation* SimulationWidget = new Simulation(this);

    addPageNode("编队参数", FleetWidget, ElaIconType::House);
    addPageNode("仿真", SimulationWidget, ElaIconType::ChartSimple);
    
    navigation("HOME");

    ElaDockWidget* logDock = new ElaDockWidget("运行日志", this);

    logDock->setFixedWidth(300); 

    logDock->setAllowedAreas(Qt::RightDockWidgetArea);
    logDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable); 
    // 或者完全固定：
    // logDock->setFeatures(QDockWidget::NoDockWidgetFeatures);

    _logWidget = new LogWidget(this);

    logDock->setWidget(_logWidget);

    this->addDockWidget(Qt::RightDockWidgetArea, logDock);
    // ui->treeView->setStyleSheet("background-color: transparent; border: none;");
    // ui->treeView->viewport()->setAttribute(Qt::WA_TranslucentBackground);
    // 如果能被菜单栏控制显示/隐藏
    // routePage->addMenuAction(logDock->toggleViewAction());
}

MainWindow::~MainWindow()
{
    delete ui;
}

std::shared_ptr<spdlog::sinks::sink> MainWindow::createGuiLogSink() {
    if (_logWidget) {
        return _logWidget->createGuiLogSink();
    }
    return nullptr;
}
