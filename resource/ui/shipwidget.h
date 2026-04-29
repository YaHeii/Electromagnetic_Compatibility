#pragma once
#include "spdlog/spdlog.h"
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include "DeviceWidget.h"
#include "deviceonship.h"
#include "Interface/DataModel.h"
class BasePage;
class ElaLineEdit;
class ElaText;
class ElaPushButton;
class ElaListView;
class ShipItemWidget;

class ShipWidget : public BasePage
{
    Q_OBJECT

public:
    explicit ShipWidget(QWidget *parent = nullptr);
    ~ShipWidget();

private:
    QVBoxLayout* _ShipListLayout;

    ShipItemWidget* _shipItemWidget;
    std::string _currentShipId;
    
    ElaPushButton* _AddShipBtn;
    ElaPushButton* _SaveShipBtn;
private slots:
    void on_AddShipBtn_clicked();
    void on_SaveShipBtn_clicked();
    // bool updateShipModelFromView();
    void on_RemoveShipItemRequested(ShipItemWidget* item);
};

class ShipItemWidget : public QWidget {
    Q_OBJECT
public:
    explicit ShipItemWidget(QWidget *parent = nullptr);
    void setData(const ShipData &data);
    ShipData getData() const;
    bool tryBuildData(ShipData& data, QString& errorMessage) const;
    QString getID() const { return _currentId; }
signals:
    void deleteMe(ShipItemWidget* widget); // 告知父容器删除本条目
private slots:
    void on_SelfReductionBtn_clicked();
    void on_AddDeviceOnShipBtn_clicked();
private:    
    QString _currentId;
    ElaLineEdit* _X_offset;
    ElaLineEdit* _Y_offset;
    ElaLineEdit* _Z_offset;

    ElaLineEdit* _ship_ID;
    ElaLineEdit* _ship_Speed;
    ElaLineEdit* _ship_Orienteation;

    QVBoxLayout* _deviceOnShipLayout;
    ElaPushButton* _AddDeviceOnShipBtn;

    QVBoxLayout* _rightPannel;
    QVBoxLayout* _leftPannel;
    ElaPushButton* _ReductionShipBtn;
};
