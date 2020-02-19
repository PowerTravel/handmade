#pragma once
struct entity;
struct component_controller
{
	game_controller_input* Controller;
	void (*ControllerMappingFunction) ( entity* Entity );
};
