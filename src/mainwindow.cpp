#include "ui_mainwindow.h"
#include "ui_devicewidget.h"
#include "../include/mainwindow.h"
#include "../resource/ui/shipwidget.h"
#include "../resource/ui/DeviceWidget.h"
#include <QMessageBox>
#include "spdlog/spdlog.h"
#include <QMetaType> // 包含 QMetaType

MainWindow::MainWindow(QWidget *parent)
    : ElaWindow(parent),
    ui(new Ui::MainWindow) {
    ui->setupUi(this);
    _treeView = new TreeViewManager(ui->treeView, this);
 
    // 设置日志控件最大行数
    ui->Debug_Edit->setMaximumBlockCount(5000);
    ui->Error_Edit->setMaximumBlockCount(5000);
 
    // 注册GridMap类型，使其可在信号槽中传递
    qRegisterMetaType<GridMap>("GridMap");
    // 连接仿真完成信号到槽
    connect(this, &MainWindow::simulationDone, this, &MainWindow::onSimulationFinished);
 
    // 初始化日志发射器并连接信号
    _logEmitter = new LogEmitter(this);
    connect(_logEmitter, &LogEmitter::newLog, this, &MainWindow::onLogReceived);
}
 
std::shared_ptr<spdlog::sinks::sink> MainWindow::createGuiLogSink() {
    // 创建并配置 UI Sink
    auto qt_sink = std::make_shared<QtTextEditSink_mt>(_logEmitter);
    qt_sink->set_pattern("[%H:%M:%S.%e] %v");
    return qt_sink;
}

MainWindow::~MainWindow()
{
    // 获取 logger
    auto logger = spdlog::default_logger();

    // 遍历所有 sink，找到我们的 QtTextEditSink 并解绑
    if (logger) {
        for (auto& sink : logger->sinks()) {
            auto qt_sink = std::dynamic_pointer_cast<QtTextEditSink_mt>(sink);
            if (qt_sink) {
                qt_sink->detach();
            }
        }
    }
    delete ui;
}

