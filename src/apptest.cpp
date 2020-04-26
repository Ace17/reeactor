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
Circuit g_circuit;
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

  {
    auto& s = g_circuit.sections.front();
    ImGui::SliderFloat("SelfFlux", &s.selfFlux, 0, 1000);
    ImGui::SliderFloat("Temperature", &s.T, 0, 200);

    auto& sMid = g_circuit.sections[g_circuit.sections.size() * 3 / 4];
    ImGui::SliderFloat("MidDamping", &sMid.damping, 0, 1);
  }

  static std::vector<float> u;
  u.resize(g_circuit.sections.size());

  for(int i = 0; i < (int)g_circuit.sections.size(); ++i)
  {
    auto& s = g_circuit.sections[i];

    u[i] += s.flux0 * 0.001;

    if(u[i] > 1.0)
      u[i] -= 1.0;

    ImGui::SetCursorPos(pos);
    auto uv0 = ImVec2(0 - u[i], 0);
    auto uv1 = ImVec2(1 - u[i], 1);
    ImGui::Image((void*)textureFlow, ImVec2(64, 64), uv0, uv1);

    uint8_t red = clamp(s.T, 0, 255);
    ImGui::GetWindowDrawList()->AddRectFilled(pos, pos + ImVec2(64, 64), 0x80000000 | red);

    pos.x += 64 + 1;
  }

  ImGui::End();
};
}

void AppInit()
{
  textureFlow = LoadTextureFromFile("data/flow.png");

  g_circuit.sections.resize(20);

  for(auto& s : g_circuit.sections)
  {
    s.n = 1000;
    s.T = 25;
  }

  for(int i = 0; i < (int)g_circuit.sections.size(); ++i)
  {
    int i0 = (i + 0) % g_circuit.sections.size();
    int i1 = (i + 1) % g_circuit.sections.size();
    connectSections(
      g_circuit,
      g_circuit.sections[i0],
      g_circuit.sections[i1]);
  }
}

void AppFrame(ImVec2 size, int deltaTicks)
{
  simulate(g_circuit);
  mainWindow(size);
}

