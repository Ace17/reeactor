#pragma once

#include <vector>

struct Section
{
  // user-updated quantities
  float selfFlux = 0; // set to non-zero for pumps
  float fluxRatio = 1; // set to less than 0 for valves

  // simulator-updated quantities
  float n = 0.0; // molecule count
  float T = 1.0; // temperature
  float flux0; // flux of the first connection

  // non-persistent quantities (=recomputed each frame)
  float P; // pressure

  // constant quantities
  float V = 1.0; // volume (constant because sections are rigid)
};

struct Connection
{
  Section* sections[2];
  float flux; // algebraic amount of fluid going from sections[0] to sections[1]
};

struct Circuit
{
  // [Section 0] -> [Flux 0] -> [Section 1] -> [Flux 1] ...
  std::vector<Section> sections;
  std::vector<Connection> connections;
};

void connectSections(Circuit& circuit, Section& a, Section& b);
void simulate(Circuit& circuit);

