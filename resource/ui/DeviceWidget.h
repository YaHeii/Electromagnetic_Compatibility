#ifndef DEVICEWIDGET_H
#define DEVICEWIDGET_H

#include <QWidget>
#include "../include/models/DataModel.h"
#ifdef IN_IDE
#   include <QObject>
#else
#   include "ui_devicewidget.h"
#endif



class DeviceWidget: public QWidget {
    Q_OBJECT
public:
    explicit DeviceWidget(QWidget *parent =  nullptr);
    ~DeviceWidget();
    // 将数据从模型加载到UI界面
    void setData(const EquipmentData &data);
    
    // 将UI界面的数据保存回模型
    void updateModelData();
private:
    Ui::DeviceWidget *ui;
    QString m_currentId;

    // 重置/清空各模块的输入框
    void resetTransmitterUI();
    void resetReceiverUI();
private slots:
    void on_equipmentReduction_clicked();
    void onEquipmentTypeChanged();
};

#endif //DEVICEWIDGET_H
