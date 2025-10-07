//
// Created by lenovo on 25-10-6.
//

#ifndef DATAMODEL_H
#define DATAMODEL_H
#include <QString>
#include <QList>
#include <QObject>
//// 设备数据模型
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

    // 发射机参数
    double transmitterPower;
    double transmitterBandwidth;
    double transmitterFrequency; // 添加发射机频率字段
    double WIP;

    // 接收机参数
    double recieverSensitive;
    double recieverBandwidth;
    double recieverFrequency; // 添加接收机频率字段
    QString reciever_TransmiterID;
    double noiseFigure;
    double SNRMargin;
    double interferenceMargin;
};
//// 船上设备数据模型
struct DeviceOnShipConfig {
    QString deviceID;
    double device_X_offset;
    double device_Y_offset;
};
//// 舰船模型
struct ShipData {
    int shipID;
    QString shipName;
    double ship_X;
    double ship_Y;
    double ship_Orienteation;
    double ship_Speed;
    QList<DeviceOnShipConfig> configuredDevices;
};
//// 数据模型
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