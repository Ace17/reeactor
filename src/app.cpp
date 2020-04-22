#include "imgui.h"
#include "backend.h"
#include "game.h"
#include <math.h>
#include <vector>
#include <memory>
#include <map>
#include "SDL.h"

bool isPointInRect(ImVec2 point, ImVec2 rectPos, ImVec2 rectSize)
{
  if(point.x < rectPos.x || point.x >= rectPos.x + rectSize.x)
    return false;

  if(point.y < rectPos.y || point.y >= rectPos.y + rectSize.y)
    return false;

  return true;
}

const float SCALE = 64.0;

std::map<const char*, intptr_t> textures;

intptr_t textureSelection;
intptr_t textureBackground;
intptr_t textureHover;

Entity* g_selection;

void AppInit()
{
  int w, h;
  textureSelection = LoadTextureFromFile("data/rect.png", w, h);
  textureBackground = LoadTextureFromFile("data/full.png", w, h);
  textureHover = LoadTextureFromFile("data/hover.png", w, h);

  GameInit();
}

intptr_t getTexture(const char* path)
{
  auto i = textures.find(path);

  if(i == textures.end())
  {
    int w, h;
    textures[path] = LoadTextureFromFile(path, w, h);
    i = textures.find(path);
  }

  return i->second;
}

static inline ImVec2 ImRotate(const ImVec2& v, float cos_a, float sin_a)
{
  return ImVec2(v.x * cos_a - v.y * sin_a, v.x * sin_a + v.y * cos_a);
}

void ImageRotated(ImTextureID tex_id, ImVec2 size, float angle)
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
    ImVec2(0.0f, 0.0f),
    ImVec2(1.0f, 0.0f),
    ImVec2(1.0f, 1.0f),
    ImVec2(0.0f, 1.0f)
  };

  ImGui::GetWindowDrawList()->AddImageQuad(tex_id, pos[0], pos[1], pos[2], pos[3], uvs[0], uvs[1], uvs[2], uvs[3], IM_COL32_WHITE);
}

bool g_debug;

void AppFrame(ImVec2 size)
{
  auto msg = IsGameFinished();

  if(!msg)
    GameTick();

  if(msg)
    g_debug = false;

  if(ImGui::IsKeyPressed(SDL_SCANCODE_R))
  {
    g_selection = nullptr;
    GameInit();
  }

  if(ImGui::IsKeyPressed(SDL_SCANCODE_SPACE))
    g_debug = !g_debug;

  auto absMousePos = ImGui::GetMousePos();
  const int H = 512;

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

  {
    const auto origin = ImVec2(H, 0);
    ImGui::SetNextWindowPos(origin);
    ImGui::SetNextWindowSize(ImVec2(size.x - H, size.y));
    ImGui::SetNextWindowBgAlpha(0.1);
    ImGui::Begin("Reactor diagram", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    const auto scrollPos = ImVec2(ImGui::GetScrollX(), ImGui::GetScrollY());
    const auto mousePos = absMousePos - origin + scrollPos;

    ImGui::SetCursorPos({});
    ImGui::Image((void*)textureBackground, ImVec2(1024, 1024));

    if(g_debug)
      ImGui::Text("Temperature display");

    for(auto& entity : g_entities)
    {
      auto entityPos = entity->pos * SCALE;
      auto entitySize = entity->size() * SCALE;

      ImGui::SetCursorPos(entityPos);
      ImGui::Image((void*)getTexture("data/pipe.png"), entitySize);

      if(auto texture = entity->texture())
      {
        ImGui::SetCursorPos(entityPos);
        ImGui::Image((void*)getTexture(texture), entitySize);
      }

      if(auto texture2 = entity->texture2())
      {
        ImGui::SetCursorPos(entityPos + entitySize * 0.5);
        ImageRotated((void*)getTexture(texture2), entitySize, entity->texture2angle());
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
        uint8_t red = (int)clamp(entity->temperature(), 0, 255);
        ImGui::GetWindowDrawList()->AddRectFilled(origin - scrollPos + entityPos, origin - scrollPos + entityPos + entitySize, 0x80000000 | red);
        ImGui::SetCursorPos(entityPos);
        ImGui::Text("P=%.2f", entity->pressure());
        ImGui::SetCursorPos(entityPos + ImVec2(0, -16));
        ImGui::Text("N=%.2f", entity->fluidQuantity() * 0.001);
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
  }
}