void MainWindow::onLogReceived(const QString& message, int level)
{
    auto logLevel = static_cast<spdlog::level::level_enum>(level);
    std::cout << "UI Trace: Slot onLogReceived called. Msg: " << message.toStdString() << std::endl;
    // 1. 设置最大行数 (防止日志无限增长占满内存)
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

void MainWindow::on_addDeviceButton_clicked()
{
    //在添加新控件前，先将UI上所有未保存的修改更新到数据模型中
    updateDeviceModelFromView();
    
    EquipmentData newDevice;
    newDevice.equipmentID = QString("NewDevice%1").arg(DataModel::instance()->allEquipments.size() + 1);
    // 首先在DataModel中占位
    DataModel::instance()->allEquipments.push_back(newDevice);
	// 然后创建新的DeviceWidget并添加到UI
    DeviceWidget *widget = new DeviceWidget(this);
    widget->setData(newDevice); // 关联UI与数据
    ui->deviceLayout->addWidget(widget);
    connect(widget, &DeviceWidget::removalRequested, this, &MainWindow::onDeviceWidgetRemovalRequested);
    connect(widget, &DeviceWidget::removalRequested, this, &MainWindow::onDeviceWidgetRemovalRequested);
    //同步treeView
    _treeView->syncViewWithModel();
}

void MainWindow::on_addShipButton_clicked()
{
    updateShipModelFromView();

    ShipData newShip;
    newShip.shipID = DataModel::instance()->allShips.size() + 1;
    newShip.shipName = QString("NewShip_%1").arg(newShip.shipID);
    DataModel::instance()->allShips.push_back(newShip);
    ShipWidget *widget = new ShipWidget(this);
    widget->setData(newShip); // 关联UI与数据
    ui->shipsLayout->addWidget(widget);
    _treeView->syncViewWithModel();
}

void MainWindow::on_DeviceSave_clicked()
{
    if (updateDeviceModelFromView()) {
        QMessageBox::information(this, "成功", "设备信息已保存并校验通过。");
        spdlog::info("Device data saved and validated.");
    }
}

void MainWindow::on_ShipSave_clicked()
{
    if (updateShipModelFromView()) {
        QMessageBox::information(this, "成功", "舰船信息已保存并校验通过。");
        spdlog::info("Ship data saved and validated.");
    }
}

bool MainWindow::updateShipModelFromView()
{
    // 1. 从 View 同步到 Model
    for (int i = 0; i < ui->shipsLayout->count(); ++i) {
        ShipWidget* widget = qobject_cast<ShipWidget*>(ui->shipsLayout->itemAt(i)->widget());
        if (widget) {
            widget->updateShipModelData(); // 这是一个 void 函数，只负责赋值
        }
    }

    // 2. 执行校验逻辑
    // 遍历 DataModel 中的所有船只进行检查
    auto& ships = DataModel::instance()->allShips;
    for (int i = 0; i < ships.size(); ++i) {
        auto result = ships[i].validate_Ship(); // 调用 validate
        if (!result.first) {
            // 校验失败，弹出警告
            QString errorMsg = QString("船只数据错误 (第 %1 个):\n%2").arg(i + 1).arg(result.second);
            QMessageBox::critical(this, "校验失败", errorMsg);
            spdlog::error("Validation failed for ship {}: {}", i, result.second.toStdString());
            return false; // 中断
        }
    }

    // 3. 同步 TreeView (如果校验通过)
    _treeView->syncViewWithModel();
    return true;
}



bool MainWindow::updateDeviceModelFromView()
{
    // 1. 从 View 同步到 Model (这部分逻辑可能分散在各个 DeviceWidget 中，或者您有统一列表)
    // 假设您有机制遍历所有 DeviceWidget，或者 DeviceWidget 已经实时更新了 Model
    // 如果没有统一遍历列表，这里假设 DataModel 已经是 UI 上的最新值（或者您可以在这里遍历 m_deviceList）
    for (int i = 0; i < ui->deviceLayout->count(); ++i) {
        DeviceWidget* widget = qobject_cast<DeviceWidget*>(ui->deviceLayout->itemAt(i)->widget());
        if (widget) {
            // 让每个Widget用自己UI上的当前值去更新数据模型
            // 存入DataModel::instance()->allDevices
            widget->updateModelData();
        }
    }
    
    // 2. 执行校验逻辑
    auto& equipments = DataModel::instance()->allEquipments;
    for (int i = 0; i < equipments.size(); ++i) {
        auto result = equipments[i].validate();
        if (!result.first) {
            QString errorMsg = QString("设备数据错误 (ID: %1):\n%2")
                                .arg(equipments[i].equipmentID)
                                .arg(result.second);
            QMessageBox::critical(this, "校验失败", errorMsg);
            spdlog::error("Validation failed for equipment {}: {}", 
                          equipments[i].equipmentID.toStdString(), result.second.toStdString());
            return false;
        }
    }

    _treeView->syncViewWithModel();
    return true;
}



// void MainWindow::on_StartSimulate_clicked() {
//     std::string Model = "PEModel";
//     PE_data PEdata;
//     Propagation_Engine PE(Model);
//     GridMap Loss2D = PE.PEmodel_computing2D(PEdata, 25);
//     PEmodel_Painting2D(Loss2D, ui->PEmodel_2Dplot);
// }

void MainWindow::on_StartSimulate_clicked() {
    spdlog::info("Simulation requested...");

    // 1. 全局数据同步与校验
    if (!updateShipModelFromView() || !updateDeviceModelFromView()) {
        return;
    }

    // 2. 准备数据快照 (深拷贝以保证线程安全)
    auto dataSnapshot = DataModel::instance()->createSnapshot();

    // 3. UI 状态更新
    ui->StartSimulate->setEnabled(false);
    ui->statusbar->showMessage("正在进行电磁仿真计算...", 0);

    // 清空旧的绘图
    ui->PEmodel_2Dplot->clearPlottables();
    ui->PEmodel_2Dplot->replot();

  //  // 4. 使用 std::async 启动异步计算任务
  //  // 使用数据快照来进行仿真，防止仿真过程中数据更改导致崩溃
  //  std::future<GridMap> future = std::async(std::launch::async, [this, dataSnapshot]() {
  //      // --- 后台线程 ---
  //      auto fleet = TransferToEngine::convertDataModelToFleet(dataSnapshot);
  //      if (!fleet) {
  //          spdlog::error("Fleet conversion failed (nullptr).");
  //          return GridMap();
  //      }

  //      // 注意：_emcengine是在主线程创建的，在后台线程使用需要确保线程安全
  //      // 如果 _emcengine 的方法是可重入的，则无需加锁
  //      _emcEngine = new EMC_Engine(ModelType::PE, std::move(fleet));
  //      connect(_emcEngine, &EMC_Engine::peComputationFinished, this, &MainWindow::onSingleGridMapReady);
  //      spdlog::info("Engine computing 2D loss map...");
		//_emcEngine->do_PE_test();
  //      return GridMap(); // 临时返回
  //  });

  //  // 5. 创建一个分离的线程来等待结果，避免阻塞UI
  //  std::thread(&MainWindow::simulationWaiter, this, std::move(future)).detach();

    //阻塞实现
	auto fleet = TransferToEngine::convertDataModelToFleet(dataSnapshot);
    if (!fleet) {
        spdlog::error("Fleet conversion failed (nullptr).");
        return;
	}
    // 注意：_emcengine是在主线程创建的，在后台线程使用需要确保线程安全
    // 如果 _emcengine 的方法是可重入的，则无需加锁
    _emcEngine = new EMC_Engine(ModelType::PE, std::move(fleet));
    connect(_emcEngine, &EMC_Engine::peComputationFinished, this, &MainWindow::onSingleGridMapReady);
	spdlog::info("Engine computing 2D loss map...");
	// GridMap lossGrid = _emcEngine->do_PE_test();
    // PEmodel_Painting2D(lossGrid, ui->PEmodel_2Dplot);
    _emcEngine->do_Validation_DuctLeakage();
}

void MainWindow::simulationWaiter(std::future<GridMap> future) {
    // --- 等待线程 ---
    try {
        GridMap result = future.get(); // 阻塞直到计算完成
        emit simulationDone(result);   // 发射信号，将结果传递给UI线程
    } catch (const std::exception& e) {
        spdlog::error("Exception in simulation thread: {}", e.what());
        emit simulationDone(GridMap()); // 发射空结果表示失败
    }
}

void MainWindow::onSimulationFinished(const GridMap& result) {
    // --- UI 线程 ---
    
    // 1. 恢复 UI
    ui->StartSimulate->setEnabled(true);
    ui->statusbar->showMessage("仿真完成", 5000);

    // 2. 检查结果
    if (result.empty() || (result.size() > 0 && result[0].empty())) {
        spdlog::warn("Simulation returned empty or invalid result.");
        QMessageBox::warning(this, "仿真警告", "仿真结果为空或无效，无法绘图。");
        return;
    }

    // 3. 调用 PaintImage.hpp 中的函数进行绘图
    // 注意：ui->PEmodel_2Dplot 必须是在 .ui 文件中提升为 QCustomPlot 的 Widget
    try {
        spdlog::info("Painting results to QCustomPlot...");
        
        // 直接调用提供的内联函数
        PEmodel_Painting2D(result, ui->PEmodel_2Dplot);
        
        // 强制刷新显示
        ui->PEmodel_2Dplot->replot();
        
    } catch (const std::exception& e) {
        spdlog::error("Error during painting: {}", e.what());
        QMessageBox::critical(this, "绘图错误", QString("绘图时发生异常: %1").arg(e.what()));
    }
}

void MainWindow::onSingleGridMapReady(const std::string& shipName, const std::string& equipmentName, const GridMap& lossGrid) {
    //    // 直接调用提供的内联函数
    //PEmodel_Painting2D(lossGrid, ui->PEmodel_2Dplot);
    //
    //// 强制刷新显示
    //ui->PEmodel_2Dplot->replot();
    // 这里可以处理单张图返回的逻辑，比如更新某个特定的UI组件
    spdlog::debug("Received single GridMap for ship: {}, equipment: {}", shipName, equipmentName);
    // 您可以选择将其绘制在一个新的窗口或特定的绘图区域
}

void MainWindow::onDeviceWidgetRemovalRequested(const QString &id)
{
    // 1. 从DataModel中移除对应的数据
    auto& equipments = DataModel::instance()->allEquipments;
    auto it = std::remove_if(equipments.begin(), equipments.end(),
        [&](const EquipmentData& ed) { return ed.equipmentID == id; });
    
    if (it != equipments.end()) {
        equipments.erase(it, equipments.end());
        spdlog::info("设备 {} 的数据已从模型中删除。", id.toStdString());

        // 2. 遍历布局，找到并删除对应的UI控件
        for (int i = 0; i < ui->deviceLayout->count(); ++i) {
            QLayoutItem* item = ui->deviceLayout->itemAt(i);
            if (item && item->widget()) {
                DeviceWidget* widget = qobject_cast<DeviceWidget*>(item->widget());
                // 假设DeviceWidget有方法可以获取其ID
                if (widget && widget->getID() == id) {
                    ui->deviceLayout->removeWidget(widget);
                    widget->deleteLater();
                    spdlog::info("设备 {} 的UI控件已删除。", id.toStdString());
                    break; // 找到并删除后即可退出循环
                }
            }
        }

        // 3. 更新TreeView
        _treeView->syncViewWithModel();
    } else {
        spdlog::warn("请求删除设备 {}，但在数据模型中未找到。", id.toStdString());
    }
}