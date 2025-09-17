#include "mainwindow.h"
#include <QtWidgets> // 包含所有常用控件的头文件

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{

}

MainWindow::~MainWindow()
{
    // C++中，父对象被销毁时，其子对象（如此处所有控件）会自动被销毁
    // 所以这里通常是空的，除非有非Qt对象的指针需要手动delete
}
//
// void MainWindow::setupUI()
// {
//     this->setWindowTitle("舰船编队电磁兼容性分析系统 (纯代码版)");
//     this->resize(1200, 800);
//
//     // --- 1. 创建所有控件实例 ---
//
//     // 左侧
//     elementTreeWidget = new QTreeWidget();
//     elementTreeWidget->setHeaderHidden(true);
//
//     parameterStackedWidget = new QStackedWidget();
//
//     // 右侧
//     interferenceMatrixTable = new QTableWidget();
//
//     // 中央
//     sceneGraphicsView = new QGraphicsView();
//
//     // --- 2. 创建并设置Dock窗口 ---
//
//     // 场景元素Dock
//     QDockWidget *sceneElementsDock = new QDockWidget("场景元素", this);
//     sceneElementsDock->setWidget(elementTreeWidget);
//     addDockWidget(Qt::LeftDockWidgetArea, sceneElementsDock);
//
//     // 参数配置Dock
//     QDockWidget *parametersDock = new QDockWidget("参数配置", this);
//     // 向StackedWidget添加页面
//     QLabel* welcomeLabel = new QLabel("请在左侧选择一个元素进行配置");
//     welcomeLabel->setAlignment(Qt::AlignCenter);
//     QWidget* equipmentPage = new QWidget(); // 参数页
//     QGridLayout* equipmentLayout = new QGridLayout(equipmentPage);
//     equipmentLayout->addWidget(new QLabel("频率(MHz):"), 0, 0);
//     equipmentLayout->addWidget(new QLineEdit(), 0, 1);
//     equipmentLayout->addWidget(new QLabel("功率(dBm):"), 1, 0);
//     equipmentLayout->addWidget(new QLineEdit(), 1, 1);
//     equipmentLayout->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding), 2, 0); // 弹簧
//     parameterStackedWidget->addWidget(welcomeLabel); // Page 0
//     parameterStackedWidget->addWidget(equipmentPage);  // Page 1
//     parametersDock->setWidget(parameterStackedWidget);
//     addDockWidget(Qt::LeftDockWidgetArea, parametersDock);
//
//     // 结果展示Dock
//     QTabWidget* resultsTabWidget = new QTabWidget();
//     resultsTabWidget->addTab(interferenceMatrixTable, "干扰矩阵");
//     resultsTabWidget->addTab(new QWidget(), "干扰曲线"); // 空的曲线图容器
//     QDockWidget *resultsDock = new QDockWidget("分析结果", this);
//     resultsDock->setWidget(resultsTabWidget);
//     addDockWidget(Qt::RightDockWidgetArea, resultsDock);
//
//     // --- 3. 设置中央控件 ---
//     setCentralWidget(sceneGraphicsView);
//
//     // --- 4. 手动连接信号和槽 ---
//     connect(elementTreeWidget, &QTreeWidget::itemClicked, this, &MainWindow::onElementTreeItemClicked);
//
//     // 默认显示参数面板的欢迎页
//     parameterStackedWidget->setCurrentIndex(0);
//
//     // 设置干扰矩阵的样式
//     interferenceMatrixTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
//     interferenceMatrixTable->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
// }

void MainWindow::createActionsAndMenus()
{
    // 文件菜单
    QMenu *fileMenu = menuBar()->addMenu("文件(&F)");

    QAction *newAct = new QAction("新建(&N)", this);
    connect(newAct, &QAction::triggered, this, &MainWindow::onActionNewTriggered);
    fileMenu->addAction(newAct);

    QAction *openAct = new QAction("打开(&O)", this);
    connect(openAct, &QAction::triggered, this, &MainWindow::onActionOpenTriggered);
    fileMenu->addAction(openAct);

    QAction *saveAct = new QAction("保存(&S)", this);
    connect(saveAct, &QAction::triggered, this, &MainWindow::onActionSaveTriggered);
    fileMenu->addAction(saveAct);
    
    fileMenu->addSeparator();

    QAction *exitAct = new QAction("退出(&X)", this);
    connect(exitAct, &QAction::triggered, this, &QMainWindow::close);
    fileMenu->addAction(exitAct);

    // 分析菜单
    QMenu *analysisMenu = menuBar()->addMenu("分析(&A)");
    QAction *startAct = new QAction("开始分析", this);
    connect(startAct, &QAction::triggered, this, &MainWindow::onActionStartAnalysisTriggered);
    analysisMenu->addAction(startAct);
}


// onElementTreeItemClicked 和其他槽函数、数据填充函数的代码几乎不变
// 只需要把 `ui->` 前缀去掉即可。

void MainWindow::onElementTreeItemClicked(QTreeWidgetItem *item, int column)
{
    // ... (代码和之前一样, 只是把 ui->elementTreeWidget 换成 elementTreeWidget)
    updateParameterPanel(item);
}

void MainWindow::updateParameterPanel(QTreeWidgetItem *item)
{
    QString type = item->data(0, Qt::UserRole).toString();
    if (type == "transmitter" || type == "receiver") {
        parameterStackedWidget->setCurrentIndex(1);
    } else {
        parameterStackedWidget->setCurrentIndex(0);
    }
}

// populateSceneTree 和 populateInterferenceMatrix 函数代码也一样，
// 只需要把 ui->elementTreeWidget 换成 elementTreeWidget，
// 把 ui->interferenceMatrixTable 换成 interferenceMatrixTable 即可。
// 槽函数 onActionNewTriggered 等也同理。
void MainWindow::populateSceneTree()
{
    elementTreeWidget->clear();
    // ... (和之前完全一样的逻辑) ...
    QTreeWidgetItem *ship1 = new QTreeWidgetItem(elementTreeWidget);
    ship1->setText(0, "旗舰001号");
    ship1->setData(0, Qt::UserRole, "ship");
    // ...
    elementTreeWidget->expandAll();
}

void MainWindow::populateInterferenceMatrix()
{
    // ... (和之前完全一样的逻辑) ...
    interferenceMatrixTable->setRowCount(1);
    // ...
}

void MainWindow::onActionNewTriggered() { QMessageBox::information(this, "提示", "“新建”功能待实现。"); }
void MainWindow::onActionOpenTriggered() { QMessageBox::information(this, "提示", "“打开”功能待实现。"); }
void MainWindow::onActionSaveTriggered() { QMessageBox::information(this, "提示", "“保存”功能待实现。"); }
void MainWindow::onActionStartAnalysisTriggered() { QMessageBox::information(this, "分析完成", "模拟分析已完成！"); }