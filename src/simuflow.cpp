#include "simuflow.h"
#include <assert.h>

void connectSections(Circuit& circuit, Section& a, Section& b)
{
  circuit.connections.push_back(Connection{
    { &a, &b }, 0.0f });
}

void simulate(Circuit& circuit)
{
  const float dt = 1.0;

  // compute section pressures
  for(auto& s : circuit.sections)
  {
    assert(s.mass >= 0);
    s.P = (s.mass * 0.003 * s.T) / s.V;
  }

  // update flux
  for(auto& conn : circuit.connections)
  {
    auto& s0 = *conn.sections[0];
    auto& s1 = *conn.sections[1];
    conn.flux += (s0.P - s1.P + s0.selfFlux) * 0.1 * dt;
    conn.flux *= s0.damping;
  }

  // apply flux: update N
  for(auto& conn : circuit.connections)
  {
    auto* s0 = conn.sections[0];
    auto* s1 = conn.sections[1];
    auto dMass = conn.flux * dt;

    if(dMass < 0)
    {
      dMass = -dMass;
      std::swap(s0, s1);
    }

    // at this point,
    // we're transfering hot molecules from s0 to s1
    assert(dMass == dMass);
    assert(dMass >= 0);

    // don't transfer more molecules than available in s0
    dMass = std::min(dMass, s0->mass);

    // update s1 temperature
    if(dMass > 0)
      s1->T = (s1->T * s1->mass + s0->T * dMass) / (s1->mass + dMass);

    assert(s1->T == s1->T);
    assert(s1->T >= 0);

    s0->mass -= dMass;
    s1->mass += dMass;

    conn.flux = (conn.flux > 0 ? dMass : -dMass) / dt;

    // update flux0 for monitoring
    conn.sections[0]->flux0 = conn.flux;
  }
}

