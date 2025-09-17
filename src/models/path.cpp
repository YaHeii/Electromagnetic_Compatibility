#include "../../include/models/Path.h"
#include "../../include/core/fleet.h"
#include <algorithm>

// Path 类实现

// 构造函数：明确指定时间范围
Electromagnetic_compatibility::models::Path::Path(int start_time, int end_time, core::Fleet* ship_fleet)
    : start_time(start_time), end_time(end_time), ship_fleet(ship_fleet) {
}

// 获取时间范围
int Electromagnetic_compatibility::models::Path::getStartTime() const {
    return start_time;
}

int Electromagnetic_compatibility::models::Path::getEndTime() const {
    return end_time;
}

int Electromagnetic_compatibility::models::Path::getTIndex() const {
    return start_time;  // 保持向后兼容
}

// 判断时间是否在范围内
bool Electromagnetic_compatibility::models::Path::isTimeInRange(int time) const {
    return time >= start_time && time <= end_time;
}

// 获取时间段长度
int Electromagnetic_compatibility::models::Path::getDuration() const {
    return end_time - start_time + 1;  // 包含开始和结束时间
}

// 设置时间范围
void Electromagnetic_compatibility::models::Path::setTimeRange(int start_time, int end_time) {
    this->start_time = start_time;
    this->end_time = end_time;
}

// 获取船队指针
const Electromagnetic_compatibility::core::Fleet* Electromagnetic_compatibility::models::Path::getShipFleet() const {
    return ship_fleet;
}