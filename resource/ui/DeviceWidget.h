#pragma once

#include <QComboBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QWidget>

#include "BasePage.h"
#include "Interface/DataModel.h"

class ElaComboBox;
class ElaLineEdit;
class ElaPushButton;
class ElaScrollArea;
class ElaScrollPageArea;
class ElaText;

class DeviceItemWidget;

class DeviceWidget : public BasePage {
    Q_OBJECT

public:
    explicit DeviceWidget(QWidget* parent = nullptr);
    ~DeviceWidget() override;

    void loadFromModel();
    bool saveToModel(QString* errorMessage = nullptr);
    void setReadOnly(bool readOnly);
    bool isDirty() const { return _isDirty; }

signals:
    void dirtyStateChanged(bool isDirty);
    void modelCommitted();
    void equipmentsCommitted();

private slots:
    void on_AddDeviceBtn_clicked();
    void on_SaveEquipmentBtn_clicked();
    void on_RemoveItemRequested(DeviceItemWidget* item);
    void on_ItemEdited();

private:
    void setDirty(bool dirty);
    void clearItems();

    QVBoxLayout* _deviceListLayout{nullptr};
    QHBoxLayout* _btnLayout{nullptr};
    ElaPushButton* AddDeviceBtn{nullptr};
    ElaPushButton* SaveEquipmentBtn{nullptr};
    bool _isDirty{false};
    bool _isLoading{false};
    bool _isReadOnly{false};
};

class DeviceItemWidget : public QWidget {
    Q_OBJECT

public:
    explicit DeviceItemWidget(QWidget* parent = nullptr);

    void setData(const EquipmentData& data);
    EquipmentData getData() const;
    bool tryBuildData(EquipmentData& data, QString& errorMessage) const;
    QString getID() const { return _currentId; }
    void setReadOnly(bool readOnly);

signals:
    void deleteMe(DeviceItemWidget* widget);
    void dataEdited();

private slots:
    void onEquipmentTypeChanged();
    void on_ReductionBtn_clicked();

private:
    void setupReceiverUI(QWidget* container);
    void setupTransmitterUI(QWidget* container);
    void resetTransmitterUI();
    void resetReceiverUI();

    QString _currentId;
    ElaComboBox* _equipmentType{nullptr};
    ElaLineEdit* _gain{nullptr};
    ElaLineEdit* _equipmentID{nullptr};
    ElaLineEdit* _X_offset{nullptr};
    ElaLineEdit* _Y_offset{nullptr};
    ElaLineEdit* _Z_offset{nullptr};
    QWidget* _RecieverWidget{nullptr};
    QWidget* _TransmitterWidget{nullptr};
    ElaLineEdit* _CentralF_Receiver{nullptr};
    ElaLineEdit* _Bandwidth_Receiver{nullptr};
    ElaLineEdit* _Sensitive_Receiver{nullptr};
    ElaLineEdit* _InterferenceMargin_Receiver{nullptr};
    ElaLineEdit* _SINRMargin_Receiver{nullptr};
    ElaLineEdit* _NoiseFigure_Receiver{nullptr};
    ElaLineEdit* _CentralF_Transmitter{nullptr};
    ElaLineEdit* _Bandwidth_Transmitter{nullptr};
    ElaLineEdit* _Power_Transmitter{nullptr};
    ElaLineEdit* _antennaPhi_Transmitter{nullptr};
    ElaLineEdit* _Beamwidth_Transmitter{nullptr};
    ElaComboBox* _PolarizationMethod_Transmitter{nullptr};
    ElaComboBox* _antennaType_Transmitter{nullptr};
    ElaPushButton* ReductionEquipmentBtn{nullptr};
};
