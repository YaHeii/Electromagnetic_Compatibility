#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include "../include/utils/data_get.h"
#include "../include/core/ship.h"
#include "../include/core/equipment.h" // Antenna.h is included by Equipment.h
#include "../include/utils/point_2D.h"
#include "../include/models/shortlist.h"
#include "../include/utils/data_get.h"
#include "../include/utils/conversions.h"
#include "../include/models/PropagationModle.h"
#include "../include/core/fleet.h"
#include "../include/core/EMC_Engine.h"
#include "../include/models/Path.h"
#include "../include/models/move.h"

using namespace Electromagnetic_compatibility::core;
using namespace Electromagnetic_compatibility::models;
using namespace Electromagnetic_compatibility::utils;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE
// 前向声明所有需要的Qt类，可以减少头文件依赖
class QTreeWidget;
class QTableWidget;
class QStackedWidget;
class QGraphicsView;
class QSlider;
class QLabel;
class QTreeWidgetItem;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    // 析构函数
    ~MainWindow();
    void init_Data();
private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H