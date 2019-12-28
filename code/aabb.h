#pragma once

#include "vector_math.h"
#include "data_containers.h"

struct aabb2f
{
  v2 P0;
  v2 P1;
};

aabb2f AABB2f( v2 P0, v2 P1 )
{
  aabb2f Result = {P0,P1};
  return Result;
};

struct aabb3f
{
  v3 P0;
  v3 P1;
};

inline aabb3f AABB3f( v3 P0, v3 P1 )
{
  aabb3f Result = {};
  Result.P0 = P0;
  Result.P1 = P1;
  return Result;
};


typedef v3 aabb_feature_vertex;
struct aabb_feature_edge
{
  aabb_feature_vertex P0;
  aabb_feature_vertex P1;
};

union aabb_feature_face
{
  struct{
    aabb_feature_vertex P0,P1,P2,P3;
  };
  aabb_feature_vertex P[4];
};

//** aabb2f Feature Extraction Functions **//

// aabb2f has following features:
//  1  [F]aces
//  4  [E]dges
//  4  [V]ertices

/*
 *
 *                                        ^
 *                                        |
 *                                        |
 *                                        n2
 *
 *                                       E2
 *                V3_________________________________V2
 *                        |                               |
 *                        |                               |
 *                        |                               |
 *                        |                               |
 *                        |                               |
 *               <--n3 E3 |              F0               | E1  n1 -->
 *    Y                   |                               |
 *    |                   |                               |
 *    |                   |                               |
 *    |                   |                               |
 *    |                   |                               |
 *    |________ X       V0|_______________________________|V1
 *                                      E0
 *                                        n0
 *                                        |
 *                                        |
 *                                        v
 */

v3 GetAABB2fEdgeNormal( const u32 EdgeIndex )
{
  switch(EdgeIndex)
  {
    case 0:
    {
      return V3(0,-1,0);
    }break;
    case 1:
    {
      return V3(1,0,0);
    }break;
    case 2:
    {
      return V3(0,1,0);
    }break;
    case 3:
    {
      return V3(-1,0,0);
    }break;
    default:
    {
      INVALID_CODE_PATH;
    }break;
  }

  return {};
}

void GetAABBVertices( const aabb2f* AABB, aabb_feature_vertex* AABBVertices )
{
  AABBVertices[0] = V3( AABB->P0.X, AABB->P0.Y, 0 );
  AABBVertices[1] = V3( AABB->P1.X, AABB->P0.Y, 0 );
  AABBVertices[2] = V3( AABB->P1.X, AABB->P1.Y, 0 );
  AABBVertices[3] = V3( AABB->P0.X, AABB->P1.Y, 0 );
}

list<aabb_feature_vertex> GetAABBVertices( memory_arena* Arena, const aabb2f* AABB )
{
  aabb_feature_vertex Vertices[4] ={};
  GetAABBVertices( AABB, Vertices );

  list<aabb_feature_vertex> Result = list<aabb_feature_vertex>(Arena);
  Result.First();

  Result.InsertAfter( Vertices[0] );
  Result.InsertAfter( Vertices[1] );
  Result.InsertAfter( Vertices[2] );
  Result.InsertAfter( Vertices[3] );

  return Result;
}


list<aabb_feature_edge> GetAABBEdges( memory_arena* Arena, const aabb2f* AABB )
{
  aabb_feature_vertex Vertices[4] ={};
  GetAABBVertices( AABB, Vertices);

  aabb_feature_edge Edges = {};
  list<aabb_feature_edge> Result = list<aabb_feature_edge>(Arena);
  Result.First();

  // Z Negative (Back)
  Edges.P0 = Vertices[0];
  Edges.P1 = Vertices[1];
  Result.InsertAfter(Edges);

  Edges.P0 = Vertices[1];
  Edges.P1 = Vertices[2];
  Result.InsertAfter(Edges);

  Edges.P0 = Vertices[2];
  Edges.P1 = Vertices[3];
  Result.InsertAfter(Edges);

  Edges.P0 = Vertices[3];
  Edges.P1 = Vertices[0];
  Result.InsertAfter(Edges);

  return Result;
}

aabb_feature_face GetAABBFace( const aabb2f* AABB )
{
  aabb_feature_vertex Vertices[8] ={};
  GetAABBVertices( AABB, Vertices);

  aabb_feature_face Result = {};
  Result.P[0] = Vertices[0];
  Result.P[1] = Vertices[1];
  Result.P[2] = Vertices[2];
  Result.P[3] = Vertices[3];
  return Result;
}



//** AABB3 Feature Extraction Functions **//

