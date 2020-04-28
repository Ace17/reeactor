#pragma once

// game logic, as seen by the rest of the program

#include <vector>
#include <string>
#include <memory>

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

enum class Type
{
  Bool,
  Float,
};

struct Property
{
  const char* name;
  Type type;
  void* pointer;
  bool readOnly = false;
};

struct Section;

struct Actor
{
  struct Sprite
  {
    const char* texture;
    float angle = 0;
  };

  virtual ~Actor() = default;

  Vec2f pos {};
  float angle = 0;
  std::string id;
  virtual float mass() = 0;
  virtual float temperature() = 0;
  virtual float pressure() = 0;
  virtual float flux0() = 0;
  virtual bool selectable() const { return true; }
  virtual Vec2f size() const { return Vec2f(1, 1); }
  virtual std::vector<Sprite> sprite() const = 0;
  virtual const char* name() const = 0;
  virtual std::vector<Property> introspect() const { return {}; };
};

extern std::vector<Actor*> GameGetActors();
extern void GameInit();
extern void GameTick();
extern const char* IsGameFinished();

