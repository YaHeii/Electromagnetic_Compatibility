#ifndef DATAMODEL_H
#define DATAMODEL_H
#include <QString>
#include <QList>
#include <QObject>
//// 设备数据模型
struct DeviceData {
    // 基本参数
    QString equipmentID;
    QString equipmentType;
    double Gain;
    //相对坐标
    double X_offset;
    double Y_offset;
    double Z_offset;

    // 接收机参数
    // 接收机中心频率
    double CentralF_Reciever;
    // 接收机带宽
    double Bandwidth_Reciever;
    // 接收机灵敏度
    double Sensitive_reciever;
    // 接收机干扰阈值
    double interferenceMargin;
    // 信噪比阈值
    double SINRMargin;
    // 噪声系数
    double noiseFigure;

    // 发射机参数
    // 发射机中心频率
    double CentralF_Transmitter;
    // 发射机带宽
    double Bandwidth_Transmitter;
    // 发射机功率
    double Power_Transmitter;
    // 天线指向角
    double antennaPhi_Transmitter;
    // 发射机波束宽度
    double Beamwidth_Transmitter;
    // 发射机极化方式
    QString PolarizationMethod_Transmitter;
    // 天线类型
    QString antennaType_Transmitter;

    // 天线参数
    // 天线中心频率
    double CentralF_Antenna;
    // 天线带宽
    double Bandwidth_Antenna;
    // 天线功率
    double Power_Antenna;
    // 天线指向角
    double antennaPhi_Antenna;
    // 天线波束宽度
    double Beamwidth_Antenna;
    // 天线极化方式
    QString PolarizationMethod_Antenna;
    // 天线类型
    QString antennaType_Antenna;
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

    std::vector<DeviceData> allDevices;
    std::vector<ShipData> allShips;

private:
    DataModel(QObject *parent = nullptr);
};



#endif //DATAMODEL_H