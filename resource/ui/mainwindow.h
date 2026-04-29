#pragma once

#include <memory>
#include <QMouseEvent>
#include <QObject>
#include <QRect>
#include <QString>

#include <spdlog/spdlog.h>

#include "DeviceWidget.h"
#include "ElaWindow.h"
#include "EnvironmentWidget.h"
#include "Simulation.h"
#include "shipwidget.h"

namespace Ui {
class MainWindow;
}

class About;
class ElaContentDialog;
class ElaSuggestBox;
class ElaToolButton;
class Home;
class LogWidget;
class Setting;
class T_BaseComponents;
class T_Icon;

class MainWindow : public ElaWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    std::shared_ptr<spdlog::sinks::sink> createGuiLogSink();

    void initWindow();
    void initEdgeLayout();
    void initContent();

protected:
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    void importJsonConfig();
    void reloadEditorsFromModel();
    void updateSimulationDraftState();
    void setEditorsReadOnly(bool readOnly);

    Ui::MainWindow* ui;
    LogWidget* _logWidget{nullptr};
    ElaContentDialog* _closeDialog{nullptr};
    ElaSuggestBox* _windowSuggestBox{nullptr};
    ElaToolButton* _importJsonButton{nullptr};
    QString _settingKey;

    Home* _homePage{nullptr};
    ShipWidget* _shipPage{nullptr};
    DeviceWidget* _devicePage{nullptr};
    EnvironmentWidget* _environmentPage{nullptr};
    Simulation* _simulationPage{nullptr};
    T_Icon* _iconPage{nullptr};
    T_BaseComponents* _baseComponentsPage{nullptr};
    About* _aboutPage{nullptr};
    Setting* _settingPage{nullptr};

    QString _elaDxgiKey;
    QString _viewKey;
    QString _aboutKey;
};
