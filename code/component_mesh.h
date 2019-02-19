#ifndef COMPONENT_RENDER_H
#define COMPONENT_RENDER_H

// Note (Jakob): component_render_mesh should contain all things needed for rendering a mesh. 
// 				 That Means Material, Triangles, Textures of all kinds, diffuse, normal, bump etc.
// 			     component_render_mesh is only used for rendering purposes and can only have 1
// 				 Surface property each.

struct face
{
	u32  nv;	// Nr Vertices
	u32* vi; 	// Vertex Indeces
	u32* ni;	// Normal Indeces
	u32* ti;  	// Texture Indeces
};

struct mesh_data
{
	u32 nv;		 // Nr Verices
	u32 nvn; 	 // Nr Vertice Normals
	u32 nvt; 	 // Nr Trxture Vertices 

	v4* v;		 // Vertices
	v4* vn;		 // Vertice Normals
	v3* vt; 	 // Texture Vertices
};

struct component_mesh
{
	// Geometric object
	u32   TriangleCount;
	face* Triangles;

	mesh_data* Data;
};

#endif // COMPONENT_RENDER_MESH_HPP