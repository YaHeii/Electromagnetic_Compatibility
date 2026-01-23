#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTreeView>
#include <QScrollArea>
#include <QSizePolicy>
#include <vector>
#include "spdlog/spdlog.h"
#include "models/DataModel.h"
#include "ElaPushButton.h"
#include "ElaTabWidget.h"
#include "ElaText.h"

class TreeViewManager;

/// <summary>
/// 用于输入模型参数
/// </summary>
class FleetInput : public QWidget {
	Q_OBJECT

public:
	explicit FleetInput(QWidget* parent = nullptr);
	~FleetInput();

private: 
	void setupUI();
	bool updateDeviceModelFromView();
	bool updateShipModelFromView();
	void connectTreeViewSignals();
	
	TreeViewManager* _treeView;
	
	// UI Components
	QVBoxLayout* mainLayout;
	QHBoxLayout* contentLayout;
	
	// Left side - Tree view
	ElaTabWidget* treeViewManager;
	QWidget* structureTab;
	QVBoxLayout* structureLayout;
	QTreeView* treeView;
	
	// Right side - Input tabs
	QVBoxLayout* rightLayout;
	ElaTabWidget* inputTabWidget;
	
	// Device tab
	QWidget* deviceTab;
	QVBoxLayout* deviceTabLayout;
	QScrollArea* deviceScrollArea;
	QWidget* deviceContentsWidget;
	QVBoxLayout* deviceLayout;
	ElaPushButton* addDeviceButton;
	ElaPushButton* DeviceSave;
	
	// Ship tab
	QWidget* shipTab;
	QVBoxLayout* shipTabLayout;
	QScrollArea* shipScrollArea;
	QWidget* shipsContentsWidget;
	QVBoxLayout* shipsLayout;
	ElaPushButton* addShipButton;
	ElaPushButton* ShipSave;

public slots:
	void on_addShipButton_clicked();
	void on_addDeviceButton_clicked();
	void on_DeviceSave_clicked();
	void on_ShipSave_clicked();
	void onDeviceWidgetRemovalRequested(const QString& id);
};
