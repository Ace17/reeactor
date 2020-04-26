#include <memory>
#include "game.h"
#include "simuflow.h"

std::vector<std::unique_ptr<Entity>> g_entities;

float Entity::fluidQuantity()
{
  return section ? section->n : 0;
}

float Entity::temperature()
{
  return section ? section->T : 0;
}

float Entity::pressure()
{
  return section ? section->P : 0;
}

namespace
{
const char* g_finishMessage = nullptr;
auto const TAU = 6.28318530717958647693;

float blend(float alpha, float a, float b)
{
  return (1 - alpha) * a + alpha * b;
}

Circuit g_circuit;

void connect(Entity* a, Entity* b)
{
  connectSections(g_circuit, *a->section, *b->section);
}

void connect(std::vector<Entity*> entities)
{
  for(int i = 0; i + 1 < entities.size(); ++i)
    connectSections(g_circuit, *entities[i]->section, *entities[i + 1]->section);
}

struct EPipe : Entity
{
  bool selectable() const override { return false; }
  const char* texture() const override { return "data/pipe.png"; }
  const char* name() const override { return "Water Pipe"; };
};

struct EReactor : Entity
{
  void tick() override
  {
    section->T += 4.0 * controlRods;
    temperature = section->T;

    if(section->T > 300)
      g_finishMessage = "YOU LOSE: THE CORE HAS MOLTEN";
  }

  Vec2f size() const override { return Vec2f(2, 4); }
  const char* texture() const override { return "data/reactor.png"; }
  const char* name() const override { return "Reactor Core"; };

  std::vector<Property> introspect() const override
  {
    return {
      Property{ "Control Rods", Type::Float, (void*)&controlRods },
      Property{ "Temperature Reading", Type::Float, (void*)&temperature, true },
    };
  }

  float controlRods = 0.1;
  float temperature = 200.0;
};

struct ETurbine : Entity
{
  void tick() override
  {
    section->T = blend(0.99, section->T, 20.0); // heat dissipation

    if(section->T > 25)
      speed += section->T * 0.001;

    speed *= 0.99; // friction
  }

  Vec2f size() const override { return Vec2f(2, 3); }
  const char* texture() const override { return "data/turbine.png"; }
  const char* name() const override { return "Steam Turbine"; };

  std::vector<Property> introspect() const override
  {
    return {
      Property{ "Angular Speed", Type::Float, (void*)&speed, true },
    };
  }

  float speed = 0;
};

struct EGenerator : Entity
{
  void tick() override
  {
    power = turbine->speed;
    totalEnergy += power * 0.001;

    if(totalEnergy > 100)
      g_finishMessage = "YOU WIN";
  }

  Vec2f size() const override { return Vec2f(2, 1); }
  const char* texture() const override { return "data/generator.png"; }
  const char* name() const override { return "Power Generator"; };

  std::vector<Property> introspect() const override
  {
    return {
      Property{ "Power (MWe)", Type::Float, (void*)&power, true },
      Property{ "Total Energy", Type::Float, (void*)&totalEnergy, true },
    };
  }

  ETurbine* turbine = nullptr;
  float power = 0;
  float totalEnergy = 0;
};

float randFloat()
{
  return rand() / float(RAND_MAX);
}

struct EPump : Entity
{
  void tick() override
  {
    float flux = enable ? powerRatio * fullPower : 0;

    // +/- 20%
    flux *= 1.0f + randFloat() * 0.4 - 0.2;

    section->selfFlux = flux;

    angle += flux * 0.01;

    if(angle > TAU)
      angle -= TAU;
  }

  const char* texture() const override { return "data/pump.png"; }
  const char* texture2() const override { return "data/pump2.png"; }
  float texture2angle() const override { return -angle; }
  const char* name() const override { return "Water Pump"; };
  std::vector<Property> introspect() const override
  {
    return {
      Property{ "Enable", Type::Bool, (void*)&enable },
      Property{ "Power", Type::Float, (void*)&powerRatio },
    };
  }

