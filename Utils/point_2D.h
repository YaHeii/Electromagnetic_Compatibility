#pragma once

class Point2D {
    public:
    Point2D(double x, double y) : x(x), y(y) {}
    double getX() const { return x; }
    double getY() const { return y; }

    double x;
    double y;
};

class Point3D {
    public:
    Point3D(double x, double y, double z) : _x(x), _y(y), _z(z) {}
    double getX() const { return _x; }
    double getY() const { return _y; }
    double getZ() const { return _z; }

    double _x;
    double _y;
    double _z;
};

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