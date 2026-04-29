#include "mainwindow.h"

#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QMetaType>
#include <QMouseEvent>
#include <QVBoxLayout>

#include "About.h"
#include "ElaContentDialog.h"
#include "ElaDef.h"
#include "ElaDockWidget.h"
#include "ElaEventBus.h"
#include "ElaMenu.h"
#include "ElaMenuBar.h"
#include "ElaMessageBar.h"
#include "ElaStatusBar.h"
#include "ElaText.h"
#include "ElaTheme.h"
#include "ElaToolBar.h"
#include "ElaToolButton.h"
#include "ExamplePage/T_BaseComponents.h"
#include "ExamplePage/T_Icon.h"
#include "Home.h"
#include "LogWidget.h"
#include "Setting.h"
#include "ui_mainwindow.h"
#include "Utils/JsonLoader.hpp"

MainWindow::MainWindow(QWidget* parent)
    : ElaWindow(parent),
      ui(new Ui::MainWindow) {
    initWindow();
    initEdgeLayout();
    initContent();

    _closeDialog = new ElaContentDialog(this);
    connect(_closeDialog, &ElaContentDialog::rightButtonClicked, this, &MainWindow::closeWindow);
    connect(_closeDialog, &ElaContentDialog::middleButtonClicked, this, [this]() {
        _closeDialog->close();
        showMinimized();
    });
    setIsDefaultClosed(false);
    connect(this, &MainWindow::closeButtonClicked, this, [this]() { _closeDialog->exec(); });
}

MainWindow::~MainWindow() {
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
    setWindowTitle(QStringLiteral("无人艇编队电磁预测系统"));
    setUserInfoCardPixmap(QPixmap(":/Image/Cirno.jpg"));
    setUserInfoCardTitle(QStringLiteral("无人艇编队电磁预测系统"));

    auto* centralStack = new ElaText(QStringLiteral("这是一个主窗口堆栈页面"), this);
    centralStack->setFocusPolicy(Qt::StrongFocus);
    centralStack->setTextPixelSize(32);
    centralStack->setAlignment(Qt::AlignCenter);
    addCentralWidget(centralStack);

    setWindowPixmap(ElaThemeType::Light, QPixmap(":/WindowBase/Miku.png"));
    setWindowPixmap(ElaThemeType::Dark, QPixmap(":/WindowBase/WorldTree.jpg"));
    setWindowMoviePath(ElaThemeType::Light, ":/WindowBase/Miku.gif");
    setWindowMoviePath(ElaThemeType::Dark, ":/WindowBase/WorldTree.gif");

    auto* logDock = new ElaDockWidget(QStringLiteral("运行日志"), this);
    logDock->setFixedWidth(300);
    logDock->setAllowedAreas(Qt::RightDockWidgetArea);
    logDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    _logWidget = new LogWidget(this);
    logDock->setWidget(_logWidget);
    addDockWidget(Qt::RightDockWidgetArea, logDock);

    auto* appBarMenu = new ElaMenu(this);
    appBarMenu->setMenuItemHeight(27);
    connect(appBarMenu->addAction(QStringLiteral("跳转到一级主要堆栈")), &QAction::triggered, this, [this]() {
        setCurrentStackIndex(0);
    });
    connect(appBarMenu->addAction(QStringLiteral("跳转到二级主要堆栈")), &QAction::triggered, this, [this]() {
        setCurrentStackIndex(1);
    });
    connect(appBarMenu->addAction(QStringLiteral("更改页面切换特效(Scale)")), &QAction::triggered, this, [this]() {
        setStackSwitchMode(ElaWindowType::StackSwitchMode::Scale);
    });
    connect(
        appBarMenu->addElaIconAction(ElaIconType::GearComplex, QStringLiteral("自定义主窗口设置")),
        &QAction::triggered,
        this,
        [this]() { navigation(_settingKey); });
    appBarMenu->addSeparator();
    connect(appBarMenu->addElaIconAction(ElaIconType::MoonStars, QStringLiteral("更改项目主题")), &QAction::triggered, this, [this]() {
        eTheme->setThemeMode(eTheme->getThemeMode() == ElaThemeType::Light ? ElaThemeType::Dark : ElaThemeType::Light);
    });
    connect(appBarMenu->addAction(QStringLiteral("使用原生菜单")), &QAction::triggered, this, [this]() {
        setCustomMenu(nullptr);
    });
    setCustomMenu(appBarMenu);
}

