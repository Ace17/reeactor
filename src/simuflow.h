// Simulation of fluid flowing inside pipes.
#pragma once

#include <vector>

// A constant-volume section of the pipeline,
// potentially connected to other sections.
// Keeps track of the fluid mass and its temperature
// inside the section.
struct Section
{
  // user-updated quantities
  float selfFlux = 0; // set to non-zero for pumps
  float damping = 0.99; // set to less 0 for valves

  // simulator-updated quantities
  float mass = 0.0; // mass of fluid inside the section
  float T = 25.0; // temperature

  // non-persistent quantities (=recomputed each frame)
  float flux0; // flux of the first connection
  float P; // pressure

  // constant quantities
  float V = 1.0; // volume (constant because sections are rigid)
};

struct Connection
{
  Section* sections[2];
  float flux; // algebraic amount of fluid going from sections[0] to sections[1],
  // in units of mass per units of time.
};

struct Circuit
{
  // [Section 0] -> [Flux 0] -> [Section 1] -> [Flux 1] ...
  std::vector<Section> sections;
  std::vector<Connection> connections;
};

void connectSections(Circuit& circuit, Section& a, Section& b);
void simulate(Circuit& circuit);

