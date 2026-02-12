#pragma once
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QComboBox>
#include <QLineEdit>
#include "Interface/DataModel.h"

// 前向声明自定义控件
class ElaPushButton;
class ElaComboBox;
class ElaLineEdit;
class ElaScrollPageArea;
class ElaText;

class DeviceWidget: public BasePage {
    Q_OBJECT
signals:
    void removalRequested(const QString &id);
public:
    explicit DeviceWidget(QWidget *parent =  nullptr);
    ~DeviceWidget();
    void setData(const EquipmentData &data);
    
    // 将UI界面的数据保存回模型
    void updateModelData();
    QString getID() const { return _currentId; }
private:
    // UI控件成员
    ElaScrollPageArea* _baseWidget;
    ElaScrollPageArea* _RecieverWidget;
    ElaScrollPageArea *TransmitterWidget;
    
    // 基础信息控件
    ElaComboBox *_equipmentType;
    ElaLineEdit *_gain;
    ElaLineEdit *_equipmentID;
    ElaLineEdit *_X_offset;
    ElaLineEdit *_Y_offset;
    ElaLineEdit *_Z_offset;
    
    // 接收机控件
    ElaLineEdit *_CentralF_Reciever;
    ElaLineEdit *_Bandwidth_Reciever;
    ElaLineEdit *_Sensitive_reciever;
    ElaLineEdit *_interferenceMargin;
    ElaLineEdit *_SINRMargin;
    ElaLineEdit *_NoiseFigure;
    
    // 发射机控件
    ElaLineEdit *_CentralF_Transmitter;
    ElaLineEdit *_Bandwidth_Transmitter;
    ElaLineEdit *_Power_Transmitter;
    ElaLineEdit *_antennaPhi_Transmitter;
    ElaLineEdit *_Beamwidth_Transmitter;
    ElaComboBox *_PolarizationMethod_Transmitter;
    ElaComboBox *_antennaType_Transmitter;
    
    // 操作控件
    ElaPushButton *_equipmentReduction;
    
    QString _currentId;

    // UI设置方法
    void setupReceiverWidget();
    void setupTransmitterWidget();
    
    // 重置/清空各模块的输入框
    void resetTransmitterUI();
    void resetReceiverUI();
private slots:
    void on_equipmentReduction_clicked();
    void onEquipmentTypeChanged();
    void on_DeviceSave_clicked();

    void updateDeviceModelFromView();
    void onDeviceWidgetRemovalRequested(const QString& id);
    void on_addDeviceButton_clicked();
};