void MainWindow::initEdgeLayout() {
    auto* menuBar = new ElaMenuBar(this);
    menuBar->setFixedHeight(30);

    auto* customWidget = new QWidget(this);
    customWidget->setFixedWidth(500);
    auto* customLayout = new QVBoxLayout(customWidget);
    customLayout->setContentsMargins(0, 0, 0, 0);
    customLayout->addWidget(menuBar);
    customLayout->addStretch();
    setCustomWidget(ElaAppBarType::MiddleArea, customWidget);

    menuBar->addElaIconAction(ElaIconType::AtomSimple, QStringLiteral("动作菜单"));
    auto* iconMenu = menuBar->addMenu(ElaIconType::Aperture, QStringLiteral("图标菜单"));
    iconMenu->setMenuItemHeight(27);
    iconMenu->addElaIconAction(ElaIconType::BoxCheck, QStringLiteral("排序方式"), QKeySequence::SelectAll);
    iconMenu->addElaIconAction(ElaIconType::Copy, QStringLiteral("复制"));
    iconMenu->addElaIconAction(ElaIconType::MagnifyingGlassPlus, QStringLiteral("显示设置"));
    iconMenu->addSeparator();
    iconMenu->addElaIconAction(ElaIconType::ArrowRotateRight, QStringLiteral("刷新"));
    iconMenu->addElaIconAction(ElaIconType::ArrowRotateLeft, QStringLiteral("撤销"));
    menuBar->addSeparator();

    auto* shortCutMenu = new ElaMenu(QStringLiteral("快捷菜单(&A)"), this);
    shortCutMenu->setMenuItemHeight(27);
    shortCutMenu->addElaIconAction(ElaIconType::BoxCheck, QStringLiteral("排序方式"), QKeySequence::Find);
    shortCutMenu->addElaIconAction(ElaIconType::Copy, QStringLiteral("复制"));
    shortCutMenu->addElaIconAction(ElaIconType::MagnifyingGlassPlus, QStringLiteral("显示设置"));
    shortCutMenu->addSeparator();
    shortCutMenu->addElaIconAction(ElaIconType::ArrowRotateRight, QStringLiteral("刷新"));
    shortCutMenu->addElaIconAction(ElaIconType::ArrowRotateLeft, QStringLiteral("撤销"));
    menuBar->addMenu(shortCutMenu);

    menuBar->addMenu(QStringLiteral("样例菜单(&B)"))->addElaIconAction(ElaIconType::ArrowRotateRight, QStringLiteral("样例选项"));
    menuBar->addMenu(QStringLiteral("样例菜单(&C)"))->addElaIconAction(ElaIconType::ArrowRotateRight, QStringLiteral("样例选项"));
    menuBar->addMenu(QStringLiteral("样例菜单(&E)"))->addElaIconAction(ElaIconType::ArrowRotateRight, QStringLiteral("样例选项"));
    menuBar->addMenu(QStringLiteral("样例菜单(&F)"))->addElaIconAction(ElaIconType::ArrowRotateRight, QStringLiteral("样例选项"));
    menuBar->addMenu(QStringLiteral("样例菜单(&G)"))->addElaIconAction(ElaIconType::ArrowRotateRight, QStringLiteral("样例选项"));

    auto* toolBar = new ElaToolBar(QStringLiteral("工具栏"), this);
    toolBar->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
    toolBar->setToolBarSpacing(3);
    toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    toolBar->setIconSize(QSize(25, 25));

    _importJsonButton = new ElaToolButton(this);
    _importJsonButton->setElaIcon(ElaIconType::Coins);
    _importJsonButton->setToolTip(QStringLiteral("导入 JSON/JSONC"));
    connect(_importJsonButton, &ElaToolButton::clicked, this, &MainWindow::importJsonConfig);
    toolBar->addWidget(_importJsonButton);
    toolBar->addSeparator();
    addToolBar(Qt::TopToolBarArea, toolBar);

    auto* statusBar = new ElaStatusBar(this);
    auto* statusText = new ElaText(QStringLiteral("初始化成功"), this);
    statusText->setTextPixelSize(14);
    statusBar->addWidget(statusText);
    setStatusBar(statusBar);
}

