#include "../include/models/Computing_distance.h"
#include "../include/core/ship.h"
#include <unordered_map>
#include <iostream>
#include <string>
#include <vector>
#include <cmath>

// 计算船的距离矩阵（二维数组）
// 将返回类型改为 double，因为距离通常是浮点数
    std::vector<std::vector<double>> calculate_distance(const std::unordered_map<std::string, std::pair<int,int>>& data) {
    std::vector<std::vector<double>> distance_arr; // 存储 double 类型距离

    int row_index = 0;
    for (auto const& item_p1 : data) { // item_p1 是第一个点 (key, value)
        double x1_value = item_p1.second.first;
        double y1_value = item_p1.second.second;

        // 为当前行创建一个新的 vector 来存储与所有其他点的距离
        std::vector<double> current_row_distances;
        current_row_distances.reserve(data.size());

        for (auto const& item_p2 : data) { // item_p2 是第二个点 (key, value)
            double x2_value = item_p2.second.first;
            double y2_value = item_p2.second.second;

            double dx = x1_value - x2_value;
            double dy = y1_value - y2_value;

            double distance = std::sqrt(dx * dx + dy * dy);

            current_row_distances.push_back(distance);
        }
        distance_arr.push_back(current_row_distances); // 将填充好的行添加到结果矩阵
        row_index++; // 递增行索引
    }
    return distance_arr;
}
//使用一个二维数组来表达干扰情况，距离在干扰范围内，设置为1
    std::vector<std::vector<int>> formation_distance(const std::vector<std::vector<double>>& distance_arr){
    int rows = distance_arr.size();
    int cols = distance_arr[0].size();
    std::vector<std::vector<int>> formation_distance_arr(rows, std::vector<int>(cols,0));
    for(int i = 0;i < distance_arr.size();++i) {
        for (int j = 0; j < distance_arr[0].size(); ++j) {
            if (distance_arr[i][j] < 1002 && i!=j) {
                formation_distance_arr[i][j] = 1;
            }
        }
    }
    return formation_distance_arr;
}