// AABB3f has following features:
//  6  [F]aces
//  12 [E]dges
//  8  [V]ertices

/*
 *
 *
 *                              V3______________E2________________V2
 *                               /|                              /|
 *                              / |                             / |
 *                             /  |                            /  |
 *                         E11/   |         F3             E10/   |
 *                           /    |E3                        /    |--------F4
 *                          /     |                         /     |
 *                       V7/_______________________________/      | E1
 *                        |       |     E6                |V6     |
 *     Y                  |       |                       |    ------------F1
 *     |          F0 -----|       |                       |       |
 *     |                  |       |                       |       |
 *     |_____X            |    V0 |____________E0_________|_______| V1
 *    /                 E7|      /                        |      /
 *   /                    |     /       F5                |E5   /
 *  Z                     |    /                          |    /
 *                        | E8/                           |   /E9
 *                        |  /                            |  /
 *                        | /                             | /
 *                        |/______________________________|/
 *                       V4             E4 |               V5
 *                                         |
 *                                         F2
 *
 *
 */

// Going CCW in the x-y plane starting with Xmin, Ymin, Zmin
// Then again with Xmin, Ymin, Zmax

r32 GetWidth( const aabb3f* AABB )
{
  return Abs( AABB->P1.X - AABB->P0.X);
}

b32 HasWidth( const aabb3f* AABB )
{
  r32 Tol = 1E-5;
  b32 HasWidth  = GetWidth(AABB) > Tol;
  return HasWidth;
}
b32 LacksWidth( const aabb3f* AABB )
{
  return ! HasWidth(AABB);
}

r32 GetHeight( const aabb3f* AABB )
{
  return Abs( AABB->P1.Y - AABB->P0.Y);
}

b32 HasHeight( const aabb3f* AABB )
{
  r32 Tol = 1E-5;
  b32 HasHeight = GetHeight(AABB) > Tol;
  return HasHeight;
}
b32 LacksHeight( const aabb3f* AABB )
{
  return ! HasHeight(AABB);
}

r32 GetDepth( const aabb3f* AABB )
{
  return Abs( AABB->P1.Z - AABB->P0.Z);
}
b32 HasDepth( const aabb3f* AABB )
{
  r32 Tol = 1E-5;
  b32 HasDepth  = GetDepth(AABB) > Tol;
  return HasDepth;
}
b32 LacksDepth( const aabb3f* AABB )
{
  return !HasDepth(AABB);
}

void GetAABBVertices(const aabb3f* AABB, aabb_feature_vertex* AABBVertices)
{
  AABBVertices[0] = V3( AABB->P0.X, AABB->P0.Y, AABB->P0.Z);
  AABBVertices[1] = V3( AABB->P1.X, AABB->P0.Y, AABB->P0.Z);
  AABBVertices[2] = V3( AABB->P1.X, AABB->P1.Y, AABB->P0.Z);
  AABBVertices[3] = V3( AABB->P0.X, AABB->P1.Y, AABB->P0.Z);
  AABBVertices[4] = V3( AABB->P0.X, AABB->P0.Y, AABB->P1.Z);
  AABBVertices[5] = V3( AABB->P1.X, AABB->P0.Y, AABB->P1.Z);
  AABBVertices[6] = V3( AABB->P1.X, AABB->P1.Y, AABB->P1.Z);
  AABBVertices[7] = V3( AABB->P0.X, AABB->P1.Y, AABB->P1.Z);
}


list<aabb_feature_vertex> GetAABBVertices( memory_arena* Arena, aabb3f* AABB )
{
  aabb_feature_vertex Vertices[8] ={};
  GetAABBVertices( AABB, Vertices );

  list<aabb_feature_vertex> Result = list<aabb_feature_vertex>(Arena);
  Result.First();

  Result.InsertAfter( Vertices[0] );
  Result.InsertAfter( Vertices[1] );
  Result.InsertAfter( Vertices[2] );
  Result.InsertAfter( Vertices[3] );

  if(  HasDepth( AABB ) )
  {
    Result.InsertAfter( Vertices[4] );
    Result.InsertAfter( Vertices[5] );
    Result.InsertAfter( Vertices[6] );
    Result.InsertAfter( Vertices[7] );
  }
  return Result;
}


