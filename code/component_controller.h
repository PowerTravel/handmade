#pragma once

struct entity;
struct game_controller_input;

struct component_controller
{
	game_controller_input* Controller;
	void (*ControllerMappingFunction) ( entity* Entity );
};
