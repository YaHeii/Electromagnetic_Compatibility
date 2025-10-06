//
// Created by lenovo on 25-10-6.
//

#ifndef DATAMODEL_H
#define DATAMODEL_H
#include <QString>
#include <QList>
#include <QObject>

struct DeviceData {
    QString equipmentID;
    QString equipmentType;
    double Gain;
    QString antennaType;
    double X_offset;
    double Y_offset;
    QString filterType;

    QString singelAntennaType;
    double antennaTheta;
    double antennaPhi;
    double pattern;

    double transmitterPower;
    double transmitterBandwidth;
    double WIP;

    double recieverSensitive;
    double recieverBandwidth;
    QString reciever_TransmiterID;
    double noiseFigure;
    double SNRMargin;
    double interferenceMargin;
};

struct DeviceOnShipConfig {
    QString deviceID; // 引用一个已存在的设备
    // ... 其他与舰船相关的配置，如安装位置等 ...
};

struct ShipData {
    int shipID; // 唯一的舰船ID
    QString shipName;
    double ship_X;
    double ship_Y;
    double ship_Orienteation;
    double ship_Speed;
    QList<DeviceOnShipConfig> configuredDevices;
};
class DataModel : public QObject
{
    Q_OBJECT
public:
    static DataModel* instance(); // 单例模式，方便全局访问

    QList<DeviceData> allDevices;
    QList<ShipData> allShips;

private:
    DataModel(QObject *parent = nullptr);
};



#endif //DATAMODEL_H
