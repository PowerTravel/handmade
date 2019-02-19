#ifndef MESH_H
#define MESH_H

typedef v3 point;
struct triangle;

struct vertex
{
 	triangle* root;
	point p;
};

struct triangle
{
	triangle* t[3];
	vertex* v[3];
};

struct ordered_mesh
{
	u32 TriangleCount;
	triangle* Triangles;

	u32 VertexCount;
	vertex* Vertecis;
};

u32 cw( u32 i ) {return ( (i+2)%3 ); }
u32 ccw( u32 i ){return ( (i+1)%3 ); }

vertex* GerVertex( triangle* t, u32 i )
{
	return t->v[i%3];
}

triangle* GetFace( vertex* v )
{
	return v->root;
}

u32 GetVertexIndex( vertex* v, triangle* t )
{
	for( u32 i = 0; i < 3; ++i )
	{
		if( v == t->v[i] )
		{
			return i;
		}
	}
	return 4;
}

u32 GetFaceIndex( triangle* IndexFrom, triangle* IndexOf )
{
	for (int i = 0; i < 3; ++i)
	{
		triangle* test = IndexFrom->t[i];
		if( (test->v[0]->p == IndexOf->v[0]->p) && 
			(test->v[1]->p == IndexOf->v[1]->p) && 
			(test->v[2]->p == IndexOf->v[2]->p) )
		{
			return i;
		}
	}
	return 4;
}


triangle* GetNeightbour( triangle* t, u32 i )
{
	return t->t[i%3];
}

#endif