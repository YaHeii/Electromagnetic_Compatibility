

EMC_Engine::EMC_Engine(const PropagationModel& prop_model)
    : m_prop_model(prop_model) {
}
std::vector<InterferenceResult> EMC_Engine::EMC_computing(const Fleet& fleet) {//返回受扰计算结果数组
    std::vector<InterferenceResult> results;

    const auto& ships = fleet.getShips();
    std::vector<std::pair<Transmitter*, ship*>> all_transmitters;
    std::vector<std::pair<Receiver*, ship*>> all_receivers;

    // 存储所有船的接收机与发射机
    for (const auto& ship_ptr : ships) {
        for (const auto& eq_ptr : ship_ptr->getEquipmentList()) {
            if (auto tx = dynamic_cast<Transmitter*>(eq_ptr.get())) {
                all_transmitters.push_back({ tx, ship_ptr.get() });
            }
            else if (auto rx = dynamic_cast<Receiver*>(eq_ptr.get())) {
                all_receivers.push_back({ rx, ship_ptr.get() });
            }
        }
    }
    //--- 受害船处理 ---
    // 迭代每个接收机
    for (const auto& rx_pair : all_receivers) {
        Receiver* victim_rx = rx_pair.first;
        ship* victim_ship = rx_pair.second;//定义受害船
        double total_interference_linear_watts = 0.0;

        double rad_orientation = victim_ship->getOrientationDeg() * M_PI / 180.0;//船的偏转角度
        double eq_rel_x = victim_rx->getRelativePosition().x;//设备相对于船的位置x
        double eq_rel_y = victim_rx->getRelativePosition().y;//设备相对于船的位置y
        Point2D victim_rx_global_pos = victim_ship->getLocation();//计算设备的全局位置
        victim_rx_global_pos.x += eq_rel_x * cos(rad_orientation) - eq_rel_y * sin(rad_orientation);
        victim_rx_global_pos.y += eq_rel_x * sin(rad_orientation) + eq_rel_y * cos(rad_orientation);


        //--------------------------------------------------攻击船处理------------------------------------------
        // 遍历发射器
        for (const auto& tx_pair : all_transmitters) {
            Transmitter* aggressor_tx = tx_pair.first;//发射设备
            ship* aggressor_ship = tx_pair.second;//发射船

            // 如果在同一艘船则跳过
            if (aggressor_ship->getID() == victim_ship->getID() && aggressor_tx->getID() == victim_rx->getID()) {
                continue;
            }
            if (aggressor_ship->getID() == victim_ship->getID()) {
                // std::cout << "Skipping intra-ship: " << aggressor_tx->getID() << " to " << victim_rx->getID() << std::endl;
                continue;
            }

            //攻击船坐标计算
            Point2D aggressor_tx_global_pos = aggressor_ship->getLocation();
            aggressor_tx_global_pos.x += aggressor_tx->getRelativePosition().x;
            aggressor_tx_global_pos.y += aggressor_tx->getRelativePosition().y;//攻击船的坐标计算比较简略

            //--------------------------------------------------计算步骤------------------------------------------
            // 1. 路径损耗
            double path_loss_db = m_prop_model.getPathLossDb(aggressor_tx_global_pos, victim_rx_global_pos, aggressor_tx->getFrequencyMHz());
            if (path_loss_db == std::numeric_limits<double>::infinity() || path_loss_db < 0) {
                // std::cout << "Warning: Invalid path loss." << std::endl;
                continue;
            }

            // 2. 天线增益计算
           /* double gain_tx_dbi = aggressor_tx->getAntenna() ? aggressor_tx->getAntenna()->getGainDbi(0) : 0.0;
            double gain_rx_dbi = victim_rx->getAntenna() ? victim_rx->getAntenna()->getGainDbi(0) : 0.0;*/
            double gain_tx_dbi = 0.0;
            double gain_rx_dbi = 0.0;


            // 3. 接收端功率
            double received_power_dbm = aggressor_tx->getPowerDBm() + gain_tx_dbi + gain_rx_dbi - path_loss_db;

            // 4. 对频段进行处理（复杂，还需要再考虑）
            double freq_diff_mhz = std::abs(aggressor_tx->getFrequencyMHz() - victim_rx->getFrequencyMHz());//计算增益差绝对值
            double victim_rx_bw_mhz = victim_rx->getBandwidthKHz() / 1000.0;//受害船频宽
            double aggressor_tx_bw_mhz = aggressor_tx->getBandwidthKHz() / 1000.0; //攻击船频宽

            bool is_potentially_interfering = (freq_diff_mhz < (victim_rx_bw_mhz / 2.0));//中心频率检查是否在一半的频带内


            if (is_potentially_interfering) {
                //如果更复杂的BPRF(带通滤波器抑制因子)可用：
                //双BPRF_db=calculate_BPRF_db(侵略者_TX->getFrequencyMHz()，victure_Rx->getFrequencyMHz()，victure_rx_bw_mhz)；
                //received_power_dbm-=BPRF_db；//减去拒绝

                total_interference_linear_watts += dbmToWatts(received_power_dbm);


            }
        } // End loop over transmitters

        // 计算最终结果
        double total_interference_dbm = wattsToDbm(total_interference_linear_watts);
        double noise_floor_dbm = victim_rx->getNoiseFloorDBm();

        //计算I+N，单位watt
        double total_interference_plus_noise_watts = total_interference_linear_watts + dbmToWatts(noise_floor_dbm);
        //计算S
        double S_db = fleet.findShipByID(victim_rx->getTransmitterInShipID())->findEquipmentByID(victim_rx->getTransmitterID())->getPowerDBm();//获得有效接收功率，单位dbm
        double S_watts = dbmToWatts(S_db);//转换为瓦特
        //计算SINR
        double SINR_watts = S_watts / total_interference_plus_noise_watts;
        double SINR_db = wattsToDbm(SINR_watts);


        //干扰裕度(IM)=灵敏度-(I+N)
        //正互调意味着即使有干扰和噪声，信号仍高于灵敏度。
        //阴性IM表示低于灵敏度，可能不起作用。
        double total_interference_plus_noise_dbm = wattsToDbm(total_interference_plus_noise_watts);

        double interference_margin_db = victim_rx->getSensitivityDBm() - total_interference_plus_noise_dbm;//指定干扰阈值

        //--------------------------------------------------输出结果------------------------------------------

        //储存计算结果
        InterferenceResult res;
        res.victim_ship_id = victim_ship->getID();
        res.victim_equip_id = victim_rx->getID();
        res.victim_rx_freq_mhz = victim_rx->getFrequencyMHz();
        res.aggressor_ship_id = "Multiple/Total";//可能为空或者多个
        res.aggressor_equip_id = "Multiple/Total";
        res.interference_power_at_rx_input_dbm = total_interference_dbm; // This is total I
        res.victim_noise_floor_dbm = noise_floor_dbm;
        res.interference_plus_noise_dbm = total_interference_plus_noise_dbm;
        res.interference_margin_db = interference_margin_db;
        res.communication_performance_db = SINR_db;
        res.is_communication_degraded = (SINR_db < victim_rx->getSINRThresholdDB());
        res.is_interference_degraded = (interference_margin_db < victim_rx->getInterferenceThresholdDB());
        results.push_back(res);

    }

    return results;
}



