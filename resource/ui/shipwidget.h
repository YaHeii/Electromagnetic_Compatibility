#ifndef SHIPWIDGET_H
#define SHIPWIDGET_H

#include <QWidget>
#include "DeviceWidget.h"
#include "deviceonship.h"
#include "../include/models/DataModel.h"
// 确保可以从构建目录找到生成的UI头文件
#ifdef IN_IDE
#   include <QObject>
#else
#   include "ui_shipwidget.h"
#endif

class ShipWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ShipWidget(QWidget *parent = nullptr);
    ~ShipWidget();
    void setData(const ShipData &data);
    void updateShipModelData();

private:
    Ui::shipWidget *ui;
    QString m_currentId;
    void syncDeviceListWithModel();
    int m_currentShipId;
    // QList<DeviceWidget*> m_deviceList;
    // QList<DeviceonShip*> m_deviceonShipList;
private slots:
    void on_shipEquipmentPlus_clicked();
};


#endif // SHIPWIDGET_H