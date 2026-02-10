#pragma once

#include <QMainWindow>
#include <QObject>
#include <QRect>
#include <iostream>
#include <memory>
#include <future>   
#include <thread>   
#include "ElaWindow.h"
#include "Utils/QtSpdlogSink.h" 

namespace Ui {
    class MainWindow;
}

class LogWidget; 
class ElaContentDialog;

class Home;
class T_Icon;
class T_ElaScreen;
class T_BaseComponents;
class T_Graphics;
class T_Navigation;
class T_Popup;
class T_Card;
class T_ListView;
class T_TableView;
class TreeView;
class About;
class T_Setting;

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

// REVIEW：参考页面
#ifdef Q_OS_WIN
    T_ElaScreen* _elaScreenPage{ nullptr };
#endif
    Home* _homePage{ nullptr };
    T_Icon* _iconPage{nullptr};
    T_BaseComponents* _baseComponentsPage{nullptr};
    T_Graphics* _graphicsPage{nullptr};
    T_Navigation* _navigationPage{nullptr};
    T_Popup* _popupPage{nullptr};
    T_Card* _cardPage{nullptr};
    T_ListView* _listViewPage{nullptr};
    T_TableView* _tableViewPage{nullptr};
    TreeView* _treeViewPage{nullptr};
    About* _aboutPage{nullptr};
    T_Setting* _settingPage{nullptr};
    QString _elaDxgiKey{""};
    QString _viewKey{""};
    QString _aboutKey{""};
};