void MainWindow::initContent() {
    _homePage = new Home(this);
    _shipPage = new ShipWidget(this);
    _devicePage = new DeviceWidget(this);
    _environmentPage = new EnvironmentWidget(this);
    _simulationPage = new Simulation(this);
    _iconPage = new T_Icon(this);
    _baseComponentsPage = new T_BaseComponents(this);
    _settingPage = new Setting(this);

    navigation("HOME");
    addPageNode("HOME", _homePage, ElaIconType::House);
    addPageNode(QStringLiteral("环境参数"), _environmentPage, ElaIconType::EarthAmericas);
    addPageNode(QStringLiteral("设备参数"), _devicePage, ElaIconType::BoxCheck);
    addPageNode(QStringLiteral("船只参数"), _shipPage, ElaIconType::Ship);
    addPageNode(QStringLiteral("仿真"), _simulationPage, ElaIconType::ChartSimple);
    addPageNode("ElaBaseComponents", _baseComponentsPage, ElaIconType::CabinetFiling);
    addPageNode("ElaIcon", _iconPage, 99, ElaIconType::FontCase);

    addFooterNode("About", nullptr, _aboutKey, 0, ElaIconType::User);
    _aboutPage = new About();
    _aboutPage->hide();
    connect(this, &ElaWindow::navigationNodeClicked, this, [this](ElaNavigationType::NavigationNodeType, const QString& nodeKey) {
        if (_aboutKey == nodeKey) {
            _aboutPage->moveToCenter();
            _aboutPage->show();
        }
    });
    addFooterNode("Setting", _settingPage, _settingKey, 0, ElaIconType::GearComplex);

    connect(this, &MainWindow::userInfoCardClicked, this, [this]() {
        navigation(_homePage->property("ElaPageKey").toString());
    });
    connect(_homePage, &Home::elaBaseComponentNavigation, this, [this]() {
        navigation(_baseComponentsPage->property("ElaPageKey").toString());
    });
    connect(_homePage, &Home::elaIconNavigation, this, [this]() {
        navigation(_iconPage->property("ElaPageKey").toString());
    });

    connect(_devicePage, &DeviceWidget::equipmentsCommitted, _shipPage, &ShipWidget::refreshEquipmentReferences);

    connect(_environmentPage, &EnvironmentWidget::dirtyStateChanged, this, [this](bool) { updateSimulationDraftState(); });
    connect(_devicePage, &DeviceWidget::dirtyStateChanged, this, [this](bool) { updateSimulationDraftState(); });
    connect(_shipPage, &ShipWidget::dirtyStateChanged, this, [this](bool) { updateSimulationDraftState(); });

    connect(_environmentPage, &EnvironmentWidget::modelCommitted, this, [this]() {
        _simulationPage->onInputModelCommitted();
        updateSimulationDraftState();
    });
    connect(_devicePage, &DeviceWidget::modelCommitted, this, [this]() {
        _simulationPage->onInputModelCommitted();
        updateSimulationDraftState();
    });
    connect(_shipPage, &ShipWidget::modelCommitted, this, [this]() {
        _simulationPage->onInputModelCommitted();
        updateSimulationDraftState();
    });

    connect(_simulationPage, &Simulation::busyStateChanged, this, [this](bool busy) {
        setEditorsReadOnly(busy);
        if (_importJsonButton) {
            _importJsonButton->setEnabled(!busy);
        }
    });

    updateSimulationDraftState();
    _simulationPage->onInputModelCommitted();

    qDebug() << "Registered events" << ElaEventBus::getInstance()->getRegisteredEventsName();
}

void MainWindow::importJsonConfig() {
    if (_simulationPage && _simulationPage->isBusy()) {
        return;
    }

    const QString filePath = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("导入 JSON/JSONC"),
        QDir::homePath(),
        QStringLiteral("JSON Files (*.json *.jsonc);;All Files (*.*)"));
    if (filePath.isEmpty()) {
        return;
    }

    if (!JsonLoader::LoadFile(filePath)) {
        spdlog::error("JSON import failed: {}", filePath.toStdString());
        ElaMessageBar::error(
            ElaMessageBarType::BottomRight,
            QStringLiteral("导入失败"),
            QStringLiteral("文件解析或语义校验失败，当前模型未被改写"),
            2500,
            this);
        return;
    }

    spdlog::info("JSON imported successfully: {}", filePath.toStdString());
    reloadEditorsFromModel();
    _simulationPage->onInputModelCommitted();
    updateSimulationDraftState();
    ElaMessageBar::success(
        ElaMessageBarType::BottomRight,
        QStringLiteral("导入成功"),
        QStringLiteral("已从 %1 回填环境、设备和船只页面").arg(QFileInfo(filePath).fileName()),
        1800,
        this);
}

void MainWindow::reloadEditorsFromModel() {
    _environmentPage->loadFromModel();
    _devicePage->loadFromModel();
    _shipPage->loadFromModel();
    _shipPage->refreshEquipmentReferences();
}

void MainWindow::updateSimulationDraftState() {
    const bool hasDraft =
        (_environmentPage && _environmentPage->isDirty()) ||
        (_devicePage && _devicePage->isDirty()) ||
        (_shipPage && _shipPage->isDirty());
    if (_simulationPage) {
        _simulationPage->onInputDraftStateChanged(hasDraft);
    }
}

void MainWindow::setEditorsReadOnly(bool readOnly) {
    if (_environmentPage) {
        _environmentPage->setReadOnly(readOnly);
    }
    if (_devicePage) {
        _devicePage->setReadOnly(readOnly);
    }
    if (_shipPage) {
        _shipPage->setReadOnly(readOnly);
    }
}

void MainWindow::mouseReleaseEvent(QMouseEvent* event) {
    if (getCurrentNavigationIndex() != 2) {
        switch (event->button()) {
        case Qt::BackButton:
            setCurrentStackIndex(0);
            break;
        case Qt::ForwardButton:
            setCurrentStackIndex(1);
            break;
        default:
            break;
        }
    }
    ElaWindow::mouseReleaseEvent(event);
}
