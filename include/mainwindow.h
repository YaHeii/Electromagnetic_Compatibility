#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

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
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // 槽函数声明保持不变
    void onElementTreeItemClicked(QTreeWidgetItem *item, int column);
    void onActionNewTriggered();
    void onActionOpenTriggered();
    void onActionSaveTriggered();
    void onActionStartAnalysisTriggered();

private:
    // 核心的UI创建函数
    void setupUI();
    
    // 创建菜单和工具栏
    void createActionsAndMenus();
    
    // 填充假数据的函数保持不变
    void populateSceneTree();
    void populateInterferenceMatrix();
    void updateParameterPanel(QTreeWidgetItem *item);

    // --- UI控件指针 ---
    // 左侧
    QTreeWidget* elementTreeWidget;
    QStackedWidget* parameterStackedWidget;

    // 右侧
    QTableWidget* interferenceMatrixTable;

    // 中央
    QGraphicsView* sceneGraphicsView;

    // 底部
    QSlider* timelineSlider;
    QLabel* statusLabel; // 用于替代statusbar的临时标签
};
#endif // MAINWINDOW_H