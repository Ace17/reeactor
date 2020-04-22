#include "imgui.h"
#include "backend.h"
#include "game.h"
#include <math.h>
#include <vector>
#include <memory>
#include <map>
#include "SDL.h"

#include "simuflow.h"

namespace
{
///////////////////////////////////////////////////////////////////////////////
// ImVec2 primitives

inline ImVec2 operator + (ImVec2 a, ImVec2 b) { return ImVec2(a.x + b.x, a.y + b.y); }
inline ImVec2 operator * (ImVec2 a, float b) { return ImVec2(a.x * b, a.y * b); }
inline ImVec2 operator - (ImVec2 a, ImVec2 b) { return ImVec2(a.x - b.x, a.y - b.y); }

///////////////////////////////////////////////////////////////////////////////

intptr_t textureFlow;
ImVec2 toImVec2(Vec2f v) { return ImVec2(v.x, v.y); }

void mainWindow(ImVec2 size)
{
  auto absMousePos = ImGui::GetMousePos();
  const auto origin = ImVec2(0, 0);
  ImGui::SetNextWindowPos(origin);
  ImGui::SetNextWindowSize(size);
  ImGui::SetNextWindowBgAlpha(0.1);
  ImGui::Begin("Test App", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

  auto pos = ImVec2(100, size.y / 2);

  static double t = 0;
  t += 0.01;

  for(int i = 0; i < 20; ++i)
  {
    ImGui::SetCursorPos(pos);
    auto uv0 = ImVec2(0 - t, 0);
    auto uv1 = ImVec2(1 - t, 1);
    ImGui::Image((void*)textureFlow, ImVec2(64, 64), uv0, uv1);

    uint8_t red = 10 * i;
    ImGui::GetWindowDrawList()->AddRectFilled(pos, pos + ImVec2(64, 64), 0x80000000 | red);

    pos.x += 64 + 1;
  }

  ImGui::End();
};
}

void AppInit()
{
  textureFlow = LoadTextureFromFile("data/flow.png");
}

void AppFrame(ImVec2 size)
{
  mainWindow(size);
}

