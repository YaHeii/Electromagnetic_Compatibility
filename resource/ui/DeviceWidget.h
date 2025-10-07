//
// Created by lenovo on 25-10-6.
//

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
    void updateModelData();
    void setData(const DeviceData &data);
    // void updateModelData();
private:
    Ui::DeviceWidget *ui;
    QString m_currentId;
private slots:
    void on_equipmentReduction_clicked();
};

#endif //DEVICEWIDGET_H
