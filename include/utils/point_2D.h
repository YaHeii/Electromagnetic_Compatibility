#pragma once

namespace Electromagnetic_compatibility {
namespace utils {
class Point2D {
    public:
    Point2D(double x, double y) : x(x), y(y) {}
    double getX() const { return x; }
    double getY() const { return y; }

    double x;
    double y;
};

} // namespace utils
} // namespace Electromagnetic_compatibility


// struct Point2D {
//     double x = 0.0;//QT设计时。考虑将数据规范化，
//     double y = 0.0;
// };

// // + 运算符重载，实现两个Point2D的坐标相加
// inline Point2D operator+(const Point2D& lhs, const Point2D& rhs) {
//     return Point2D{lhs.x + rhs.x, lhs.y + rhs.y};
// }
// double getX() { return x; }
// double getY() { return y; }