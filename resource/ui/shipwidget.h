#ifndef SHIPWIDGET_H
#define SHIPWIDGET_H
#include "spdlog/spdlog.h"
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include "DeviceWidget.h"
#include "deviceonship.h"
#include "Interface/DataModel.h"
#include "ElaPushButton.h"
#include "ElaLineEdit.h"
#include "ElaText.h"
#include "BasePage.h"

class ShipWidget : public BasePage
{
    Q_OBJECT

public:
    explicit ShipWidget(QWidget *parent = nullptr);
    ~ShipWidget();
    void setData(const ShipData &data);
    void updateShipModelData();

private:
   
    void syncDeviceListWithModel();
    std::string _currentShipId;
    
    ElaLineEdit* _X_offset;
    ElaLineEdit* _Y_offset;
    ElaLineEdit* _Z_offset;

    ElaLineEdit* _ship_ID;
    ElaLineEdit* _ship_Speed;
    ElaLineEdit* _ship_Orienteation;
    
    QVBoxLayout* _rightPannel;
    QVBoxLayout* _leftPannel;
private slots:
    void on_shipEquipmentPlus_clicked();
    void on_deleteShip_clicked();
    void onDeviceOnShipRemovalRequested();
    void on_addShipButton_clicked();
    void on_ShipSave_clicked();
    bool updateShipModelFromView();
};


#endif // SHIPWIDGET_H