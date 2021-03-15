#pragma once

#include "vector_math.h"
#include "string.h"

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

enum class aabb_metric{
  X_LENGTH,
  Y_LENGTH,
  Z_LENGTH,
  X_AREA,
  Y_AREA,
  Z_AREA,
  VOLUME
};


memory_index AABBToString(aabb3f* AABB, memory_index DestCount, char* const DestString);
r32 GetSize( const aabb3f* AABB, const aabb_metric Metric = aabb_metric::VOLUME );
void GetAABBVertices(const aabb3f* AABB, v3* AABBVertices, u32* VerticeIndeces );
inline v3 GetAABBCenter( const aabb3f& AABB );
inline aabb3f MergeAABB( const aabb3f& A, const aabb3f& B, const v3& Envelope = {});
b32 AABBIntersects( const v3* P, const aabb3f* A);
b32 AABBIntersects( const aabb3f* A, const aabb3f* B);
b32 SweeptAABB2D( const aabb2f& a, const aabb2f& b, const v2& aStep, r32& HitPercentage, v2& HitNormal );
b32 SweeptAABB( aabb3f& a, aabb3f& b, v3& Step, r32& HitPercentage, v3& HitNormal );
aabb3f TransformAABB( const aabb3f& AABB, const m4& TransMat );
b32 AABBRay( v3 const & RayOrigin, v3 const & Direction, aabb3f const & AABB, r32* tMin, r32* tMax);