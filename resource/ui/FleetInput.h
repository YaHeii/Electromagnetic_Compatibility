#pragma once

#include <QWidget>
#include <vector>
#include "spdlog/spdlog.h"
#include "models/DataModel.h"


QT_BEGIN_NAMESPACE
namespace Ui { class FleetInput; }
QT_END_NAMESPACE

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
	bool updateDeviceModelFromView();
	bool updateShipModelFromView();
	
	TreeViewManager* _treeView;
	Ui::FleetInput* ui;

public slots:
	void on_addShipButton_clicked();
	void on_addDeviceButton_clicked();
	void on_DeviceSave_clicked();
	void on_ShipSave_clicked();
	void onDeviceWidgetRemovalRequested(const QString& id);
};
