#ifndef COMPONENT_RENDER_H
#define COMPONENT_RENDER_H

// Note (Jakob): component_mesh should contain all things needed for rendering a mesh. 
// 				 That Means Material, Triangles, Textures of all kinds, diffuse, normal, bump etc.
// 			     component_render_mesh is only used for rendering purposes and can only have 1
// 				 Surface property each.

struct mesh_data
{
	u32 nv;		 // Nr Verices
	u32 nvn; 	 // Nr Vertice Normals
	u32 nvt; 	 // Nr Trxture Vertices 

	v3* v;		 // Vertices
	v3* vn;		 // Vertice Normals
	v2* vt; 	 // Texture Vertices
};

struct mesh_indeces
{
	u32 Count;
	u32* vi; 	// Vertex Indeces
	u32* ti;  	// Texture Indeces
	u32* ni;	// Normal Indeces
};

struct component_mesh
{
	u32 VAO;
	mesh_data* Data;
	mesh_indeces Indeces;
};

#endif // COMPONENT_RENDER_MESH_HPP