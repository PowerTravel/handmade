#pragma once

#include "vector_math.h"

// Requires spatial and collision primitive
struct component_dynamics
{
  v3  LinearVelocity;
  v3  AngularVelocity;
  v3  ExternalForce;
  r32 Mass;
};