#pragma once

#include "vector_math.h"

// Requires spatial and collision primitive
struct component_dynamics
{
  v3  Velocity;
  v3  ExternalForce;
  r32 Mass;
};