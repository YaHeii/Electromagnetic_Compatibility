#ifndef SHIPWIDGET_H
#define SHIPWIDGET_H
#include "spdlog/spdlog.h"
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include "DeviceWidget.h"
#include "deviceonship.h"
#include "../include/models/DataModel.h"
#include "ElaPushButton.h"
#include "ElaLineEdit.h"
#include "ElaText.h"

class ShipWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ShipWidget(QWidget *parent = nullptr);
    ~ShipWidget();
    void setData(const ShipData &data);
    void updateShipModelData();

private:
    void setupUI();
    QString m_currentId;
    void syncDeviceListWithModel();
    int m_currentShipId;
    
    // UI Components
    QVBoxLayout* mainLayout;
    QWidget* coordinatesWidget;
    QHBoxLayout* coordinatesLayout;
    
    // Left side - Coordinates
    QHBoxLayout* leftCoordinatesLayout;
    QVBoxLayout* coordinateFieldsLayout;
    QHBoxLayout* xLayout;
    ElaText* xLabel;
    ElaLineEdit* X_offset;
    QHBoxLayout* yLayout;
    ElaText* yLabel;
    ElaLineEdit* Y_offset;
    QHBoxLayout* zLayout;
    ElaText* zLabel;
    ElaLineEdit* Z_offset;
    
    // Right side - Ship properties
    QVBoxLayout* shipPropertiesLayout;
    QHBoxLayout* speedLayout;
    ElaText* speedLabel;
    ElaLineEdit* ship_Speed;
    QHBoxLayout* orientationLayout;
    ElaText* orientationLabel;
    ElaLineEdit* ship_Orienteation;
    
    // Device management
    QVBoxLayout* deviceManagementLayout;
    QScrollArea* scrollArea;
    QWidget* scrollAreaWidgetContents;
    QVBoxLayout* scrollAreaContentsLayout;
    QVBoxLayout* DeviceonShipLayout;
    ElaPushButton* shipEquipmentPlus;
    ElaPushButton* deleteShip;

private slots:
    void on_shipEquipmentPlus_clicked();
    void on_deleteShip_clicked();
    void onDeviceOnShipRemovalRequested();
};


#endif // SHIPWIDGET_H