list<aabb_feature_edge> GetAABBEdges( memory_arena* Arena, aabb3f* AABB )
{
  aabb_feature_vertex Vertices[8] ={};
  GetAABBVertices( AABB, Vertices);

  aabb_feature_edge Edges = {};
  list<aabb_feature_edge> Result = list<aabb_feature_edge>(Arena);
  Result.First();

  // Z Negative (Back)
  Edges.P0 = Vertices[0];
  Edges.P1 = Vertices[1];
  Result.InsertAfter(Edges);

  Edges.P0 = Vertices[1];
  Edges.P1 = Vertices[2];
  Result.InsertAfter(Edges);

  Edges.P0 = Vertices[2];
  Edges.P1 = Vertices[3];
  Result.InsertAfter(Edges);

  Edges.P0 = Vertices[3];
  Edges.P1 = Vertices[0];
  Result.InsertAfter(Edges);

  if( HasDepth(AABB) )
  {
    // Z Positive (Front)
    Edges.P0 = Vertices[4];
    Edges.P1 = Vertices[5];
    Result.InsertAfter(Edges);

    Edges.P0 = Vertices[5];
    Edges.P1 = Vertices[6];
    Result.InsertAfter(Edges);

    Edges.P0 = Vertices[6];
    Edges.P1 = Vertices[7];
    Result.InsertAfter(Edges);

    Edges.P0 = Vertices[7];
    Edges.P1 = Vertices[4];
    Result.InsertAfter(Edges);

    // Linking Back To Front
    Edges.P0 = Vertices[0];
    Edges.P1 = Vertices[4];
    Result.InsertAfter(Edges);

    Edges.P0 = Vertices[1];
    Edges.P1 = Vertices[5];
    Result.InsertAfter(Edges);

    Edges.P0 = Vertices[2];
    Edges.P1 = Vertices[6];
    Result.InsertAfter(Edges);

    Edges.P0 = Vertices[3];
    Edges.P1 = Vertices[7];
    Result.InsertAfter(Edges);
  }

  return Result;
}

v3 GetFaceNormal( aabb_feature_face* Face )
{
  v3 Edge0 = Face->P1-Face->P0;
  v3 Edge1 = Face->P2-Face->P1;
  v3 Result = Normalize( CrossProduct( Edge0, Edge1 ) );

  return Result;
}


list<aabb_feature_face> GetAABBFaces( memory_arena* Arena, aabb3f* AABB )
{
  aabb_feature_vertex Vertices[8] = {};
  GetAABBVertices( AABB, Vertices );

  list<aabb_feature_face> Faces = list< aabb_feature_face >(Arena);

  aabb_feature_face Face = {};

  // Z Negative
  Face.P[0] = Vertices[1];
  Face.P[1] = Vertices[0];
  Face.P[2] = Vertices[3];
  Face.P[3] = Vertices[2];
  Faces.InsertAfter(Face);
  Assert( (GetFaceNormal( &Face ) * V3(0,0,-1) - 1 ) < 10E-4 );

  if( HasDepth(AABB) )
  {
    // X Negative
    Face.P[0] = Vertices[0];
    Face.P[1] = Vertices[4];
    Face.P[2] = Vertices[7];
    Face.P[3] = Vertices[3];
    Faces.InsertAfter(Face);
    Assert( (GetFaceNormal( &Face ) * V3(-1,0,0) - 1 ) < 10E-4 );

    // X Positive
    Face.P[0] = Vertices[5];
    Face.P[1] = Vertices[1];
    Face.P[2] = Vertices[2];
    Face.P[3] = Vertices[6];
    Faces.InsertAfter(Face);
    Assert( (GetFaceNormal( &Face ) * V3(1,0,0) - 1 ) < 10E-4 );

    // Y Negative
    Face.P[0] = Vertices[0];
    Face.P[1] = Vertices[1];
    Face.P[2] = Vertices[5];
    Face.P[3] = Vertices[4];
    Faces.InsertAfter(Face);
    Assert( (GetFaceNormal( &Face ) * V3(0,-1,0) - 1 ) < 10E-4 );

    // Y Positive
    Face.P[0] = Vertices[7];
    Face.P[1] = Vertices[6];
    Face.P[2] = Vertices[2];
    Face.P[3] = Vertices[3];
    Faces.InsertAfter(Face);
    Assert( (GetFaceNormal( &Face ) * V3(0,1,0) - 1 ) < 10E-4 );

    // Z Positive
    Face.P[0] = Vertices[4];
    Face.P[1] = Vertices[5];
    Face.P[2] = Vertices[6];
    Face.P[3] = Vertices[7];
    Faces.InsertAfter(Face);
    Assert( (GetFaceNormal( &Face ) * V3(0,0,1) - 1 ) < 10E-4 );
  }

  return Faces;
}

inline v3
GetAABBCenter( const aabb3f& AABB )
{
  return (AABB.P0 + AABB.P1) * 0.5;
}

