#include "ui_mainwindow.h"
#include "ui_devicewidget.h"
#include "../include/mainwindow.h"
#include "../resource/ui/shipwidget.h"
#include "../resource/ui/DeviceWidget.h"
#include <QMessageBox>
#include "../include/models/TransferToEngin.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow) {
    
    ui->setupUi(this);
    m_treeView = new TreeViewManager(ui->treeView, this);  // 修正：正确初始化成员变量
    // syncDeviceViewWithModel(); //调用使加载初始数据
}
MainWindow::~MainWindow()
{
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
//// TODO：设计Path部分
//// TODO: 完成二级筛选
//// TODO：添加海杂波模块
// 将函数改为MainWindow的成员函数
void MainWindow::on_StartSimulate_clicked() {
    // 获取数据模型实例
    DataModel* dataModel = DataModel::instance();
    // 转换为舰队对象
    std::unique_ptr<Fleet> fleet = TransferToEngine::convertDataModelToFleet(dataModel);
    
    if (!fleet) {
        QMessageBox::warning(this, "错误", "舰队数据转换失败！");
        return;
    }
    
    // 使用共享指针管理fleet对象，确保在Path对象使用期间不会被销毁
    std::shared_ptr<Fleet> fleetPtr = std::move(fleet);
    
    //----------------------------------------------创建编队内船的移动路径---------------------------------------------------
    //对路径进行时间采样，通过每个多个采样点来综合判断系统效能
    // 修改为使用共享指针，避免原始指针可能导致的问题
    Path ship0_path(0, 10, fleetPtr.get());
    int t = 60;//输入总计算时间，单位s
    //cin>>t;//输入时间，在整段时间内完成效能评估,单位s
    int t_step_num = 60;//采样点数
    int t_step = t/t_step_num;//时间步长，单位s

    vector<Path> path_list;
    path_list.push_back(ship0_path);
    PathManager Total_Path(1, path_list);
    
    //---------------------------------------------根据时间采样，不断移动编队，计算电磁兼容情况----------------------------------
    for(int t_index = 0; t_index < t_step_num; t_index++){
        // 检查fleet是否仍然有效
        if (!fleetPtr) {
            QMessageBox::warning(this, "错误", "舰队对象已失效！");
            break;
        }
        
        moveModel::move_location(*fleetPtr, t_step, Total_Path);//移动船
        //根据频率是否在接收机范围进行筛选

        //对有指向性的天线进行筛选

        //使用自由空间衰减计算最坏的传输情况
        FreeSpaceModel prop_modle_FREE;//采用自由空间衰减模型
        EMCEngine EMC_engine(prop_modle_FREE);//实例化
        vector<InterferenceResult> results = EMC_engine.analyzeFleet(*fleetPtr);//存储编队内部所有的电磁兼容情况
        
        // 添加结果检查，避免访问空vector
        if (!results.empty()) {
            cout << "results: " << results.size() << endl;
            cout << results[0].victim_equip_id << endl;
        } else {
            cout << "无干扰结果" << endl;
        }
    }
}