#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include <sstream>
#include <map>
#include <unordered_map>

//计算每个坐标之间的距离
std::vector<std::vector<double>> calculate_distance(const std::unordered_map<std::string,std::pair<int,int>>& data);
//根据距离来实现编队的第一轮缩减
std::vector<std::vector<int>> formation_distance(const std::vector<std::vector<double>>& distance);
//要将电磁兼容范围内的船舶做等效源处


