#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cmath>

class AntennaPatternInterpolator {
public:
    // 从文件加载数据
    bool loadFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            return false;
        }

        std::string line;
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            double theta, phi, gain;
            std::string value;

            // 读取第一列(theta)
            if (!(iss >> value)) continue;
            theta = std::stod(value);

            // 读取第二列(phi)
            if (!(iss >> value)) continue;
            phi = std::stod(value);

            // 跳过第三到第六列
            for (int i = 0; i < 4; ++i) {
                if (!(iss >> value)) continue;
            }

            // 读取第七列(gain)
            if (!(iss >> value)) continue;
            gain = std::stod(value);

            // 存储数据点
            dataPoints.push_back({theta, phi, gain});
            
            // 收集唯一的theta和phi值
            uniqueThetas.insert(theta);
            uniquePhis.insert(phi);
        }

        // 构建网格
        if (!buildGrid()) {
            return false;
        }

        return true;
    }

    // 查询增益值，支持插值
    double queryGain(double theta, double phi) const {
        // 检查是否有足够的数据进行插值
        if (grid.empty() || grid[0].empty()) {
            throw std::runtime_error("No data available for interpolation");
        }

        // 查找theta和phi的位置
        auto thetaIt = std::lower_bound(thetaValues.begin(), thetaValues.end(), theta);
        auto phiIt = std::lower_bound(phiValues.begin(), phiValues.end(), phi);

        // 处理边界情况
        if (thetaIt == thetaValues.begin()) {
            thetaIt = std::next(thetaIt);
        }
        if (thetaIt == thetaValues.end()) {
            thetaIt = std::prev(thetaIt);
        }
        if (phiIt == phiValues.begin()) {
            phiIt = std::next(phiIt);
        }
        if (phiIt == phiValues.end()) {
            phiIt = std::prev(phiIt);
        }

        size_t i = std::distance(thetaValues.begin(), thetaIt) - 1;
        size_t j = std::distance(phiValues.begin(), phiIt) - 1;

        // 获取四个最近点的坐标和增益值
        double theta1 = thetaValues[i];
        double theta2 = thetaValues[i+1];
        double phi1 = phiValues[j];
        double phi2 = phiValues[j+1];

        double q11 = grid[i][j];
        double q12 = grid[i][j+1];
        double q21 = grid[i+1][j];
        double q22 = grid[i+1][j+1];

        // 双线性插值
        double r1 = ((theta2 - theta) / (theta2 - theta1)) * q11 + ((theta - theta1) / (theta2 - theta1)) * q21;
        double r2 = ((theta2 - theta) / (theta2 - theta1)) * q12 + ((theta - theta1) / (theta2 - theta1)) * q22;
        double result = ((phi2 - phi) / (phi2 - phi1)) * r1 + ((phi - phi1) / (phi2 - phi1)) * r2;

        return result;
    }

private:
    // 数据点结构
    struct DataPoint {
        double theta;
        double phi;
        double gain;
    };

    std::vector<DataPoint> dataPoints;  // 原始数据点
    std::vector<double> thetaValues;    // 有序的theta值
    std::vector<double> phiValues;      // 有序的phi值
    std::vector<std::vector<double>> grid;  // 二维网格存储增益值

    std::set<double> uniqueThetas;  // 用于收集唯一的theta值
    std::set<double> uniquePhis;    // 用于收集唯一的phi值

    // 构建二维网格
    bool buildGrid() {
        // 将唯一值转换为有序向量
        thetaValues.assign(uniqueThetas.begin(), uniqueThetas.end());
        phiValues.assign(uniquePhis.begin(), uniquePhis.end());

        // 初始化网格
        size_t numThetas = thetaValues.size();
        size_t numPhis = phiValues.size();
        
        if (numThetas < 2 || numPhis < 2) {
            return false;  // 数据点不足，无法构建网格
        }

        grid.resize(numThetas, std::vector<double>(numPhis, 0.0));

        // 填充网格
        for (const auto& point : dataPoints) {
            // 查找theta和phi在向量中的位置
            auto thetaIt = std::lower_bound(thetaValues.begin(), thetaValues.end(), point.theta);
            auto phiIt = std::lower_bound(phiValues.begin(), phiValues.end(), point.phi);

            if (thetaIt != thetaValues.end() && phiIt != phiValues.end() &&
                *thetaIt == point.theta && *phiIt == point.phi) {
                
                size_t i = std::distance(thetaValues.begin(), thetaIt);
                size_t j = std::distance(phiValues.begin(), phiIt);
                
                grid[i][j] = point.gain;
            }
        }

        return true;
    }
};

  