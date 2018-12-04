#ifndef OBJ_LOADER_H
#define OBJ_LOADER_H

#define OBJ_MAX_LINE_LENGTH 512
#define OBJ_MAX_WORD_LENGTH 64


struct obj_group;
struct obj_mtl_data;
struct loaded_obj_file
{
	u32 ObjectCount;
	obj_group* Objects;

	mesh_data* MeshData;

	obj_mtl_data* MaterialData;
};


loaded_obj_file* ReadOBJFile(thread_context* Thread, game_state* aGameState, 
			   debug_platform_read_entire_file* ReadEntireFile,
			   debug_platfrom_free_file_memory* FreeEntireFile,
			   char* FileName);
void SetMeshAndMaterialComponentFromObjFile( world* World, loaded_obj_file* ObjFile );

#endif OBJ_LOADER_H