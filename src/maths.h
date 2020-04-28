#pragma once

struct Vec2f
{
  Vec2f() = default;
  Vec2f(float x_, float y_) : x(x_), y(y_) {}
  float x {};
  float y {};
};

inline Vec2f operator + (Vec2f a, Vec2f b) { return Vec2f(a.x + b.x, a.y + b.y); }
inline Vec2f operator * (Vec2f a, float b) { return Vec2f(a.x * b, a.y * b); }
inline Vec2f operator - (Vec2f a, Vec2f b) { return Vec2f(a.x - b.x, a.y - b.y); }

inline float clamp(float val, float min, float max)
{
  if(val < min)
    return min;

  if(val > max)
    return max;

  return val;
}

