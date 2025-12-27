#ifndef DATAMODEL_H
#define DATAMODEL_H
#include <QString>
#include <QList>
#include <QObject>
#include <vector> 
#include <cmath>  

/// <summary>
/// 设备数据模型 (DTO)
/// 包含发射机、接收机、天线的物理参数
/// </summary>
struct EquipmentData {
    // --- 基础标识与位置 ---
    QString equipmentID = "";
    QString equipmentType = " "; // 发射机/接收机/收发一体机/天线
    double Gain = 0.0;              // 单位: dBi/dBm

    // 相对坐标 (相对于载体中心)
    double X_offset = 0.0;
    double Y_offset = 0.0;
    double Z_offset = 0.0;

    // --- 接收机参数 ---
    double CentralF_Reciever = 0.0;     // MHz
    double Bandwidth_Reciever = 0.0;    // MHz/KHz (需统一单位，建议 MHz)
    double Sensitive_reciever = -100.0; // dBm (默认给一个较低的灵敏度)
    double interferenceMargin = 0.0;    // dB
    double SINRMargin = 0.0;            // dB
    double noiseFigure = 3.0;           // dB (典型噪声系数)

    // --- 发射机参数 ---
    double CentralF_Transmitter = 0.0;   // MHz
    double Bandwidth_Transmitter = 0.0;  // MHz
    double Power_Transmitter = 0.0;      // dBm/W (建议明确单位，通常UI显示dBm)
    double antennaPhi_Transmitter = 0.0; // 指向角 (Azimuth)
    double Beamwidth_Transmitter = 0.0;  // 波束宽度 (度)
    QString PolarizationMethod_Transmitter = "垂直极化";
    QString antennaType_Transmitter = "全向天线";

    // --- 天线参数 ---
    double CentralF_Antenna = 0.0;
    double Bandwidth_Antenna = 0.0;
    double Power_Antenna = 0.0;
    double antennaPhi_Antenna = 0.0;
    double Beamwidth_Antenna = 0.0;
    QString PolarizationMethod_Antenna = "垂直极化";
    QString antennaType_Antenna = "全向天线";

    // --- 数据合法性校验 ---
    // 返回 pair: first=是否合法, second=错误信息
    std::pair<bool, QString> validate() const {
        // 检查频率 (物理上必须 > 0)
        // 根据 equipmentType 检查对应的频率
        if (equipmentType == "接收机" || equipmentType == "收发一体机") {
            if (CentralF_Reciever <= 0) return { false, "接收机中心频率必须大于 0" };
            if (Bandwidth_Reciever <= 0) return { false, "接收机带宽必须大于 0" };
        }
        
        if (equipmentType == "发射机" || equipmentType == "收发一体机") {
            if (CentralF_Transmitter <= 0) return { false, "发射机中心频率必须大于 0" };
            if (Bandwidth_Transmitter <= 0) return { false, "发射机带宽必须大于 0" };
        }

        if (equipmentType == "天线") {
            // 天线通常是被动的，但作为独立设备时可能有工作频段
             if (CentralF_Antenna <= 0) return { false, "天线中心频率必须大于 0" };
        }

        // 检查角度范围 (0~360 或 -180~180)
        // 这里假设是波束宽度，物理上不能超过 360，且必须 > 0
        if (equipmentType == "天线") {
            if (Beamwidth_Antenna <= 0 || Beamwidth_Antenna > 360) 
                return { false, "天线波束宽度必须在 (0, 360] 范围内" };
        }
        
        // 检查ID
        if (equipmentID.isEmpty()) return { false, "设备ID不能为空" };

        return { true, "" };
    }
};

/// <summary>
/// 船上挂载设备引用
/// </summary>
struct EquipmentOnShip {
    QString equipmentID;
    // 动态开启/关闭状态
    bool isEnabled = true;
};

/// <summary>
/// 舰船数据模型
/// </summary>
struct ShipData {
    int shipID = 0;
    QString shipName = "未命名船只";
    QString shipType = "驱逐舰";

    // 初始位置与姿态
    double X_offset = 0.0; // 这里的 Offset 可能是指在仿真场景中的绝对坐标？建议改名为 Latitude/Longitude 或 WorldX/WorldY
    double Y_offset = 0.0;
    double Z_offset = 0.0;

    double ship_Orienteation = 0.0; // 航向角 0-360
    double ship_Speed = 0.0;        // 节 (knots) 或 m/s

    // 船上挂载的设备列表
    std::vector<EquipmentOnShip> Equipments;

    // 校验逻辑
    std::pair<bool, QString> validate() const {
        if (shipName.isEmpty()) return { false, "船名不能为空" };
        if (ship_Speed < 0) return { false, "航速不能为负数" };
        return { true, "" };
    }
};


/// <summary>
/// 全局数据模型 (单例管理的数据容器)
/// </summary>
class DataModel : public QObject {
    Q_OBJECT
public:
    static DataModel* instance() {
        static DataModel _instance;
        return &_instance;
    }

    // 全局数据存储
    std::vector<EquipmentData> allEquipments; // 设备库
    std::vector<ShipData> allShips;           // 部署的船只

    // 根据ID查找设备的辅助函数
    const EquipmentData* findEquipmentByID(const QString& id) const {
        for (const auto& eq : allEquipments) {
            if (eq.equipmentID == id) return &eq;
        }
        return nullptr;
    }

private:
    DataModel() = default; 
    ~DataModel() = default;
    DataModel(const DataModel&) = delete;
    DataModel& operator=(const DataModel&) = delete;
};

#endif //DATAMODEL_H