inline v3
GetHalfSideLength( const aabb3f& AABB, const v3& CollisionEnvelope = {} )
{
  return (AABB.P1 - AABB.P0 + CollisionEnvelope) * 0.5;
}

inline v3
GetPenetrationDepth( const aabb3f& A, const aabb3f& B )
{
  v3 ACenter = GetAABBCenter(A);
  v3 ASide   = GetHalfSideLength(A);

  v3 BCenter = GetAABBCenter(B);
  v3 BSide   = GetHalfSideLength(B);

  v3 ContactSeparation = ASide + BSide;

  v3 ABSeparation = Abs( BCenter - ACenter);

  v3 PenetrationDepth = ContactSeparation - ABSeparation;

  return PenetrationDepth;
}

inline aabb3f
MergeAABB( const aabb3f& A, const aabb3f& B, v3 Envelope = {})
{
  aabb3f Result = {};

  Result.P0.X = Minimum( A.P0.X, B.P0.X) - Envelope.X;
  Result.P0.Y = Minimum( A.P0.Y, B.P0.Y) - Envelope.Y;
  Result.P0.Z = Minimum( A.P0.Z, B.P0.Z) - Envelope.Z;

  Result.P1.X = Maximum( A.P1.X, B.P1.X) + Envelope.X;
  Result.P1.Y = Maximum( A.P1.Y, B.P1.Y) + Envelope.Y;
  Result.P1.Z = Maximum( A.P1.Z, B.P1.Z) + Envelope.Z;

  return Result;
}


enum aabb_contact_type
{
  AABB_CONTACT_TYPE_SEPARATE,
  AABB_CONTACT_TYPE_RESTING,
  AABB_CONTACT_TYPE_PENETRATION
};

struct aabb_contact
{
  aabb_contact_type Type;
  v3 CollisionNormal;
  v3 PenetrationDepth;
};

aabb_contact AABBContact( aabb3f& A, aabb3f& B, v3 ContactTolerance = V3(1E-7, 1E-7, 1E-7) )
{
  aabb_contact Result = {};
  Result.PenetrationDepth  = GetPenetrationDepth( A, B );

  if( ( Result.PenetrationDepth.X < -ContactTolerance.X ) ||
    ( Result.PenetrationDepth.Y < -ContactTolerance.Y ) ||
    ( Result.PenetrationDepth.Z < -ContactTolerance.Z ) )
  {
    Result.Type = AABB_CONTACT_TYPE_SEPARATE;
    return Result;
  }

  if( ( Result.PenetrationDepth.X <= ContactTolerance.X ) &&
      ( Result.PenetrationDepth.Y <= ContactTolerance.Y ) &&
      ( Result.PenetrationDepth.Z <= ContactTolerance.Z ) )
  {
    Result.Type = AABB_CONTACT_TYPE_RESTING;
    return Result;
  }

  Result.Type = AABB_CONTACT_TYPE_PENETRATION;

  return Result;
}

bool AABBRestingContact(aabb3f& A, aabb3f& B, v3 ContactTolerance = V3(1E-7,1E-7,1E-7) )
{
  v3 ACenter = GetAABBCenter(A);
  v3 ASide   = GetHalfSideLength(A);

  v3 BCenter = GetAABBCenter(B);
  v3 BSide   = GetHalfSideLength(B);

  v3 ContactSeparation = ASide + BSide;

  v3 ABSeparation = Abs( BCenter - ACenter);

  v3 PenetrationDepth = ABSeparation - ContactSeparation;

  if( ( PenetrationDepth.X >= 0 ) && ( PenetrationDepth.X <= ContactTolerance.X ) &&
    ( PenetrationDepth.Y >= 0 ) && ( PenetrationDepth.Y <= ContactTolerance.Y ) &&
    ( PenetrationDepth.Z >= 0 ) && ( PenetrationDepth.Z <= ContactTolerance.Z ) )
  {
    return true;
  }

  return false;
}