  bool enable = true;
  float powerRatio = 0.5;
  float angle = 0;
  const float fullPower = 30.0;
};

struct EManometer : Entity
{
  void tick() override
  {
    pressure = blend(0.1, pressure, section->P);
  }

  const char* texture() const override { return "data/manometer.png"; }
  virtual const char* texture2() const { return "data/manometer_pin.png"; };
  virtual float texture2angle() const { return clamp(pressure * 0.03, 0.1, TAU - 0.1); }

  const char* name() const override { return "Pressure Manometer"; };
  std::vector<Property> introspect() const override
  {
    return {
      Property{ "Pressure Reading", Type::Float, (void*)&pressure, true },
    };
  }

  float pressure = 0.0;
};

struct EHeatSink : Entity
{
  void tick() override
  {
    temperature = section->T;
  }

  const char* texture() const override { return "data/heatsink.png"; }
  const char* name() const override { return "Temperature Sensor"; };
  std::vector<Property> introspect() const override
  {
    return {
      Property{ "Temperature Reading", Type::Float, (void*)&temperature, true },
    };
  }

  float temperature = 0;
};

struct EFlowMeter : Entity
{
  void tick() override
  {
    flow = blend(0.1, flow, section->flux0);
    angle += flow * 0.05;

    if(angle > TAU)
      angle -= TAU;
  }

  const char* texture() const override { return "data/flowmeter.png"; }
  virtual const char* texture2() const { return "data/flowmeter_pin.png"; };
  virtual float texture2angle() const { return -angle; }
  const char* name() const override { return "Flow Meter"; };
  std::vector<Property> introspect() const override
  {
    return {
      Property{ "Flow Reading", Type::Float, (void*)&flow, true },
    };
  }

  float flow = 0;
  float angle = 0;
};

struct EHeatExchanger : Entity
{
  void tick() override
  {
    if(other)
    {
      double delta = other->section->T - section->T;
      delta *= 0.1;
      other->section->T -= delta;
      section->T += delta;
    }

    temperature = section->T;
  }

  Vec2f size() const override { return Vec2f(2, 1); }
  const char* texture() const override { return "data/heatexchanger.png"; }
  const char* name() const override { return "Heat Exchanger"; };
  std::vector<Property> introspect() const override
  {
    return {
      Property{ "Temperature Reading", Type::Float, (void*)&temperature, true },
    };
  }

  EHeatExchanger* other = nullptr;
  float temperature = 0.0;
};

struct EValve : Entity
{
  void tick() override
  {
    section->damping = open;
  }

  const char* texture() const override { return nullptr; }
  const char* texture2() const override { return "data/valve.png"; }
  float texture2angle() const override { return open * -8.0; }
  const char* name() const override { return "Valve"; };
  std::vector<Property> introspect() const override
  {
    return {
      Property{ "Opening ratio", Type::Float, (void*)&open },
    };
  }

