#pragma once

#include <algorithm>
#include <cmath>

namespace CinematicSystem::Interpolation {

inline float Linear(float start, float end, float t)
{
    t = std::clamp(t, 0.0f, 1.0f);
    return start + (end - start) * t;
}

inline float SmootherStep(float t)
{
    t = std::clamp(t, 0.0f, 1.0f);
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

inline float ShortestAngleDelta(float start, float end, float period)
{
    return std::remainder(end - start, period);
}

template <class Point>
Point CatmullRom(const Point& p0, const Point& p1, const Point& p2, const Point& p3, float t)
{
    const float t2 = t * t;
    const float t3 = t2 * t;
    return {
        0.5f * (2.0f * p1.x + (-p0.x + p2.x) * t + (2.0f * p0.x - 5.0f * p1.x + 4.0f * p2.x - p3.x) * t2 + (-p0.x + 3.0f * p1.x - 3.0f * p2.x + p3.x) * t3),
        0.5f * (2.0f * p1.y + (-p0.y + p2.y) * t + (2.0f * p0.y - 5.0f * p1.y + 4.0f * p2.y - p3.y) * t2 + (-p0.y + 3.0f * p1.y - 3.0f * p2.y + p3.y) * t3),
        0.5f * (2.0f * p1.z + (-p0.z + p2.z) * t + (2.0f * p0.z - 5.0f * p1.z + 4.0f * p2.z - p3.z) * t2 + (-p0.z + 3.0f * p1.z - 3.0f * p2.z + p3.z) * t3)
    };
}

}  // namespace CinematicSystem::Interpolation