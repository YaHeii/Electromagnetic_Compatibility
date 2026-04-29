#pragma once

#include <QHBoxLayout>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>

#include "DeviceWidget.h"
#include "Interface/DataModel.h"

class BasePage;
class ElaLineEdit;
class ElaListView;
class ElaPushButton;
class ElaText;

class ShipItemWidget;

class ShipWidget : public BasePage {
    Q_OBJECT

public:
    explicit ShipWidget(QWidget* parent = nullptr);
    ~ShipWidget() override;

    void loadFromModel();
    bool saveToModel(QString* errorMessage = nullptr);
    void setReadOnly(bool readOnly);
    void refreshEquipmentReferences();
    bool isDirty() const { return _isDirty; }

signals:
    void dirtyStateChanged(bool isDirty);
    void modelCommitted();

private slots:
    void on_AddShipBtn_clicked();
    void on_SaveShipBtn_clicked();
    void on_RemoveShipItemRequested(ShipItemWidget* item);
    void on_ItemEdited();

private:
    void setDirty(bool dirty);
    void clearItems();

    QVBoxLayout* _ShipListLayout{nullptr};
    ShipItemWidget* _shipItemWidget{nullptr};
    std::string _currentShipId;
    ElaPushButton* _AddShipBtn{nullptr};
    ElaPushButton* _SaveShipBtn{nullptr};
    bool _isDirty{false};
    bool _isLoading{false};
    bool _isReadOnly{false};
};

class ShipItemWidget : public QWidget {
    Q_OBJECT

public:
    explicit ShipItemWidget(QWidget* parent = nullptr);

    void setData(const ShipData& data);
    ShipData getData() const;
    bool tryBuildData(ShipData& data, QString& errorMessage) const;
    QString getID() const { return _currentId; }
    void refreshEquipmentOptions();
    void setReadOnly(bool readOnly);

signals:
    void deleteMe(ShipItemWidget* widget);
    void dataEdited();

private slots:
    void on_SelfReductionBtn_clicked();
    void on_AddDeviceOnShipBtn_clicked();

private:
    QString _currentId;
    ElaLineEdit* _X_offset{nullptr};
    ElaLineEdit* _Y_offset{nullptr};
    ElaLineEdit* _Z_offset{nullptr};
    ElaLineEdit* _ship_ID{nullptr};
    ElaLineEdit* _ship_Speed{nullptr};
    ElaLineEdit* _ship_Orienteation{nullptr};
    QVBoxLayout* _deviceOnShipLayout{nullptr};
    ElaPushButton* _AddDeviceOnShipBtn{nullptr};
    QVBoxLayout* _rightPannel{nullptr};
    QVBoxLayout* _leftPannel{nullptr};
    ElaPushButton* _ReductionShipBtn{nullptr};
};
