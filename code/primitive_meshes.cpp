#include "primitive_meshes.h"
#include "platform_opengl.h"
#include "vector_math.h"
#include "types.h"

internal void PushGLVertex( u32 Idx, v3 v, v3 n, opengl_vertex* Data )
{
  opengl_vertex* Vert = Data+Idx;
  *Vert = {};
  Vert->v = v;
  Vert->vn = n;
}

internal void GetSquare( opengl_vertex* Data )
{
  PushGLVertex(0, V3(-0.5,  0.5, 0), V3( 0, 0,-1), Data);
  PushGLVertex(1, V3( 0.5,  0.5, 0), V3( 0, 0,-1), Data);
  PushGLVertex(2, V3(-0.5, -0.5, 0), V3( 0, 0,-1), Data);
  PushGLVertex(3, V3(-0.5, -0.5, 0), V3( 0, 0,-1), Data);
  PushGLVertex(4, V3( 0.5,  0.5, 0), V3( 0, 0,-1), Data);
  PushGLVertex(5, V3( 0.5, -0.5, 0), V3( 0, 0,-1), Data);
}

internal void GetBox( opengl_vertex* Data )
{
  PushGLVertex( 0, V3(-0.5, -0.5,  0.5), V3( 0, 0, 1), Data); /*1*/
  PushGLVertex( 1, V3( 0.5, -0.5,  0.5), V3( 0, 0, 1), Data); /*2*/
  PushGLVertex( 2, V3(-0.5,  0.5,  0.5), V3( 0, 0, 1), Data); /*3*/
  PushGLVertex( 3, V3(-0.5,  0.5,  0.5), V3( 0, 0, 1), Data); /*3*/
  PushGLVertex( 4, V3( 0.5, -0.5,  0.5), V3( 0, 0, 1), Data); /*2*/
  PushGLVertex( 5, V3( 0.5,  0.5,  0.5), V3( 0, 0, 1), Data); /*4*/
  PushGLVertex( 6, V3(-0.5,  0.5,  0.5), V3( 0, 1, 0), Data); /*3*/
  PushGLVertex( 7, V3( 0.5,  0.5,  0.5), V3( 0, 1, 0), Data); /*4*/
  PushGLVertex( 8, V3(-0.5,  0.5, -0.5), V3( 0, 1, 0), Data); /*5*/
  PushGLVertex( 9, V3(-0.5,  0.5, -0.5), V3( 0, 1, 0), Data); /*5*/
  PushGLVertex(10, V3( 0.5,  0.5,  0.5), V3( 0, 1, 0), Data); /*4*/
  PushGLVertex(11, V3( 0.5,  0.5, -0.5), V3( 0, 1, 0), Data); /*6*/
  PushGLVertex(12, V3(-0.5,  0.5, -0.5), V3( 0, 0,-1), Data); /*5*/
  PushGLVertex(13, V3( 0.5,  0.5, -0.5), V3( 0, 0,-1), Data); /*6*/
  PushGLVertex(14, V3(-0.5, -0.5, -0.5), V3( 0, 0,-1), Data); /*7*/
  PushGLVertex(15, V3(-0.5, -0.5, -0.5), V3( 0, 0,-1), Data); /*7*/
  PushGLVertex(16, V3( 0.5,  0.5, -0.5), V3( 0, 0,-1), Data); /*6*/
  PushGLVertex(17, V3( 0.5, -0.5, -0.5), V3( 0, 0,-1), Data); /*8*/
  PushGLVertex(18, V3(-0.5, -0.5, -0.5), V3( 0,-1, 0), Data); /*7*/
  PushGLVertex(19, V3( 0.5, -0.5, -0.5), V3( 0,-1, 0), Data); /*8*/
  PushGLVertex(20, V3(-0.5, -0.5,  0.5), V3( 0,-1, 0), Data); /*1*/
  PushGLVertex(21, V3(-0.5, -0.5,  0.5), V3( 0,-1, 0), Data); /*1*/
  PushGLVertex(22, V3( 0.5, -0.5, -0.5), V3( 0,-1, 0), Data); /*8*/
  PushGLVertex(23, V3( 0.5, -0.5,  0.5), V3( 0,-1, 0), Data); /*2*/
  PushGLVertex(24, V3( 0.5, -0.5,  0.5), V3( 1, 0, 0), Data); /*2*/
  PushGLVertex(25, V3( 0.5, -0.5, -0.5), V3( 1, 0, 0), Data); /*8*/
  PushGLVertex(26, V3( 0.5,  0.5,  0.5), V3( 1, 0, 0), Data); /*4*/
  PushGLVertex(27, V3( 0.5,  0.5,  0.5), V3( 1, 0, 0), Data); /*4*/
  PushGLVertex(28, V3( 0.5, -0.5, -0.5), V3( 1, 0, 0), Data); /*8*/
  PushGLVertex(29, V3( 0.5,  0.5, -0.5), V3( 1, 0, 0), Data); /*6*/
  PushGLVertex(30, V3(-0.5, -0.5, -0.5), V3(-1, 0, 0), Data); /*7*/
  PushGLVertex(31, V3(-0.5, -0.5,  0.5), V3(-1, 0, 0), Data); /*1*/
  PushGLVertex(32, V3(-0.5,  0.5, -0.5), V3(-1, 0, 0), Data); /*5*/
  PushGLVertex(33, V3(-0.5,  0.5, -0.5), V3(-1, 0, 0), Data); /*5*/
  PushGLVertex(34, V3(-0.5, -0.5,  0.5), V3(-1, 0, 0), Data); /*1*/
  PushGLVertex(35, V3(-0.5,  0.5,  0.5), V3(-1, 0, 0), Data); /*3*/

/*
1  V3(-0.5, -0.5,  0.5),
2  V3( 0.5, -0.5,  0.5),
3  V3(-0.5,  0.5,  0.5),
4  V3( 0.5,  0.5,  0.5),
5  V3(-0.5,  0.5, -0.5),
6  V3( 0.5,  0.5, -0.5),
7  V3(-0.5, -0.5, -0.5),
8  V3( 0.5, -0.5, -0.5),

1  V3( 0, 0, 1),
2  V3( 0, 1, 0),
3  V3( 0, 0,-1),
4  V3( 0,-1, 0),
5  V3( 1, 0, 0),
6  V3(-1, 0, 0),

s 1
f 1/1/1 2/2/1 3/3/1
f 3/3/1 2/2/1 4/4/1
s 2
f 3/2/2 4/5/2 5/4/2
f 5/4/2 4/5/2 6/6/2
s 3
f 5/5/3 6/7/3 7/6/3
f 7/6/3 6/7/3 8/8/3
s 4
f 7/7/4 8/9/4 1/8/4
f 1/8/4 8/9/4 2/10/4
s 5
f 2/3/5 8/4/5 4/12/5
f 4/12/5 8/4/5 6/13/5
s 6
f 7/4/6 1/6/6 5/13/6
f 5/13/6 1/6/6 3/14/6
*/
}

void getPrimitive( primitive_type Type, opengl_vertex* GLData )
{
  switch(Type)
  {
    case primitive_type::SQUARE: GetSquare( GLData ); return;
    case primitive_type::BOX:    GetBox( GLData );    return;
  }
}