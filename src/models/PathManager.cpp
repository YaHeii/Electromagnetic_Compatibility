#include "../../include/models/PathManager.h"
#include <stdexcept>

// PathManager 类实现

Electromagnetic_compatibility::models::PathManager::PathManager(int max_num_path, std::vector<Path> Path_list) {
    this->max_num_path = max_num_path;
    this->Path_list = Path_list;
}

Electromagnetic_compatibility::models::Path Electromagnetic_compatibility::models::PathManager::Id_findPath(std::string ship_id, std::vector<Path> path_list) const {
    for (const auto& path : path_list) {
        if (path.getShipFleet()->findShipByID(ship_id) != nullptr) {
            return path;
        }
    }
    // 如果没有找到匹配的路径，抛出异常
    throw std::runtime_error("No path found for ship ID: " + ship_id);
}

// 获取指定时间的路径算法
Electromagnetic_compatibility::models::Path Electromagnetic_compatibility::models::PathManager::getPathsInTimeRange(int time_index, std::vector<Path> path_list) const {
    for (const auto& path : path_list) {
        // 检查时间范围是否有重叠
        if (!(path.getEndTime() < time_index || path.getStartTime() > time_index)) {
            return path;
        }
    }
    // 如果没有找到匹配的路径，抛出异常
    throw std::runtime_error("No path found for time index: " + std::to_string(time_index));
}

// 判断指定时间是否有路径数据
bool Electromagnetic_compatibility::models::PathManager::hasPathAtTime(int time) const {
    try {
        getPathsInTimeRange(time, Path_list);
        return true;
    } catch (const std::runtime_error&) {
        return false;
    }
}
vector<Electromagnetic_compatibility::models::Path> Electromagnetic_compatibility::models::PathManager::getPathList() const {
    return Path_list;
}
void Electromagnetic_compatibility::models::PathManager::setPathList(vector<Path> Path_list) {
    this->Path_list = Path_list;
}
int Electromagnetic_compatibility::models::PathManager::getMaxNumPath() const {
    return max_num_path;
}
void Electromagnetic_compatibility::models::PathManager::setMaxNumPath(int max_num_path) {
    this->max_num_path = max_num_path;
}