#pragma once

#include <QWidget>
#include <qevent.h>
#include "Interface/DataModel.h"
class ElaComboBox;
class DeviceonShip : public QWidget {
    Q_OBJECT

public:
    explicit DeviceonShip(QWidget *parent = nullptr);
    ~DeviceonShip() override;

    // 设置并获取挂载在船上的设备引用数据（修复编译错误）
    // 这里使用 DataModel 中定义的 EquipmentOnShip 类型
    void setData(const EquipmentOnShip& data);
    EquipmentOnShip getData() const;
    void refreshEquipmentList();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

signals:
    void removalRequested();
        
private slots:
    // 简单实现：当删除按钮被点击时，通知外部移除即可
    void on_deleteDeviceonShip_clicked();
    
private:
    EquipmentOnShip _data;
    ElaComboBox* _EquipmentIDCombo;
};
