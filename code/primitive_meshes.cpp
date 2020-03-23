#include "primitive_meshes.h"
#include "platform_opengl.h"
#include "math/vector_math.h"
#include "types.h"

internal void PushGLVertex( u32 Idx, v3 v, v3 n, v2 t, opengl_vertex* Data )
{
  opengl_vertex* Vert = Data+Idx;
  *Vert = {};
  Vert->v = v;
  Vert->vn = n;
  Vert->vt = t;
}

// Need 6 indeces, 4 vertices
internal void GetQuad( u32* Indeces, opengl_vertex* Data )
{
  Indeces[0] = 0;
  Indeces[1] = 1;
  Indeces[2] = 2;
  Indeces[3] = 0;
  Indeces[4] = 2;
  Indeces[5] = 3;
  PushGLVertex(0, V3(-0.5, -0.5, 0), V3( 0, 0, 1), V2( 0, 0), Data);
  PushGLVertex(1, V3( 0.5, -0.5, 0), V3( 0, 0, 1), V2( 1, 0), Data);
  PushGLVertex(2, V3( 0.5,  0.5, 0), V3( 0, 0, 1), V2( 1, 1), Data);
  PushGLVertex(3, V3(-0.5,  0.5, 0), V3( 0, 0, 1), V2( 0, 1), Data);
}

internal void GetVoxel( u32* Indeces, opengl_vertex* Data )
{
  Indeces[ 0] = 0;
  Indeces[ 1] = 1;
  Indeces[ 2] = 2;
  Indeces[ 3] = 2;
  Indeces[ 4] = 1;
  Indeces[ 5] = 3;
  Indeces[ 6] = 4;
  Indeces[ 7] = 5;
  Indeces[ 8] = 6;
  Indeces[ 9] = 6;
  Indeces[10] = 5;
  Indeces[11] = 7;
  Indeces[12] = 8;
  Indeces[13] = 9;
  Indeces[14] =10;
  Indeces[15] =10;
  Indeces[16] = 9;
  Indeces[17] =11;
  Indeces[18] =12;
  Indeces[19] =13;
  Indeces[20] =14;
  Indeces[21] =14;
  Indeces[22] =13;
  Indeces[23] =15;
  Indeces[24] =16;
  Indeces[25] =17;
  Indeces[26] =18;
  Indeces[27] =18;
  Indeces[28] =17;
  Indeces[29] =19;
  Indeces[30] =20;
  Indeces[31] =21;
  Indeces[32] =22;
  Indeces[33] =22;
  Indeces[34] =21;
  Indeces[35] =23;
  PushGLVertex( 0, V3(-0.5, -0.5,  0.5), V3( 0.0,  0.0,  1.0), V2( 0.0,  0.0 ), Data);
  PushGLVertex( 1, V3( 0.5, -0.5,  0.5), V3( 0.0,  0.0,  1.0), V2( 1.0,  0.0 ), Data);
  PushGLVertex( 2, V3(-0.5,  0.5,  0.5), V3( 0.0,  0.0,  1.0), V2( 0.0,  1.0 ), Data);
  PushGLVertex( 3, V3( 0.5,  0.5,  0.5), V3( 0.0,  0.0,  1.0), V2( 1.0,  1.0 ), Data);
  PushGLVertex( 4, V3(-0.5,  0.5,  0.5), V3( 0.0,  1.0,  0.0), V2( 0.0,  1.0 ), Data);
  PushGLVertex( 5, V3( 0.5,  0.5,  0.5), V3( 0.0,  1.0,  0.0), V2( 1.0,  1.0 ), Data);
  PushGLVertex( 6, V3(-0.5,  0.5, -0.5), V3( 0.0,  1.0,  0.0), V2( 0.0,  1.0 ), Data);
  PushGLVertex( 7, V3( 0.5,  0.5, -0.5), V3( 0.0,  1.0,  0.0), V2( 1.0,  1.0 ), Data);
  PushGLVertex( 8, V3(-0.5,  0.5, -0.5), V3( 0.0,  0.0, -1.0), V2( 0.0,  1.0 ), Data);
  PushGLVertex( 9, V3( 0.5,  0.5, -0.5), V3( 0.0,  0.0, -1.0), V2( 1.0,  1.0 ), Data);
  PushGLVertex(10, V3(-0.5, -0.5, -0.5), V3( 0.0,  0.0, -1.0), V2( 0.0,  0.0 ), Data);
  PushGLVertex(11, V3( 0.5, -0.5, -0.5), V3( 0.0,  0.0, -1.0), V2( 1.0,  0.0 ), Data);
  PushGLVertex(12, V3(-0.5, -0.5, -0.5), V3( 0.0, -1.0,  0.0), V2( 0.0,  0.0 ), Data);
  PushGLVertex(13, V3( 0.5, -0.5, -0.5), V3( 0.0, -1.0,  0.0), V2( 1.0,  0.0 ), Data);
  PushGLVertex(14, V3(-0.5, -0.5,  0.5), V3( 0.0, -1.0,  0.0), V2( 0.0,  0.0 ), Data);
  PushGLVertex(15, V3( 0.5, -0.5,  0.5), V3( 0.0, -1.0,  0.0), V2( 1.0,  0.0 ), Data);
  PushGLVertex(16, V3( 0.5, -0.5,  0.5), V3( 1.0,  0.0,  0.0), V2( 1.0,  0.0 ), Data);
  PushGLVertex(17, V3( 0.5, -0.5, -0.5), V3( 1.0,  0.0,  0.0), V2( 1.0,  0.0 ), Data);
  PushGLVertex(18, V3( 0.5,  0.5,  0.5), V3( 1.0,  0.0,  0.0), V2( 1.0,  1.0 ), Data);
  PushGLVertex(19, V3( 0.5,  0.5, -0.5), V3( 1.0,  0.0,  0.0), V2( 1.0,  1.0 ), Data);
  PushGLVertex(20, V3(-0.5, -0.5, -0.5), V3(-1.0,  0.0,  0.0), V2( 0.0,  0.0 ), Data);
  PushGLVertex(21, V3(-0.5, -0.5,  0.5), V3(-1.0,  0.0,  0.0), V2( 0.0,  0.0 ), Data);
  PushGLVertex(22, V3(-0.5,  0.5, -0.5), V3(-1.0,  0.0,  0.0), V2( 0.0,  1.0 ), Data);
  PushGLVertex(23, V3(-0.5,  0.5,  0.5), V3(-1.0,  0.0,  0.0), V2( 0.0,  1.0 ), Data);
}

void getPrimitive( primitive_type Type, u32* Indeces, opengl_vertex* GLData )
{
  switch(Type)
  {
    case primitive_type::QUAD:  GetQuad(Indeces, GLData );  return;
    case primitive_type::VOXEL: GetVoxel(Indeces, GLData ); return;
  }
}