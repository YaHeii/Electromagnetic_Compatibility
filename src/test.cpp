#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include "../include/utils/data_get.h"
#include "../include/core/ship.h"
#include "../include/core/equipment.h" // Antenna.h is included by Equipment.h
#include "../include/utils/point_2D.h"
#include "../include/models/shortlist.h"
#include "../include/utils/data_get.h"
#include "../include/utils/conversions.h"
#include "../include/models/PropagationModle.h"
#include "../include/core/fleet.h"
#include "../include/core/EMC_Engine.h"
#include "../include/models/Path.h"
#include "../include/models/move.h"

using namespace std;
using namespace Electromagnetic_compatibility::core;
using namespace Electromagnetic_compatibility::models;
using namespace Electromagnetic_compatibility::utils;
int main(){
    //----------------------------------------------------第二章---------------------------------------------------------

    //----------------------------------------------------创建船，设备----------------------------------------------------
    //设定船数量最大值
    int total_ship_num = 5;//后面要对这个值进行输入
    // 创建编队
    Electromagnetic_compatibility::core::Fleet ship_fleet;
    //读取文件
    unordered_map<string,pair<int,int>> data = Electromagnetic_compatibility::utils::read_file("D:\\code\\C++\\Electromagnetic_compatibility\\src\\input.txt",3);
    //生成路径队列
    vector<Electromagnetic_compatibility::models::Path> Path_list;
    //生成全部的船和设备等，并绑定
    for (int i = 0;i<total_ship_num;i++) {
        double x = data["ship"+to_string(i)].first;
        double y = data["ship"+to_string(i)].second;
        auto myShip = std::make_unique<ship>("ship"+to_string(i),Point2D{x,y});
        // 创建一个全向天线
        auto omniAnt = std::make_unique<OmniAntenna>("Ant1_Omni", 2.0); // 2 dBi gain
        // 创建一个设备并安装天线
        auto equip0 = std::make_unique<Equipment>("Equip0_Generic", EquipmentType::GENERIC, Point2D{0.5, 0}); // 船上相对位置 (0.5, 0)
        // std::cout << "Equipment " << equip0->getID()
        //   << " has antenna: " << equip0->getAntenna()->getID()
        //   << " with gain: " << equip0->getAntenna()->getGainDbi(0) << " dBi" << std::endl;
        equip0->setAntenna(std::move(omniAnt)); // 把天线给设备
        // 把设备添加到船上
        myShip->addEquipment(std::move(equip0));
        // std::cout << Ship->getID() << " has " << Ship->getEquipmentList().size() << " equipment." << endl;
        // std::cout << "First equipment ID: " << Ship->getEquipmentList()[0]->getID() << endl;

        // 创建一个发射机
        auto tx1 = std::make_unique<Transmitter>("CommsTX1", 433.0, 20.0, 25.0, Point2D{0.1, 0});
        tx1->setAntenna(std::make_unique<OmniAntenna>("Ant_TX1", 3.0)); // 3 dBi gain
        // std::cout << "TX1: " << tx1->getID() << " Freq: " << tx1->getFrequencyMHz() << " MHz, Power: " << tx1->getPowerDBm() << " dBm"
        //           << " Antenna Gain: " << tx1->getAntenna()->getGainDbi(0) << " dBi" << std::endl;
        myShip->addEquipment(std::move(tx1));

        // 创建一个接收机
        auto rx1 = std::make_unique<Receiver>("NavRX1", 1575.42, -120.0, 2000.0, "CommsTX1", "ship"+to_string(i), 2.5, 10.0, 10.0, Point2D{-0.1, 0});

        /*
        Receiver(const std::string& id,
             double frequency_mhz,//频率
             double sensitivity_dbm, // 接收机灵敏度
             double bandwidth_khz,//带宽
             std::string transmitter_id,//表示发送设备id
             std::string transmitter_in_ship_id,//表示设备所在船ID
             double noise_figure_db = 3.0, // 噪声系数 (dB)
             double SINR_threshold_db = 10.0, // 信噪比阈值 (dB)
             double interference_threshold_db = 10.0, // 干扰阈值 (dB)
             const utils::Point2D& relative_pos = {0.0,0.0})
        
        */
        // GPS L1, 2MHz BW, 2.5dB NF
        rx1->setAntenna(std::make_unique<OmniAntenna>("Ant_RX1", 1.0)); // 1 dBi gain
        // std::cout << "RX1: " << rx1->getID() << " Freq: " << rx1->getFrequencyMHz() << " MHz, Sensitivity: " << rx1->getSensitivityDBm() << " dBm"
        //           << " Noise Floor: " << rx1->getNoiseFloorDBm() << " dBm" << std::endl;
        myShip->addEquipment(std::move(rx1));

        // std::cout << ship->getID() << " now has " << ship->getEquipmentList().size() << " equipment items." << std::endl;
        // for(const auto& eq_ptr : ship->getEquipmentList()){
        //     std::cout << " - Equip ID: " << eq_ptr->getID() << ", Type: " << static_cast<int>(eq_ptr->getType()) << std::endl;
        // }
        ship_fleet.addShip(std::move(myShip));
    }
    //----------------------------------------------创建编队内船的移动路径---------------------------------------------------
    //对路径进行时间采样，通过每个多个采样点来综合判断系统效能
    Path ship0_path(0,10,&ship_fleet);
    int t = 60;//输入总计算时间，单位s
    //cin>>t;//输入时间，在整段时间内完成效能评估,单位s
    int t_step_num = 60;//采样点数
    int t_step = t/t_step_num;//时间步长，单位s

    vector<Electromagnetic_compatibility::models::Path> path_list;
    path_list.push_back(ship0_path);
    Electromagnetic_compatibility::models::PathManager Total_Path(1,path_list);
    //---------------------------------------------根据时间采样，不断移动编队，计算电磁兼容情况----------------------------------
    for(int t_index = 0;t_index<t_step_num;t_index++){
        Electromagnetic_compatibility::models::move_location(ship_fleet,t_step,Total_Path);//移动船
        //根据频率是否在接收机范围进行筛选

        //对有指向性的天线进行筛选

        //使用自由空间衰减计算最坏的传输情况
        Electromagnetic_compatibility::models::FreeSpaceModel prop_modle_FREE;//采用自由空间衰减模型
        Electromagnetic_compatibility::core::EMCEngine EMC_engine(prop_modle_FREE);//实例化
        vector<Electromagnetic_compatibility::core::InterferenceResult> results = EMC_engine.analyzeFleet(ship_fleet);//存储编队内部所有的电磁兼容情况
        cout<<"results: "<<results.size()<<endl;
        cout<<results[0].victim_equip_id<<endl;
        //如果传输功率小于自身噪声，则排除

        //--------------------------------------------------第三章-----------------------------------------------------------
        //对有影响的信号，计算其杂波功率（海杂波与大气杂波）

        //在接收端计算SINR

        //将接收功率结合有效孔径转换成电场强度，并与标准场强比较
        //--------------------------------------------------第四章-----------------------------------------------------------
        //综合SINR以及电场强度判断电磁兼容情况

        //提出电磁屏蔽来衰减功率或电场的影响

    }
    return 0;
}

    



    //------------------------------------------------测试exam-----------------------------------------------------------
    // //----------------------------------干扰矩阵生成--------------------------------------
    // //计算距离，生成距离二维矩阵
    // vector<vector<double>> distance_arr = Electromagnetic_compatibility::models::calculate_distance(data);
    // //根据距离矩阵生成判断矩阵
    // vector<vector<int>> formation_distance_arr = Electromagnetic_compatibility::models::formation_distance(distance_arr);
    // // for(int i = 0;i<formation_distance_arr.size();i++){
    // //     for(int j = 0;j<formation_distance_arr[0].size();j++){
    // //         cout<<formation_distance_arr[i][j]<<" ";
    // //     }
    // //     cout<<endl;
    // // }//输出判断矩阵
    // // cout<<formation_distance_arr[3][3];
    // int choice = 0;
    // // 选择特定舰船进行兼容性分析
    // // cin>>choice;
    // // 针对formation_distance_arr[choice]进行兼容性分析
    // int x = data["ship"+to_string(choice)].first;
    // int y = data["ship"+to_string(choice)].second;
