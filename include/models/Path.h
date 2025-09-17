#ifndef ELECTROMAGNETIC_COMPATIBILITY_MODELS_PATH_H
#define ELECTROMAGNETIC_COMPATIBILITY_MODELS_PATH_H

//记录不同时刻 无人船位置。单个船
#include <iostream>
#include <vector>
#include "../core/fleet.h"
#include "../core/ship.h"

//定义一个类，索引是一个时间段，值是船的id，船速，航向等
namespace Electromagnetic_compatibility {
namespace models {
class Path{
    public:
        // 构造函数：明确指定时间范围
        Path(int start_time, int end_time, Electromagnetic_compatibility::core::Fleet* ship_fleet);
        
        // 获取时间范围
        int getStartTime() const;
        int getEndTime() const;
        int getTIndex() const;  // 保持向后兼容，返回开始时间
        
        // 判断时间是否在范围内
        bool isTimeInRange(int time) const;
        
        // 获取时间段长度
        int getDuration() const;
        
        // 设置时间范围
        void setTimeRange(int start_time, int end_time);
        
        // 获取船队指针
        const Electromagnetic_compatibility::core::Fleet* getShipFleet() const;
        
    private:
        int start_time;  // 开始时间
        int end_time;    // 结束时间
        Electromagnetic_compatibility::core::Fleet* ship_fleet;  // 改为指针
};

}
}   

#endif // ELECTROMAGNETIC_COMPATIBILITY_MODELS_PATH_H

