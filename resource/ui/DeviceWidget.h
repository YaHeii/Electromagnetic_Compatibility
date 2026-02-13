#pragma once
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QComboBox>
#include <QLineEdit>
#include "Interface/DataModel.h"
#include "BasePage.h"
#include "ElaScrollPageArea.h"

// 前向声明自定义控件
class ElaPushButton;
class ElaComboBox;
class ElaLineEdit;
class ElaScrollArea;
class ElaText;
class DeviceItemWidget;
class ElaScrollPageArea;
class DeviceWidget: public BasePage {
    Q_OBJECT
signals:
    void removalRequested(const QString &id);
public:
    explicit DeviceWidget(QWidget *parent =  nullptr);
    ~DeviceWidget();
private:
    QVBoxLayout* _deviceListLayout;      
    //ElaScrollArea* _mainScrollArea;

    ElaPushButton* AddDeviceBtn;
    ElaPushButton* SaveEquipmentBtn;
private slots:
    void on_AddDeviceBtn_clicked();      
    void on_SaveEquipmentBtn_clicked();    
    void on_RemoveItemRequested(DeviceItemWidget* item);
};

class DeviceItemWidget : public ElaScrollPageArea {
    Q_OBJECT
public:
    explicit DeviceItemWidget(QWidget *parent = nullptr);
    
    // 数据同步接口
    void setData(const EquipmentData &data);
    EquipmentData getData() const;
    QString getID() const { return _currentId; }

signals:
    void deleteMe(DeviceItemWidget* widget); // 告知父容器删除本条目

private slots:
    void onEquipmentTypeChanged();
    void on_ReductionBtn_clicked();

private:
    void setupReceiverUI(ElaScrollPageArea* container);
    void setupTransmitterUI(ElaScrollPageArea* container);
    void resetTransmitterUI();
    void resetReceiverUI();

    QString _currentId;
    
    // --- UI 控件 ---
    ElaComboBox *_equipmentType;
    ElaLineEdit *_gain, *_equipmentID;
    ElaLineEdit *_X_offset, *_Y_offset, *_Z_offset;
    
    ElaScrollPageArea* _RecieverWidget;
    ElaScrollPageArea* _TransmitterWidget;
    
    // 接收机特有
    ElaLineEdit *_CentralF_Receiver, *_Bandwidth_Receiver, *_Sensitive_Receiver;
    ElaLineEdit *_InterferenceMargin_Receiver, *_SINRMargin_Receiver, *_NoiseFigure_Receiver;
    
    // 发射机特有
    ElaLineEdit *_CentralF_Transmitter, *_Bandwidth_Transmitter, *_Power_Transmitter;
    ElaLineEdit *_antennaPhi_Transmitter, *_Beamwidth_Transmitter;
    ElaComboBox *_PolarizationMethod_Transmitter, *_antennaType_Transmitter;

    ElaPushButton* ReductionEquipmentBtn;
};