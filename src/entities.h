#pragma once

#include "imgui.h" // ImVec2
#include <vector>
#include <string>
#include <memory>

using Vec2f = ImVec2;

inline ImVec2 operator + (ImVec2 a, ImVec2 b) { return ImVec2(a.x + b.x, a.y + b.y); }
inline ImVec2 operator * (ImVec2 a, float b) { return ImVec2(a.x * b, a.y * b); }
inline ImVec2 operator - (ImVec2 a, ImVec2 b) { return ImVec2(a.x - b.x, a.y - b.y); }

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

struct Entity
{
  Vec2f pos {};
  std::string id;
  Section* section = nullptr;
  float fluidQuantity();
  float temperature();
  float pressure();
  virtual void tick() {};
  virtual bool selectable() const { return true; }
  virtual Vec2f size() const { return Vec2f(1, 1); }
  virtual const char* texture() const = 0;
  virtual const char* texture2() const { return nullptr; };
  virtual float texture2angle() const { return 0; }
  virtual const char* name() const = 0;
  virtual std::vector<Property> introspect() const { return {}; };
};

extern std::vector<std::unique_ptr<Entity>> g_entities;
extern void GameInit();
extern void GameTick();
extern const char* IsGameFinished();