// Sweep a in the direction of v against b, returns true & info if there was a hit
// ===================================================================
b32 SweeptAABB2D( const aabb2f& a, const aabb2f& b, const v2& aStep, r32& HitPercentage, v2& HitNormal )
{
  //Initialise out info
  HitNormal = V2(0,0);

  if( (aStep.X == 0) && (aStep.Y == 0) )
  {
    return false;
  }

  // Treat a as stationary, so invert Step to get relative velocity
  v2 bStep = -aStep;

  HitPercentage = 0.0f;   // The % of aStep where the boxes collide.
  r32 ExitPercentage = 1.0f;  // The % of aStep where the boxes exit exit echother if they could pass through.
  v2 overlapTime = V2(0,0);

  // A is traveling to the right relative B
  if( bStep.X < 0 )
  {
    // A is to the right of B ( They are separating )
    if( b.P1.X <= a.P0.X )
    {
      return false;
    }

    // Left edge of A is to the left of Bs right edge, potentially overlapping and closing in on eachother.
    if( b.P1.X > a.P0.X )
    {
      ExitPercentage = Minimum( (a.P0.X - b.P1.X) / bStep.X, ExitPercentage );
    }

    // A is to the left of B with no overlap and closing in on eachother.
    if( a.P1.X <= b.P0.X )
    {
      overlapTime.X = (a.P1.X - b.P0.X) / bStep.X;
      HitPercentage = Maximum(overlapTime.X, HitPercentage);
    }
  }
  // A is traveling to the left relative B
  else if( bStep.X > 0 )
  {
    // A is to the left of B ( They are separating )
    if( b.P0.X >= a.P1.X )
    {
      return false;
    }

    // Right edge of A is to the right of Bs left edge, potentially overlapping and closing in on eachother.
    if( a.P1.X > b.P0.X )
    {
      ExitPercentage = Minimum( (a.P1.X - b.P0.X) / bStep.X, ExitPercentage );
    }

    // A is to the right of B with no overlap and closing in on eachother.
    if( b.P1.X <= a.P0.X )
    {
      overlapTime.X = (a.P0.X - b.P1.X) / bStep.X;
      HitPercentage = Maximum(overlapTime.X, HitPercentage);
    }
  }

  if( HitPercentage > ExitPercentage )
  {
    return false;
  }

  //=================================

  // A is traveling up relative to B
  if( bStep.Y < 0 )
  {
    // A is above B and separating
    if( b.P1.Y <= a.P0.Y )
    {
      return false;
    }

    // Bottom of A is below top of B, potentially overlapping and closing in on eachother.
    if( b.P1.Y > a.P0.Y )
    {
      ExitPercentage = Minimum( (a.P0.Y - b.P1.Y) / bStep.Y, ExitPercentage );
    }

    // A is below B with no overlap and closing in on eachother.
    if( a.P1.Y <= b.P0.Y )
    {
      overlapTime.Y = (a.P1.Y - b.P0.Y) / bStep.Y;
      HitPercentage = Maximum(overlapTime.Y, HitPercentage);
    }
  }
  // A is traveling down relative to B
  else if( bStep.Y > 0 )
  {
    // A is below B and separating
    if( b.P0.Y >= a.P1.Y )
    {
      return false;
    }

    // Top of A is above bottom of B, potentially overlapping and closing in on eachother.
    if( a.P1.Y > b.P0.Y )
    {
      ExitPercentage = Minimum( (a.P1.Y - b.P0.Y) / bStep.Y, ExitPercentage );
    }

    // A is above B with no overlap and closing in on eachother.
    if( b.P1.Y <= a.P0.Y )
    {
      overlapTime.Y = (a.P0.Y - b.P1.Y) / bStep.Y;
      HitPercentage = Maximum(overlapTime.Y, HitPercentage);
    }
  }

  if( HitPercentage > ExitPercentage )
  {
    return false;
  }

  // Hit normal is along axis with the highest overlap time
  if( overlapTime.X > overlapTime.Y )
  {
    if( bStep.X  > 0 )
    {
      HitNormal = V2(1, 0);
    }else if(bStep.X  < 0){
      HitNormal = V2(-1, 0);
    }else{
      Assert(0);
    }
  }else if( overlapTime.X < overlapTime.Y ){
    if( bStep.Y  > 0 )
    {
      HitNormal = V2(0, 1);
    }else if( bStep.Y  < 0 ){
      HitNormal = V2(0, -1);
    }else{
      Assert(0);
    }
  }else{
    if( bStep.X  > 0 )
    {
      HitNormal = V2(1, 0);
    }else if(bStep.X  < 0){
      HitNormal = V2(-1, 0);
    }else if( bStep.Y  > 0 ){
      HitNormal = V2(0, 1);
    }else if( bStep.Y  < 0 ){
      HitNormal = V2(0, -1);
    }else{
      Assert(0);
    }
  }

  return true;
}

b32 SweeptAABB( aabb3f& a, aabb3f& b, v3& Step, r32& HitPercentage, v3& HitNormal )
{
  v2 HitNomral2D = {};
  b32 Result = SweeptAABB2D( AABB2f(V2(a.P0), V2(a.P1)), AABB2f(V2(b.P0), V2(b.P1)), V2(Step),
                             HitPercentage, HitNomral2D );
  HitNormal = V3(HitNomral2D);
  return Result;
}