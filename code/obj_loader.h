#ifndef OBJ_LOADER_H
#define OBJ_LOADER_H

struct obj_group;
struct obj_mtl_data;
struct loaded_obj_file;


loaded_obj_file* ReadOBJFile(thread_context* Thread, game_state* aGameState, 
			   debug_platform_read_entire_file* ReadEntireFile,
			   debug_platfrom_free_file_memory* FreeEntireFile,
			   char* FileName);

#endif OBJ_LOADER_H