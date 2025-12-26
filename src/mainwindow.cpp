#include "ui_mainwindow.h"
#include "ui_devicewidget.h"
#include "../include/mainwindow.h"
#include "../resource/ui/shipwidget.h"
#include "../resource/ui/DeviceWidget.h"
#include <QMessageBox>
#include "../include/utils/TransferToEngin.h"
#include "spdlog/spdlog.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow) {
    
    ui->setupUi(this);
    m_treeView = new TreeViewManager(ui->treeView, this);  
    ui->Debug_Edit->appendPlainText("=== DEBUG TEXT ===");
    m_logEmitter = new LogEmitter(this);
    connect(m_logEmitter, &LogEmitter::newLog, this, &MainWindow::onLogReceived);

    // 2. 【核心修改】获取现有的全局 Logger
    // spdlog::default_logger() 返回的是 main.cpp 中 set_default_logger 设置的那个指针
	auto logger = spdlog::default_logger();

    if (logger) {
        // 3. 创建并挂载 UI Sink
        auto qt_sink = std::make_shared<QtTextEditSink_mt>(m_logEmitter);
        qt_sink->set_pattern("[%H:%M:%S.%e] %v");

        // 将 Sink 加到 logger 的接收列表中
        logger->sinks().push_back(qt_sink);

        spdlog::debug("UI Sink attached successfully via spdlog::get!");
    }
    else {
        // 如果为空，说明 main.cpp 里的 init_logger 可能没执行或者名字写错了
        ui->Debug_Edit->appendHtml(QString("<font color='green'>Critical Error: Global logger not found!</font>"));
    }
	//测试日志输出
	ui->Debug_Edit->appendHtml(QString("<font color='green'>Logger initialized and UI Sink attached.</font>"));
    
}
MainWindow::~MainWindow()
{
    // 获取 logger
    auto logger = spdlog::default_logger();

    // 遍历所有 sink，找到我们的 QtTextEditSink 并解绑
    for (auto& sink : logger->sinks()) {
        // dynamic_pointer_cast 尝试转换类型
        auto qt_sink = std::dynamic_pointer_cast<QtTextEditSink_mt>(sink);
        if (qt_sink) {
            qt_sink->detach(); // 安全断开连接
        }
    }
    delete ui;
}
void MainWindow::on_addDeviceButton_clicked()
{
    //在添加新控件前，先将UI上所有未保存的修改更新到数据模型中
    updateDeviceModelFromView();

    DeviceData newDevice;
    newDevice.equipmentID = QString("NewDevice_%1").arg(DataModel::instance()->allDevices.count() + 1);
    DataModel::instance()->allDevices.append(newDevice);

    DeviceWidget *widget = new DeviceWidget(this);  // 修正：设置父对象
    widget->setData(newDevice); // 用新的空数据填充它
    ui->deviceLayout->addWidget(widget);
    m_treeView->syncViewWithModel();  // 修正：调用TreeView同步方法
}

void MainWindow::on_addShipButton_clicked()
{
    updateShipModelFromView();

    ShipData newShip;
    newShip.shipID = DataModel::instance()->allShips.count() + 1;
    newShip.shipName = QString("NewShip_%1").arg(newShip.shipID);
    DataModel::instance()->allShips.append(newShip);

    ShipWidget *widget = new ShipWidget(this);  // 修正：设置父对象
    widget->setData(newShip);
    ui->shipsLayout->addWidget(widget);
    m_treeView->syncViewWithModel();  // 修正：调用TreeView同步方法
}

void MainWindow::on_DeviceSave_clicked()
{
    updateDeviceModelFromView();
    QMessageBox::information(this, "成功", "所有设备更改已应用到数据模型。");
    m_treeView->syncViewWithModel();
}

void MainWindow::on_ShipSave_clicked()
{
    updateShipModelFromView();
    QMessageBox::information(this, "成功", "所有舰船更改已应用到数据模型。");
    m_treeView->syncViewWithModel();
}

void MainWindow::updateDeviceModelFromView()
{
    for (int i = 0; i < ui->deviceLayout->count(); ++i) {
        DeviceWidget* widget = qobject_cast<DeviceWidget*>(ui->deviceLayout->itemAt(i)->widget());
        if (widget) {
            // 让每个Widget用自己UI上的当前值去更新数据模型
            widget->updateModelData();
        }
    }
    m_treeView->syncViewWithModel();  // 修正：调用TreeView同步方法
}

void MainWindow::updateShipModelFromView()
{
    for (int i = 0; i < ui->shipsLayout->count(); ++i) {
        ShipWidget *widget = qobject_cast<ShipWidget*>(ui->shipsLayout->itemAt(i)->widget());
        if (widget) {
            // 让每个Widget用自己UI上的当前值去更新数据模型
            widget->updateShipModelData();
        }
    }
    m_treeView->syncViewWithModel();  // 修正：调用TreeView同步方法
}

void MainWindow::on_StartSimulate_clicked() {
    string Model = "PEModel";
    PE_data PEdata;
    Propagation_Engine PE(Model);
    GridMap Loss2D = PE.PEmodel_computing2D(PEdata, 25);
	PEmodel_Painting2D(Loss2D, ui->PEmodel_2Dplot);

}

void MainWindow::onLogReceived(const QString& message, int level)
{
    auto logLevel = static_cast<spdlog::level::level_enum>(level);

    // 1. 设置最大行数 (防止日志无限增长占满内存)
    // 建议在构造函数里设置，但这里写是为了演示
    const int maxBlockCount = 5000;
    if (ui->Debug_Edit->maximumBlockCount() == 0) {
        ui->Debug_Edit->setMaximumBlockCount(maxBlockCount);
        ui->Error_Edit->setMaximumBlockCount(maxBlockCount);
    }

    // 2. 根据级别输出
    if (logLevel == spdlog::level::err || logLevel == spdlog::level::critical) {
        // Error 级别 -> 红色高亮
        // QPlainTextEdit 支持 appendHtml 来显示颜色
        ui->Error_Edit->appendHtml(QString("<font color='#FF0000'>%1</font>").arg(message));
    }
    else if (logLevel == spdlog::level::debug) {
        // Debug 级别 -> 纯文本 (性能最高)
        ui->Debug_Edit->appendPlainText(message);
    }
    else if (logLevel == spdlog::level::info) {
        // Info 级别 -> 绿色高亮
        ui->Debug_Edit->appendHtml(QString("<font color='green'>%1</font>").arg(message));
    }
}