#include "../include/models/PEModel.h"
int main() {
    // 1. 仿真参数设置
    double freq = 9.4e9;       // 9.4 GHz (X-band)
    double dx = 50.0;          // 步进 50m
    double dz = 0.2;           // 垂直分辨率 0.2m (越高越好，建议 <= lambda/2)
    int nz = 2048;             // 物理高度网格 (总高度 ~400m)
    double max_range = 50000.0;// 50 km

    // 2. 初始化大气模型：蒸发波导高度 20m
    // 对应 Paper 2 Fig. 6(d) 和 Eq. (35)
    AtmosphereModel atm(20.0);

    // 3. 预计算折射率剖面 (Profile)
    // 这一步非常重要，避免在 step 循环中重复计算 log 函数，提升效率
    std::vector<double> n_profile(nz);
    for (int i = 0; i < nz; ++i) {
        double z = i * dz;
        n_profile[i] = atm.getRefractiveIndex(z);
    }

    // 4. 初始化求解器 (包含 FFTW3 和 镜像法)
    PEModel solver(freq, dx, dz, nz);

    // 初始化高斯波束：天线高度 25m
    solver.PEmodel_computing2D(25.0, 2.0, 0.0);

    // 5. 开始步进仿真
    std::cout << "Range(km) \t Loss(dB) \t (Atmosphere: Evaporation Duct 20m)" << std::endl;

    for (double r = 0; r < max_range; r += dx) {
        // [关键]：将预计算好的大气剖面传递给求解器
        // 7.0 是风速 (m/s)，用于计算 Miller-Brown 粗糙度
        solver.step_Miller_Brown(r, 7.0, n_profile);

        // 输出数据
        if (std::abs(fmod(r, 1000.0)) < 0.1) {
            // 获取接收天线高度 15m 处的损耗
            int rx_idx = static_cast<int>(15.0 / dz);
            double loss = solver.getPathLoss(rx_idx, r);
            std::cout << r / 1000.0 << " \t\t " << loss << std::endl;
        }
    }

    return 0;
}
