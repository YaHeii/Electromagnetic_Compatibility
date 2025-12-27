#include "../../include/models/Path.h"
#include "../../include/core/fleet.h"
#include <algorithm>

// Path 类实现

// 构造函数：明确指定时间范围
Path::Path(int start_time, int end_time, Fleet* ship_fleet)
    : start_time(start_time), end_time(end_time), ship_fleet(ship_fleet) {
}

// 获取时间范围
int Path::getStartTime() const {
    return start_time;
}

int Path::getEndTime() const {
    return end_time;
}

int Path::getTIndex() const {
    return start_time;  // 保持向后兼容
}

// 判断时间是否在范围内
bool Path::isTimeInRange(int time) const {
    return time >= start_time && time <= end_time;
}

// 获取时间段长度
int Path::getDuration() const {
    return end_time - start_time + 1;  // 包含开始和结束时间
}

// 设置时间范围
void Path::setTimeRange(int start_time, int end_time) {
    this->start_time = start_time;
    this->end_time = end_time;
}

// 获取船队指针
const Fleet* Path::getShipFleet() const {
    return ship_fleet;
}