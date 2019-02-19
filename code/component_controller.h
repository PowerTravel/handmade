#ifndef COMPONENT_CONTROLLER_H
#define COMPONENT_CONTROLLER_H

struct entity;
struct component_controller
{
	game_controller_input* Controller;
	void (*ControllerMappingFunction) ( entity* Entity );
};


#endif // COMPONENT_CONTROLLER_H