#pragma once

struct Point
{
    float x{0.0f};
    float y{0.0f};
    float z{0.0f};

    Point() = default;

    Point(float _x, float _y, float _z = 0.0f) 
        : x(_x), y(_y), z(_z) {}

    Point(int _x, int _y, int _z = 0) 
        : x(static_cast<float>(_x)), y(static_cast<float>(_y)), z(static_cast<float>(_z)) {}

    int ix() const { return static_cast<int>(x); }
    int iy() const { return static_cast<int>(y); }
    int iz() const { return static_cast<int>(z); }
};

struct DPoint
{
	double X;
	double Y;
	double Z;
};
