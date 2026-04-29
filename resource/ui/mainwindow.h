#pragma once

#include <QMainWindow>
#include <QObject>
#include <QRect>
#include "ElaWindow.h"
#include "shipwidget.h"
#include "DeviceWidget.h"
#include <spdlog/spdlog.h>

namespace Ui {
    class MainWindow;
}

class LogWidget; 
class ElaContentDialog;

class Home;
class T_Icon;
class T_BaseComponents;
class T_Navigation;
class T_Popup;
class T_Card;
class T_ListView;
class T_TableView;
class TreeView;
class About;
class Setting;

class MainWindow : public ElaWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    std::shared_ptr<spdlog::sinks::sink> createGuiLogSink();

    void initWindow();
    void initEdgeLayout();
    void initContent();
// REVIEW: 是否保留虚函数
protected:
    virtual void mouseReleaseEvent(QMouseEvent* event);

private:
    Ui::MainWindow *ui;
    LogWidget* _logWidget;
    ElaContentDialog* _closeDialog{ nullptr };
    ElaSuggestBox* _windowSuggestBox{ nullptr };
    QString _settingKey{ "" };

    Home* _homePage{ nullptr };
    ShipWidget* _shipPage{ nullptr };
    DeviceWidget* _devicePage { nullptr };
    T_Icon* _iconPage{nullptr};
    T_BaseComponents* _baseComponentsPage{nullptr};
    About* _aboutPage{nullptr};
    Setting* _settingPage{nullptr};
    
    QString _elaDxgiKey{""};
    QString _viewKey{""};
    QString _aboutKey{""};
};

