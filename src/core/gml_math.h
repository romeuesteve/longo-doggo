/*
 * The GameMaker math the ports lean on, in one place.  Angles are
 * GameMaker degrees: 0 = down, 90 = right, 180 = up, 270 = left,
 * counterclockwise on screen; len_dir_y and point_direction negate the
 * sine to match.  View code passes these values through as rotation and
 * the render backend converts to its API exactly once.
 */
#ifndef LONGO_GML_MATH_H
#define LONGO_GML_MATH_H

#include <math.h>

#define GML_DEG_TO_RAD (3.14159265358979f / 180.0f)

static inline float f_lerp(float a, float b, float t)
{
    return a + (b - a) * t;
}

static inline float len_dir_x(float len, float dir)
{
    return cosf(dir * GML_DEG_TO_RAD) * len;
}

static inline float len_dir_y(float len, float dir)
{
    return -sinf(dir * GML_DEG_TO_RAD) * len;
}

static inline float point_direction(float x1, float y1, float x2, float y2)
{
    float dir = atan2f(-(y2 - y1), x2 - x1) * (180.0f / 3.14159265358979f);
    if (dir < 0.0f) dir += 360.0f;
    return dir;
}

#endif /* LONGO_GML_MATH_H */
