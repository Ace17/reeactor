#include "imgui.h"
#include "backend.h"
#include "game.h"
#include <math.h>
#include <vector>
#include <memory>
#include <map>
#include "SDL.h"

namespace
{
int gameTicks = 0;
auto const GAME_PERIOD_IN_TICKS = 20;
///////////////////////////////////////////////////////////////////////////////
// ImVec2 primitives

inline bool isPointInRect(ImVec2 point, ImVec2 rectPos, ImVec2 rectSize)
{
  if(point.x < rectPos.x || point.x >= rectPos.x + rectSize.x)
    return false;

  if(point.y < rectPos.y || point.y >= rectPos.y + rectSize.y)
    return false;

  return true;
}

inline ImVec2 operator + (ImVec2 a, ImVec2 b) { return ImVec2(a.x + b.x, a.y + b.y); }
inline ImVec2 operator * (ImVec2 a, float b) { return ImVec2(a.x * b, a.y * b); }
inline ImVec2 operator - (ImVec2 a, ImVec2 b) { return ImVec2(a.x - b.x, a.y - b.y); }

///////////////////////////////////////////////////////////////////////////////
// Texture cache

std::map<const char*, intptr_t> textureCache;

intptr_t getTexture(const char* path)
{
  auto i = textureCache.find(path);

  if(i == textureCache.end())
  {
    textureCache[path] = LoadTextureFromFile(path);
    i = textureCache.find(path);
  }

  return i->second;
}

///////////////////////////////////////////////////////////////////////////////

const float SCALE = 64.0;

intptr_t textureSelection;
intptr_t textureBackground;
intptr_t textureHover;
intptr_t textureFlow;

Entity* g_selection;
bool g_debug;

ImVec2 toImVec2(Vec2f v) { return ImVec2(v.x, v.y); }

inline ImVec2 ImRotate(const ImVec2& v, float cos_a, float sin_a)
{
  return ImVec2(v.x * cos_a - v.y * sin_a, v.x * sin_a + v.y * cos_a);
}

void ImageRotated(ImTextureID tex_id, ImVec2 size, float angle, ImVec2 uv0 = ImVec2(0, 0), ImVec2 uv1 = ImVec2(1, 1))
{
  ImVec2 center = ImGui::GetCursorScreenPos();

  float cos_a = cosf(angle);
  float sin_a = sinf(angle);
  ImVec2 pos[4] =
  {
    center + ImRotate(ImVec2(-size.x * 0.5f, -size.y * 0.5f), cos_a, sin_a),
    center + ImRotate(ImVec2(+size.x * 0.5f, -size.y * 0.5f), cos_a, sin_a),
    center + ImRotate(ImVec2(+size.x * 0.5f, +size.y * 0.5f), cos_a, sin_a),
    center + ImRotate(ImVec2(-size.x * 0.5f, +size.y * 0.5f), cos_a, sin_a)
  };
  ImVec2 uvs[4] =
  {
    ImVec2(uv0.x, uv0.y),
    ImVec2(uv1.x, uv0.y),
    ImVec2(uv1.x, uv1.y),
    ImVec2(uv0.x, uv1.y)
  };

  ImGui::GetWindowDrawList()->AddImageQuad(tex_id, pos[0], pos[1], pos[2], pos[3], uvs[0], uvs[1], uvs[2], uvs[3], IM_COL32_WHITE);
}

const int H = 512;

void windowReactorControl(ImVec2 size)
{
  ImGui::SetNextWindowPos(ImVec2(0, 0));
  ImGui::SetNextWindowSize(ImVec2(H, size.y));
  ImGui::Begin("Reactor control", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

  if(g_selection)
  {
    ImGui::Text("Name: %s", g_selection->id.c_str());
    ImGui::Text("Type: %s", g_selection->name());
    ImGui::Text("");

    for(auto& prop : g_selection->introspect())
    {
      switch(prop.type)
      {
      case Type::Float:

        if(prop.readOnly)
          ImGui::Text("%s: %.2f", prop.name, *(float*)prop.pointer);
        else
          ImGui::SliderFloat(prop.name, (float*)prop.pointer, 0, 1);

        break;
      case Type::Bool:
        ImGui::Checkbox(prop.name, (bool*)prop.pointer);
        break;
      }
    }
  }
  else
  {
    ImGui::Text("Please select an element from the diagram");
  }

  ImGui::End();
}

void windowReactorDiagram(ImVec2 size, const char* msg)
{
  auto absMousePos = ImGui::GetMousePos();
  const auto origin = ImVec2(H, 0);
  ImGui::SetNextWindowPos(origin);
  ImGui::SetNextWindowSize(ImVec2(size.x - H, size.y));
  ImGui::SetNextWindowBgAlpha(0.1);
  ImGui::Begin("Reactor diagram", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

  const auto scrollPos = ImVec2(ImGui::GetScrollX(), ImGui::GetScrollY());
  const auto mousePos = absMousePos - origin + scrollPos;

  ImGui::SetCursorPos({});
  ImGui::Image((void*)textureBackground, ImVec2(16 * SCALE, 16 * SCALE));

  if(g_debug)
    ImGui::Text("Temperature display");

  for(auto& entity : g_entities)
  {
    ImVec2 entityPos = toImVec2(entity->pos) * SCALE;
    ImVec2 entitySize = toImVec2(entity->size()) * SCALE;

    ImGui::SetCursorPos(entityPos);
    ImGui::Image((void*)getTexture("data/pipe.png"), entitySize);

    for(auto& sprite : entity->sprite())
    {
      ImGui::SetCursorPos(entityPos + entitySize * 0.5);
      ImageRotated((void*)getTexture(sprite.texture), entitySize, sprite.angle + entity->angle);
    }

    if(entity->selectable() && !msg)
    {
      bool mouseOver = isPointInRect(mousePos, entityPos, entitySize);

      if(entity.get() == g_selection)
      {
        ImGui::SetCursorPos(entityPos);
        ImGui::Image((void*)textureSelection, entitySize);
      }
      else if(mouseOver)
      {
        ImGui::SetCursorPos(entityPos);
        ImGui::Image((void*)textureHover, entitySize);

        if(ImGui::IsMouseClicked(0))
          g_selection = entity.get();
      }

      // show tooltip
      if(mouseOver)
      {
        ImGui::BeginTooltip();
        ImGui::Text("%s", entity->name());
        ImGui::Text("%s", entity->id.c_str());

        for(auto& prop : entity->introspect())
        {
          switch(prop.type)
          {
          case Type::Float:
            ImGui::Text("%s: %.2f", prop.name, *(float*)prop.pointer);
            break;
          case Type::Bool:
            ImGui::Text("%s: %s", prop.name, *(bool*)prop.pointer ? "on" : "off");
            break;
          }
        }

        ImGui::EndTooltip();
      }
    }

    if(g_debug)
    {
      // flow drawing
      {
        static std::map<Entity*, float> u;
        auto& phase = u[entity.get()];

        phase += entity->flux0() * 0.001;

        if(phase > 1.0)
          phase -= 1.0;

        ImGui::SetCursorPos(entityPos + entitySize * 0.5);
        auto uv0 = ImVec2(0 - phase, 0);
        auto uv1 = ImVec2(1 - phase, 1);
        ImageRotated((void*)textureFlow, entitySize, entity->angle, uv0, uv1);
      }

      uint8_t red = (int)clamp(entity->temperature(), 0, 255);
      ImGui::GetWindowDrawList()->AddRectFilled(origin - scrollPos + entityPos, origin - scrollPos + entityPos + entitySize, 0x80000000 | red);
      ImGui::SetCursorPos(entityPos);
      ImGui::Text("P=%.2f", entity->pressure());
      ImGui::SetCursorPos(entityPos + ImVec2(0, -16));
      ImGui::Text("m=%.2f", entity->mass() * 0.001);
    }
  }

  if(msg)
  {
    ImGui::SetCursorPos(size * 0.5 - origin);
    ImGui::Text("GAME FINISHED: %s", msg);
    ImGui::SetCursorPos(size * 0.5 - origin + ImVec2(0, 16));
    ImGui::Text("THANKS FOR PLAYING!");
    ImGui::SetCursorPos(size * 0.5 - origin + ImVec2(0, 32));
    ImGui::Text("PRESS 'R' TO PLAY AGAIN");
  }

  ImGui::End();
};
}

void AppInit()
{
  textureSelection = getTexture("data/rect.png");
  textureBackground = getTexture("data/full.png");
  textureHover = getTexture("data/hover.png");
  textureFlow = getTexture("data/flowalpha.png");

  GameInit();
}

void AppFrame(ImVec2 size, int deltaTicks)
{
  auto msg = IsGameFinished();

  if(!msg)
  {
    gameTicks += deltaTicks;

    while(gameTicks > GAME_PERIOD_IN_TICKS)
    {
      GameTick();
      gameTicks -= GAME_PERIOD_IN_TICKS;
    }
  }

  if(msg)
    g_debug = false;

  if(ImGui::IsKeyPressed(SDL_SCANCODE_R))
  {
    g_selection = nullptr;
    GameInit();
  }

  if(ImGui::IsKeyPressed(SDL_SCANCODE_SPACE))
    g_debug = !g_debug;

  windowReactorControl(size);
  windowReactorDiagram(size, msg);
}

