#pragma once
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QComboBox>
#include <QLineEdit>
#include "../../Interface/DataModel.h"

// 前向声明自定义控件
class ElaPushButton;
class ElaComboBox;
class ElaLineEdit;
class ElaScrollPageArea;
class ElaText;

class DeviceWidget: public QWidget {
    Q_OBJECT
signals:
    void removalRequested(const QString &id);
public:
    explicit DeviceWidget(QWidget *parent =  nullptr);
    ~DeviceWidget();
    // 将数据从模型加载到UI界面
    void setData(const EquipmentData &data);
    
    // 将UI界面的数据保存回模型
    void updateModelData();
    QString getID() const { return m_currentId; }
private:
    // UI控件成员
    ElaScrollPageArea *BaseWidget;
    ElaScrollPageArea *RecieverWidget;
    ElaScrollPageArea *TransmitterWidget;
    
    // 基础信息控件
    ElaComboBox *equipmentType;
    ElaLineEdit *Gain;
    ElaLineEdit *equipmentID;
    ElaLineEdit *X_offset;
    ElaLineEdit *Y_offset;
    ElaLineEdit *Z_offset;
    
    // 接收机控件
    ElaLineEdit *CentralF_Reciever;
    ElaLineEdit *Bandwidth_Reciever;
    ElaLineEdit *Sensitive_reciever;
    ElaLineEdit *interferenceMargin;
    ElaLineEdit *SINRMargin;
    ElaLineEdit *noiseFigure;
    
    // 发射机控件
    ElaLineEdit *CentralF_Transmitter;
    ElaLineEdit *Bandwidth_Transmitter;
    ElaLineEdit *Power_Transmitter;
    ElaLineEdit *antennaPhi_Transmitter;
    ElaLineEdit *Beamwidth_Transmitter;
    ElaComboBox *PolarizationMethod_Transmitter;
    ElaComboBox *antennaType_Transmitter;
    
    // 操作控件
    ElaPushButton *equipmentReduction;
    
    QString m_currentId;

    // UI设置方法
    void setupUI();
    void setupBaseWidget();
    void setupReceiverWidget();
    void setupTransmitterWidget();
    
    // 重置/清空各模块的输入框
    void resetTransmitterUI();
    void resetReceiverUI();
private slots:
    void on_equipmentReduction_clicked();
    void onEquipmentTypeChanged();
};

