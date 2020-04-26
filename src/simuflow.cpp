#include "simuflow.h"
#include <assert.h>

void connectSections(Circuit& circuit, Section& a, Section& b)
{
  circuit.connections.push_back(Connection{
    { &a, &b }, 0.0f });
}

void simulate(Circuit& circuit)
{
  // compute section pressures
  for(auto& s : circuit.sections)
  {
    assert(s.n >= 0);
    s.P = (s.n * 0.003 * s.T) / s.V;
  }

  // update flux
  for(auto& conn : circuit.connections)
  {
    auto& s0 = *conn.sections[0];
    auto& s1 = *conn.sections[1];
    conn.flux += (s0.P - s1.P + s0.selfFlux) * 0.1;
    conn.flux *= s0.damping;
  }

  // apply flux: update N
  for(auto& conn : circuit.connections)
  {
    auto* s0 = conn.sections[0];
    auto* s1 = conn.sections[1];
    auto flux = conn.flux;

    if(flux < 0)
    {
      flux = -flux;
      std::swap(s0, s1);
    }

    // at this point,
    // we're transfering hot molecules from s0 to s1
    assert(flux == flux);
    assert(flux >= 0);

    // don't transfer more molecules than available in s0
    flux = std::min(flux, s0->n);

    // update s1 temperature
    if(flux > 0)
      s1->T = (s1->T * s1->n + s0->T * flux) / (s1->n + flux);

    assert(s1->T == s1->T);
    assert(s1->T >= 0);

    s0->n -= flux;
    s1->n += flux;

    conn.flux = conn.flux > 0 ? flux : -flux;

    // update flux0 for monitoring
    conn.sections[0]->flux0 = conn.flux;
  }
}

