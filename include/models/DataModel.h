#ifndef DATAMODEL_H
#define DATAMODEL_H
#include <QString>
#include <QList>
#include <QObject>
#include <vector> 
#include <cmath>  

const int X_MIN = -50;
const int X_MAX = 50;
const int Y_MIN = -50;
const int Y_MAX = 50;
const int Z_MIN = 0;
const int Z_MAX = 50;


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
    //TODO:设计场景：队内通信、岸基控制、应急通信、定位接受
    double Sensitive_reciever = -100.0; // dBm (默认给一个较低的灵敏度)
    // TODO: 根据接收机类型
    // - UHF/VHF接收机：    -20dBm 到 0dBm
    // - GNSS接收机：      -30dBm 到 -15dBm  
    // - 4G/5G接收机：      -25dBm 到 -10dBm
    // - WiFi接收机：      -20dBm 到 0dBm
    double interferenceMargin = 0.0;    // dB
    //DEPRECATED：目前不考虑通信性能
    double SINRMargin = 0.0;            // dB
    // TODO:根据波段
    //MINNF typNF MAXNF
    // {"VHF (30-300MHz)",  0.8, 1.5, 3.0},
    // {"UHF (300-1000MHz)", 1.0, 2.0, 4.0},
    // {"L波段 (1-2GHz)",    1.2, 2.5, 5.0},
    // {"S波段 (2-4GHz)",    1.5, 3.0, 6.0},
    // {"C波段 (4-8GHz)",    2.0, 4.0, 8.0}
    double noiseFigure = 3.0;           // dB (典型噪声系数)

    // --- 发射机参数 ---
    double CentralF_Transmitter = 0.0;   // MHz
    double Bandwidth_Transmitter = 0.0;  // MHz
    //REVIEW: 发射机是否要保留增益参数
    double Power_Transmitter = 0.0;      // dBm
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

    std::pair<bool,QString> validate_EquipmentBaseInfo() const {
        if (equipmentID.isEmpty()) return { false, "设备ID不能为空" };
        if(equipmentID.toStdString().find('_')){
            return {false, "设备名称不应含有‘_’"};
        }
        if(Gain < 0){
            return {false, "不支持增益为负"};
        }

        auto inRange = [](int val, int min, int max) { return val >= min && val <= max; };

        if (!inRange(X_offset, X_MIN, X_MAX) ||
            !inRange(Y_offset, Y_MIN, Y_MAX) ||
            !inRange(Z_offset, Z_MIN, Z_MAX)) {
            return {false, "超出地图范围,50<x<50,50<y<50,0<z<50"};
        }
        return {true, ""};
    }

    std::pair<bool, QString> validate_reciever() const {
        auto baseResult = validate_EquipmentBaseInfo();
        if (!baseResult.first) return baseResult;
        if (CentralF_Reciever <= 0) return { false, "接收机中心频率必须大于 0" };
        if (Bandwidth_Reciever <= 0) return { false, "接收机带宽必须大于 0" };
        if (Sensitive_reciever > -90) return {false, "灵敏度不足,编队内通信最低灵敏度为-90dBm"};
        if (interferenceMargin >= 0) return {false, "干扰阈值应小于0dBm"};
        if (noiseFigure <= 0.8) return {false, "噪声系数不应小于0.8"};
        return {true, ""};
    }

    std::pair<bool, QString> valiate_Transmitter() const {
        auto baseResult = validate_EquipmentBaseInfo();
        if (!baseResult.first) return baseResult;
        if (CentralF_Transmitter <= 0) return { false, "发射机中心频率必须大于 0" };
        if (Bandwidth_Transmitter <= 0) return { false, "接收机带宽必须大于 0" };
        if (Power_Transmitter < 0) return {false, "发射机增益不应小于0"};
        if (antennaPhi_Transmitter < 0 || antennaPhi_Transmitter > 180) return {false, "天线仰角必须在 [0, 180] 范围内"};
        if (Beamwidth_Transmitter < 0 || Beamwidth_Transmitter > 360) return {false, "天线波束宽度必须在 [0, 360] 范围内"};
        return {true, ""};
    } 

    std::pair<bool,QString> validate() const{
        if(equipmentType == "接收机") return validate_reciever();
        if(equipmentType == "发射机") return valiate_Transmitter();
        //TODO:收发一体
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
     //DEPRECATED: 没有Type接口
    QString shipType = "驱逐舰";

    // 初始位置与姿态
    // TODO: 修改变量为WorldX
    double X_offset = 0.0; 
    double Y_offset = 0.0;
    double Z_offset = 0.0;

    double ship_Orienteation = 0.0; // 航向角 0-360
    double ship_Speed = 0.0;        // 节 (knots) 或 m/s

    // 船上挂载的设备列表
    std::vector<EquipmentOnShip> Equipments;

    // 校验逻辑
    std::pair<bool, QString> validate_Ship() const {
        if (shipName.isEmpty()) return { false, "船名不能为空" };
        if (ship_Speed < 0) return { false, "航速不能为负数" };

        auto inRange = [](int val, int min, int max) { return val >= min && val <= max; };

        if (!inRange(X_offset, X_MIN, X_MAX) ||
            !inRange(Y_offset, Y_MIN, Y_MAX) ||
            !inRange(Z_offset, Z_MIN, Z_MAX)) {
            return {false, "超出地图范围,50<x<50,50<y<50,0<z<50"};
        }

        if(ship_Orienteation<0 || ship_Orienteation>360){
            return {false, "船向范围应在[0,360]"};
        }
        return { true, "" };
    }
};


/// <summary>
/// 全局数据模型 (单例管理的数据容器)
/// </summary>
class DataModel : public QObject {
    Q_OBJECT
public:
    // 定义一个可拷贝的数据快照结构体
    struct DataSnapshot {
        std::vector<EquipmentData> allEquipments;
        std::vector<ShipData> allShips;
    };

    static DataModel* instance() {
        static DataModel _instance;
        return &_instance;
    }

    // 创建数据快照的成员函数
    DataSnapshot createSnapshot() const {
        // 这里可以加锁（如果需要的话），保证创建快照时的原子性
        // std::lock_guard<std::mutex> lock(m_mutex);
        return { allEquipments, allShips };
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