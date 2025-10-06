#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include <sstream>
#include <map>
#include <unordered_map>
#include "../utils/data_get.h"
using namespace std;

//计算每个坐标之间的距离
vector<vector<double>> calculate_distance(const unordered_map<string,pair<int,int>>& data);
//根据距离来实现编队的第一轮缩减
vector<vector<int>> formation_distance(const vector<vector<double>>& distance);
//要将电磁兼容范围内的船舶做等效源处


