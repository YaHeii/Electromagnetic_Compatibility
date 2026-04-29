#include "ui_mainwindow.h"
#include "mainwindow.h"
#include <QMessageBox>
#include <QMetaType> 
#include "Simulation.h"
#include "LogWidget.h"
#include "ElaContentDialog.h"
#include "ElaDockWidget.h"
#include "ElaEventBus.h"
#include "ElaMenu.h"
#include "ElaMenuBar.h"
#include "ElaStatusBar.h"
#include "ElaText.h"
#include "ElaTheme.h"
#include "ElaToolBar.h"
#include "ElaToolButton.h"

#include "Home.h"
#include "About.h"
#include "Setting.h"
#include "ExamplePage/T_BaseComponents.h"



#include "ExamplePage/T_Icon.h"
#include "LogWidget.h"
#include "ExamplePage/T_Popup.h"

MainWindow::MainWindow(QWidget *parent)
    : ElaWindow(parent),
    ui(new Ui::MainWindow) {
    initWindow();

    ////额外布局
    initEdgeLayout();

    ////中心窗口
    initContent();

    // 拦截默认关闭事件
    _closeDialog = new ElaContentDialog(this);
    connect(_closeDialog, &ElaContentDialog::rightButtonClicked, this, &MainWindow::closeWindow);
    connect(_closeDialog, &ElaContentDialog::middleButtonClicked, this, [=]() {
        _closeDialog->close();
        showMinimized();
        });
    this->setIsDefaultClosed(false);
    connect(this, &MainWindow::closeButtonClicked, this, [=]() {_closeDialog->exec(); });
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

void MainWindow::initWindow() {
    setFocusPolicy(Qt::StrongFocus);
    //创建面板
    // resize(1200, 740);
    setWindowTitle("无人船舰队电磁预测系统");
    setUserInfoCardPixmap(QPixmap(":/Image/Cirno.jpg"));
    setUserInfoCardTitle("无人船舰队电磁预测系统");
    //setUserInfoCardSubTitle("Liniyous@gmail.com");

    ElaText* centralStack = new ElaText("这是一个主窗口堆栈页面", this);
    centralStack->setFocusPolicy(Qt::StrongFocus);
    centralStack->setTextPixelSize(32);
    centralStack->setAlignment(Qt::AlignCenter);
    addCentralWidget(centralStack);

    setWindowPixmap(ElaThemeType::Light, QPixmap(":/WindowBase/Miku.png"));
    setWindowPixmap(ElaThemeType::Dark, QPixmap(":/WindowBase/WorldTree.jpg"));
    setWindowMoviePath(ElaThemeType::Light, ":/WindowBase/Miku.gif");
    setWindowMoviePath(ElaThemeType::Dark, ":/WindowBase/WorldTree.gif");



    ElaDockWidget* logDock = new ElaDockWidget("运行日志", this);

    logDock->setFixedWidth(300);
    logDock->setAllowedAreas(Qt::RightDockWidgetArea);
    logDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    // 或者完全固定：
    // logDock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    _logWidget = new LogWidget(this);
    logDock->setWidget(_logWidget);
    this->addDockWidget(Qt::RightDockWidgetArea, logDock);

    // 自定义AppBar菜单
    ElaMenu* appBarMenu = new ElaMenu(this);
    appBarMenu->setMenuItemHeight(27);
    connect(appBarMenu->addAction("跳转到一级主要堆栈"), &QAction::triggered, this, [=]() {
        setCurrentStackIndex(0);
        });
    connect(appBarMenu->addAction("跳转到二级主要堆栈"), &QAction::triggered, this, [=]() {
        setCurrentStackIndex(1);
        });
    connect(appBarMenu->addAction("更改页面切换特效(Scale)"), &QAction::triggered, this, [=]() {
        setStackSwitchMode(ElaWindowType::StackSwitchMode::Scale);
        });
    connect(appBarMenu->addElaIconAction(ElaIconType::GearComplex, "自定义主窗口设置"), &QAction::triggered, this, [=]() {
        navigation(_settingKey);
        });
    appBarMenu->addSeparator();
    connect(appBarMenu->addElaIconAction(ElaIconType::MoonStars, "更改项目主题"), &QAction::triggered, this, [=]() {
        eTheme->setThemeMode(eTheme->getThemeMode() == ElaThemeType::Light ? ElaThemeType::Dark : ElaThemeType::Light);
        });
    connect(appBarMenu->addAction("使用原生菜单"), &QAction::triggered, this, [=]() {
        setCustomMenu(nullptr);
        });
    setCustomMenu(appBarMenu);
}

// REVIEW: 参考侧边栏
void MainWindow::initEdgeLayout()
{
    //菜单栏
    ElaMenuBar* menuBar = new ElaMenuBar(this);
    menuBar->setFixedHeight(30);
    QWidget* customWidget = new QWidget(this);
    customWidget->setFixedWidth(500);
    QVBoxLayout* customLayout = new QVBoxLayout(customWidget);
    customLayout->setContentsMargins(0, 0, 0, 0);
    customLayout->addWidget(menuBar);
    customLayout->addStretch();
    // this->setMenuBar(menuBar);
    this->setCustomWidget(ElaAppBarType::MiddleArea, customWidget);

    menuBar->addElaIconAction(ElaIconType::AtomSimple, "动作菜单");
    ElaMenu* iconMenu = menuBar->addMenu(ElaIconType::Aperture, "图标菜单");
    iconMenu->setMenuItemHeight(27);
    iconMenu->addElaIconAction(ElaIconType::BoxCheck, "排序方式", QKeySequence::SelectAll);
    iconMenu->addElaIconAction(ElaIconType::Copy, "复制");
    iconMenu->addElaIconAction(ElaIconType::MagnifyingGlassPlus, "显示设置");
    iconMenu->addSeparator();
    iconMenu->addElaIconAction(ElaIconType::ArrowRotateRight, "刷新");
    iconMenu->addElaIconAction(ElaIconType::ArrowRotateLeft, "撤销");
    menuBar->addSeparator();
    ElaMenu* shortCutMenu = new ElaMenu("快捷菜单(&A)", this);
    shortCutMenu->setMenuItemHeight(27);
    shortCutMenu->addElaIconAction(ElaIconType::BoxCheck, "排序方式", QKeySequence::Find);
    shortCutMenu->addElaIconAction(ElaIconType::Copy, "复制");
    shortCutMenu->addElaIconAction(ElaIconType::MagnifyingGlassPlus, "显示设置");
    shortCutMenu->addSeparator();
    shortCutMenu->addElaIconAction(ElaIconType::ArrowRotateRight, "刷新");
    shortCutMenu->addElaIconAction(ElaIconType::ArrowRotateLeft, "撤销");
    menuBar->addMenu(shortCutMenu);

    menuBar->addMenu("样例菜单(&B)")->addElaIconAction(ElaIconType::ArrowRotateRight, "样例选项");
    menuBar->addMenu("样例菜单(&C)")->addElaIconAction(ElaIconType::ArrowRotateRight, "样例选项");
    menuBar->addMenu("样例菜单(&E)")->addElaIconAction(ElaIconType::ArrowRotateRight, "样例选项");
    menuBar->addMenu("样例菜单(&F)")->addElaIconAction(ElaIconType::ArrowRotateRight, "样例选项");
    menuBar->addMenu("样例菜单(&G)")->addElaIconAction(ElaIconType::ArrowRotateRight, "样例选项");

    //工具栏
    ElaToolBar* toolBar = new ElaToolBar("工具栏", this);
    toolBar->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
    toolBar->setToolBarSpacing(3);
    toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    toolBar->setIconSize(QSize(25, 25));
    // toolBar->setFloatable(false);
    // toolBar->setMovable(false);


    //TODO: 使用这个按钮唤醒QFileDialog ,
    // 1. 定义文件过滤器（支持标准 json  jsonc）     
    // 2. 获取默认打开路径（例如：桌面或文档）
    // 3. 唤醒原生对话框
    // 4. 路径有效性校验
    // 5. JsonLoader 进行解析 成功后需要反馈到ui界面,失败输出日志 
    ElaToolButton* toolButton1 = new ElaToolButton(this);
    toolButton1->setElaIcon(ElaIconType::Coins);
    toolBar->addWidget(toolButton1);
    toolBar->addSeparator();
   
    QAction* test = new QAction(this);
    test->setMenu(new QMenu(this));

    // ElaProgressBar* progressBar = new ElaProgressBar(this);
    // progressBar->setMinimum(0);
    // progressBar->setMaximum(0);
    // progressBar->setFixedWidth(350);
    // toolBar->addWidget(progressBar);

    this->addToolBar(Qt::TopToolBarArea, toolBar);


    // 底部状态栏
    ElaStatusBar* statusBar = new ElaStatusBar(this);
    ElaText* statusText = new ElaText("初始化成功！", this);
    statusText->setTextPixelSize(14);
    statusBar->addWidget(statusText);
    this->setStatusBar(statusBar);
}

// REVIEW:参考页面
void MainWindow::initContent()
{
    _homePage = new Home(this);
    _shipPage = new ShipWidget(this);
    _devicePage = new DeviceWidget(this);
    _iconPage = new T_Icon(this);
    _baseComponentsPage = new T_BaseComponents(this);
    _settingPage = new Setting(this);
    Simulation* SimulationWidget = new Simulation(this);

 
    QString testKey_1;
    QString testKey_2;
    navigation("HOME");
    addPageNode("HOME", _homePage, ElaIconType::House);
    addPageNode("设备参数", _devicePage, ElaIconType::House);
    addPageNode("船只参数", _shipPage, ElaIconType::House);
    addPageNode("仿真", SimulationWidget, ElaIconType::ChartSimple);
    addPageNode("ElaBaseComponents", _baseComponentsPage, ElaIconType::CabinetFiling);
    addPageNode("ElaIcon", _iconPage, 99, ElaIconType::FontCase);

    addFooterNode("About", nullptr, _aboutKey, 0, ElaIconType::User);
    _aboutPage = new About();

    _aboutPage->hide();
    connect(this, &ElaWindow::navigationNodeClicked, this, [=](ElaNavigationType::NavigationNodeType nodeType, QString nodeKey) {
        if (_aboutKey == nodeKey)
        {
            _aboutPage->moveToCenter();
            _aboutPage->show();
        }
    });
    addFooterNode("Setting", _settingPage, _settingKey, 0, ElaIconType::GearComplex);
    connect(this, &MainWindow::userInfoCardClicked, this, [=]() {
        this->navigation(_homePage->property("ElaPageKey").toString());
    });

    connect(_homePage, &Home::elaBaseComponentNavigation, this, [=]() {
        this->navigation(_baseComponentsPage->property("ElaPageKey").toString());
    });

    connect(_homePage, &Home::elaIconNavigation, this, [=]() {
        this->navigation(_iconPage->property("ElaPageKey").toString());
    });

    //_windowSuggestBox->addSuggestion(getNavigationSuggestDataList());
    qDebug() << "已注册的事件列表" << ElaEventBus::getInstance()->getRegisteredEventsName();
}

void on_toolButton10_clicked(){
    
}


void MainWindow::mouseReleaseEvent(QMouseEvent* event)
{
    if (getCurrentNavigationIndex() != 2)
    {
        switch (event->button())
        {
        case Qt::BackButton:
        {
            this->setCurrentStackIndex(0);
            break;
        }
        case Qt::ForwardButton:
        {
            this->setCurrentStackIndex(1);
            break;
        }
        default:
        {
            break;
        }
        }
    }
    ElaWindow::mouseReleaseEvent(event);
}
