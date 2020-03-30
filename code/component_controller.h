#pragma once

struct entity;
struct game_controller_input;

enum ControllerType
{
  ControllerType_Hero,
  ControllerType_EpaGjkVisualizer,
  ControllerType_FlyingCamera,
};

struct component_controller
{
	game_controller_input* Controller;
  u32 Type;
};