//
//     //--------------------------计算干扰距离--------------------------------------------
//     // 测试单位转换
//     double power_watts = dbmToWatts(20.0); // 20 dBm
//     std::cout << "20 dBm is " << power_watts << " Watts." << std::endl;
//     std::cout << power_watts << " Watts is " << wattsToDbm(power_watts) << " dBm." << std::endl;
//     //测试传播模型
//     Electromagnetic_compatibility::models::FreeSpaceModel fs_model;
//     Electromagnetic_compatibility::core::Point2D p1 = {0, 0};
//     Electromagnetic_compatibility::core::Point2D p2 = {1000, 0}; // 1 km distance
//     double freq_mhz = 433.0;
//
//     double path_loss = fs_model.getPathLossDb(p1, p2, freq_mhz);
//     std::cout << "Path loss at " << freq_mhz << " MHz over "
//             << "1000m: " << path_loss << " dB" << std::endl; // Should be around 84.7 dB
//
//     freq_mhz = 1575.42; // GPS L1
//     path_loss = fs_model.getPathLossDb(p1, p2, freq_mhz);
//     std::cout << "Path loss at " << freq_mhz << " MHz over "
//             << "1000m: " << path_loss << " dB" << std::endl; // Should be around 95.9 dB
//
//     std::cout << "Phase 3 test complete." << std::endl;
//     std::cout << "Basic structure test complete." << std::endl;
//
//
//
//
// // ---------------创建对象------------------------
//
//     auto ship0 = std::make_unique<ship>(unordered_map<string,pair<int,int>>{{"ship"+to_string(choice),{x,y}}}, distance_arr[choice]);//make_unique是C++11标准库中的一个智能指针，用于创建一个unique_ptr对象，它会在离开作用域时自动销毁所管理的对象
//     std::cout << "Created ship: " << ship0->getID()
//               << " at (" << ship0->getLocation().x << ", " << ship0->getLocation().y << ")"<<endl;
//              // << " orientation: " << ship1->getOrientationDeg() << " deg" << std::endl;
//     ship_fleet.addShip(std::move_location(ship0));
//     for (int i = 0;i<formation_distance_arr[choice].size();i++) {
//         if (formation_distance_arr[choice][i] == 1&& i!=choice ) {
//             int x = data["ship"+to_string(i)].first;
//             int y = data["ship"+to_string(i)].second;
//             auto ship_inDistance = std::make_unique<ship>(unordered_map<string,pair<int,int>>{{"ship"+to_string(i),{x,y}}},distance_arr[i-1]);
//             ship_fleet.addShip(std::move_location(ship_inDistance));
//         }
//     }
//     //测试编队模型
//     auto ship1 = std::make_unique<ship>(unordered_map<string,pair<int,int>>{{"ship"+to_string(1),{2,3}}}, distance_arr[1]);
//
//     ship_fleet.addShip(std::move_location(ship1));