  float open = 1.0;
};

template<typename T>
T* Spawn(std::unique_ptr<T> entity)
{
  T* r = entity.get();
  g_circuit.sections.push_back({});
  entity->section = &g_circuit.sections.back();
  entity->section->n = 1000; // put some water
  entity->section->T = 25; // room temperature
  g_entities.push_back(std::move(entity));
  return r;
}

void buildPrimaryCircuit(EHeatExchanger* HeatExchanger)
{
  auto MainPrimary = Spawn(std::make_unique<EPipe>());

  auto FlowMeter = Spawn(std::make_unique<EFlowMeter>());
  FlowMeter->id = "[Primary Circuit] Flow Meter";

  auto ColdPressure = Spawn(std::make_unique<EManometer>());
  ColdPressure->id = "[Primary Circuit] Cold Pressure";

  auto PreValve1 = Spawn(std::make_unique<EValve>());
  PreValve1->id = "[Primary Circuit] Main Pre-Valve 1";

  auto PreValve2 = Spawn(std::make_unique<EValve>());
  PreValve2->id = "[Primary Circuit] Main Pre-Valve 2";

  auto Pump1 = Spawn(std::make_unique<EPump>());
  Pump1->id = "[Primary Circuit] Pump 1";
  Pump1->powerRatio = 0.04;

  auto Pump2 = Spawn(std::make_unique<EPump>());
  Pump2->id = "[Primary Circuit] Pump 2";
  Pump2->powerRatio = 0.1;

  auto PostValve1 = Spawn(std::make_unique<EValve>());
  PostValve1->id = "[Primary Circuit] Main Post-Valve 1";

  auto PostValve2 = Spawn(std::make_unique<EValve>());
  PostValve2->id = "[Primary Circuit] Main Post-Valve 2";

  auto ColdHeatSensor = Spawn(std::make_unique<EHeatSink>());
  ColdHeatSensor->id = "[Primary Circuit] Cold Heat Sensor";

  auto ReactorCore = Spawn(std::make_unique<EReactor>());
  ReactorCore->id = "Reactor Core";
  ReactorCore->controlRods = 0;

  auto HotHeatSensor = Spawn(std::make_unique<EHeatSink>());
  HotHeatSensor->id = "[Primary Circuit] Hot Heat Sensor";

  auto HotPressure = Spawn(std::make_unique<EManometer>());
  HotPressure->id = "[Primary Circuit] Hot Pressure";

  auto HotFlow = Spawn(std::make_unique<EFlowMeter>());
  HotFlow->id = "[Primary Circuit] Hot Flow";

  // --------------------------------------

  const Vec2f origin = Vec2f(3, 6);
  MainPrimary->pos = Vec2f(4, 3) + origin;
  FlowMeter->pos = Vec2f(3, 3) + origin;
  ColdPressure->pos = Vec2f(2, 3) + origin;
  PreValve1->pos = Vec2f(1, 6) + origin;
  PreValve2->pos = Vec2f(1, 7) + origin;
  Pump1->pos = Vec2f(2, 6) + origin;
  Pump2->pos = Vec2f(2, 7) + origin;
  PostValve1->pos = Vec2f(3, 6) + origin;
  PostValve2->pos = Vec2f(3, 7) + origin;
  ColdHeatSensor->pos = Vec2f(5, 7) + origin;
  ReactorCore->pos = Vec2f(6, 5) + origin;
  HotHeatSensor->pos = Vec2f(8, 7) + origin;
  HotPressure->pos = Vec2f(9, 7) + origin;
  HotFlow->pos = Vec2f(10, 7) + origin;
  HeatExchanger->pos = Vec2f(6, 3) + origin;

  // --------------------------------------

  connect({
      MainPrimary,
      FlowMeter,
      ColdPressure,
      PreValve1,
      Pump1,
      PostValve1,
      ColdHeatSensor,
      ReactorCore,
      HotHeatSensor,
      HotPressure,
      HotFlow,
      HeatExchanger,
      MainPrimary });

  // redundant pump
  connect({
      ColdPressure,
      PreValve2,
      Pump2,
      PostValve2,
      ColdHeatSensor,
    });
}

void buildSecondaryCircuit(EHeatExchanger* HeatExchanger)
{
  auto Turbine = Spawn(std::make_unique<ETurbine>());
  Turbine->id = "Turbine #1";

  auto Generator = Spawn(std::make_unique<EGenerator>());
  Generator->turbine = Turbine;
  Generator->id = "Power Generator";

  auto FlowMeter = Spawn(std::make_unique<EFlowMeter>());
  FlowMeter->id = "[Secondary Circuit] Flow Meter";

  auto ColdPressure = Spawn(std::make_unique<EManometer>());
  ColdPressure->id = "[Secondary Circuit] Cold Pressure";

  auto PreValve1 = Spawn(std::make_unique<EValve>());
  PreValve1->id = "[Secondary Circuit] Main Pre-Valve 1";

  auto PreValve2 = Spawn(std::make_unique<EValve>());
  PreValve2->id = "[Secondary Circuit] Main Pre-Valve 2";

  auto Pump1 = Spawn(std::make_unique<EPump>());
  Pump1->id = "[Secondary Circuit] Pump 1";
  Pump1->powerRatio = 0.3;

  auto Pump2 = Spawn(std::make_unique<EPump>());
  Pump2->id = "[Secondary Circuit] Pump 2";
  Pump2->powerRatio = 0.6;

  auto PostValve1 = Spawn(std::make_unique<EValve>());
  PostValve1->id = "[Secondary Circuit] Main Post-Valve 1";

  auto PostValve2 = Spawn(std::make_unique<EValve>());
  PostValve2->id = "[Secondary Circuit] Main Post-Valve 2";

  auto ColdHeatSensor = Spawn(std::make_unique<EHeatSink>());
  ColdHeatSensor->id = "[Secondary Circuit] Cold Heat Sensor";

  auto HotHeatSensor = Spawn(std::make_unique<EHeatSink>());
  HotHeatSensor->id = "[Secondary Circuit] Hot Heat Sensor";

  auto HotPressure = Spawn(std::make_unique<EManometer>());
  HotPressure->id = "[Secondary Circuit] Hot Pressure";

  // --------------------------------------

  const Vec2f origin = Vec2f(3, 0);

  Turbine->pos = Vec2f(6, 2) + origin;
  Generator->pos = Vec2f(6, 1) + origin;
  FlowMeter->pos = Vec2f(3, 3) + origin;
  ColdPressure->pos = Vec2f(2, 3) + origin;
  PreValve1->pos = Vec2f(1, 6) + origin;
  PreValve2->pos = Vec2f(1, 7) + origin;
  Pump1->pos = Vec2f(2, 6) + origin;
  Pump2->pos = Vec2f(2, 7) + origin;
  PostValve1->pos = Vec2f(3, 6) + origin;
  PostValve2->pos = Vec2f(3, 7) + origin;
  ColdHeatSensor->pos = Vec2f(5, 7) + origin;
  HeatExchanger->pos = Vec2f(6, 7) + origin;
  HotHeatSensor->pos = Vec2f(8, 7) + origin;
  HotPressure->pos = Vec2f(9, 7) + origin;

  // --------------------------------------

  connect({ Turbine,
            FlowMeter,
            ColdPressure,
            PreValve1,
            Pump1,
            PostValve1,
            ColdHeatSensor,
            HeatExchanger,
            HotHeatSensor,
            HotPressure,
            Turbine });

  // redundant pump
  connect({
      ColdPressure,
      PreValve2,
      Pump2,
      PostValve2,
      ColdHeatSensor,
    });
}
}

void GameInit()
{
  g_finishMessage = nullptr;
  g_entities.clear();
  g_circuit = {};
  // never reallocate, we take pointers on elements
  g_circuit.sections.reserve(4096);

  auto PrimaryHeatExchanger = Spawn(std::make_unique<EHeatExchanger>());
  PrimaryHeatExchanger->id = "Primary Heat Exchanger";

  auto SecondaryHeatExchanger = Spawn(std::make_unique<EHeatExchanger>());
  SecondaryHeatExchanger->id = "Secondary Heat Exchanger";

  PrimaryHeatExchanger->other = SecondaryHeatExchanger;

  buildPrimaryCircuit(PrimaryHeatExchanger);
  buildSecondaryCircuit(SecondaryHeatExchanger);
}

void GameTick()
{
  simulate(g_circuit);

  for(auto& entity : g_entities)
    entity->tick();
}

const char* IsGameFinished()
{
  return g_finishMessage;
}

