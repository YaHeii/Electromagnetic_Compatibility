#pragma once

#include "../core/fleet.h"
#include "../core/ship.h"
#include "Path.h"
#include <vector>
class PathManager{
public:
    PathManager(int max_num_path,std::vector<Path> Path_list);
    //获取指定时间的路径算法
    Path getPathsInTimeRange(int time_index,std::vector<Path> Path_list) const;
    //获取路径列表
    std::vector<Path> getPathList() const;
    //设置路径列表
    void setPathList(std::vector<Path> Path_list);
    //获取最大路径数
    int getMaxNumPath() const;
    //设置最大路径数
    void setMaxNumPath(int max_num_path);
    // 判断指定时间是否有路径数据
    bool hasPathAtTime(int time) const;
    Path Id_findPath(std::string ship_id,std::vector<Path> Path_list) const;
private:
    int max_num_path;
    std::vector<Path> Path_list;
};
