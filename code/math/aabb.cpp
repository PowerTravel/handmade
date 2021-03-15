#include "aabb.h"

memory_index AABBToString(aabb3f* AABB, memory_index DestCount, char* const DestString)
{
  char* Scanner = DestString;
  memory_index Len = str::ToString( AABB->P0, 2, DestCount, Scanner);
  Scanner+=Len;
  *Scanner++ = ' ';

  Len = str::ToString( AABB->P1, 2, DestCount, Scanner);
  Scanner+=Len;
  *Scanner = '\0';

  return Scanner - DestString;
}

r32 GetSize( const aabb3f* AABB, const aabb_metric Metric )
{
  switch(Metric)
  {
    case aabb_metric::VOLUME:
    {
      return Abs(AABB->P1.X - AABB->P0.X) * Abs(AABB->P1.Y - AABB->P0.Y) * Abs(AABB->P1.Z - AABB->P0.Z);
    }break;
    case aabb_metric::X_LENGTH:
    {
      return Abs(AABB->P1.X - AABB->P0.X);
    }break;
    case aabb_metric::Y_LENGTH:
    {
      return Abs(AABB->P1.Y - AABB->P0.Y);
    }break;
    case aabb_metric::Z_LENGTH:
    {
      return Abs(AABB->P1.Z - AABB->P0.Z);
    }break;
    case aabb_metric::X_AREA:
    {
      return Abs(AABB->P1.Y - AABB->P0.Y) * Abs(AABB->P1.Z - AABB->P0.Z);
    }break;
    case aabb_metric::Y_AREA:
    {
      return Abs(AABB->P1.X - AABB->P0.X) * Abs(AABB->P1.Z - AABB->P0.Z);
    }break;
    case aabb_metric::Z_AREA:
    {
      return Abs(AABB->P1.X - AABB->P0.X) * Abs(AABB->P1.Y - AABB->P0.Y);
    }break;
  }
  return 0;
}

void GetAABBVertices(const aabb3f* AABB, v3* AABBVertices, u32* VerticeIndeces = 0 )
{
  AABBVertices[0] = V3( AABB->P0.X, AABB->P0.Y, AABB->P0.Z);
  AABBVertices[1] = V3( AABB->P1.X, AABB->P0.Y, AABB->P0.Z);
  AABBVertices[2] = V3( AABB->P1.X, AABB->P1.Y, AABB->P0.Z);
  AABBVertices[3] = V3( AABB->P0.X, AABB->P1.Y, AABB->P0.Z);
  AABBVertices[4] = V3( AABB->P0.X, AABB->P0.Y, AABB->P1.Z);
  AABBVertices[5] = V3( AABB->P1.X, AABB->P0.Y, AABB->P1.Z);
  AABBVertices[6] = V3( AABB->P1.X, AABB->P1.Y, AABB->P1.Z);
  AABBVertices[7] = V3( AABB->P0.X, AABB->P1.Y, AABB->P1.Z);

 if(VerticeIndeces)
 {
    // Z Negative
    VerticeIndeces[0] = 1;
    VerticeIndeces[1] = 0;
    VerticeIndeces[2] = 3;

    VerticeIndeces[3] = 1;
    VerticeIndeces[4] = 3;
    VerticeIndeces[5] = 2;

    // Z Positive
    VerticeIndeces[6] = 4;
    VerticeIndeces[7] = 5;
    VerticeIndeces[8] = 6;

    VerticeIndeces[9] = 4;
    VerticeIndeces[10] = 6;
    VerticeIndeces[11] = 7;

    // X Negative
    VerticeIndeces[12] = 0;
    VerticeIndeces[13] = 4;
    VerticeIndeces[14] = 7;

    VerticeIndeces[15] = 0;
    VerticeIndeces[16] = 7;
    VerticeIndeces[17] = 3;

    // X Positive
    VerticeIndeces[18] = 5;
    VerticeIndeces[19] = 1;
    VerticeIndeces[20] = 2;

    VerticeIndeces[21] = 5;
    VerticeIndeces[22] = 2;
    VerticeIndeces[23] = 6;

    // Y Negative
    VerticeIndeces[24] = 0;
    VerticeIndeces[25] = 1;
    VerticeIndeces[26] = 5;

    VerticeIndeces[27] = 0;
    VerticeIndeces[28] = 5;
    VerticeIndeces[29] = 4;

    // Y Positive
    VerticeIndeces[30] = 7;
    VerticeIndeces[31] = 6;
    VerticeIndeces[32] = 2;

    VerticeIndeces[33] = 7;
    VerticeIndeces[34] = 2;
    VerticeIndeces[35] = 3;

  }
}

inline v3
GetAABBCenter( const aabb3f& AABB )
{
  return (AABB.P0 + AABB.P1) * 0.5;
}

inline aabb3f
MergeAABB( const aabb3f& A, const aabb3f& B, const v3& Envelope)
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


b32 AABBIntersects( const v3* P, const aabb3f* A)
{
  if( (P->X < A->P0.X) || (P->X > A->P1.X) ||
      (P->Y < A->P0.Y) || (P->Y > A->P1.Y) ||
      (P->Z < A->P0.Z) || (P->Z > A->P1.Z) )
  {
    return false;
  }
  return true;
}

b32 AABBIntersects( const aabb3f* A, const aabb3f* B)
{
  if( (A->P1.X < B->P0.X) || (A->P0.X > B->P1.X) ||
      (A->P1.Y < B->P0.Y) || (A->P0.Y > B->P1.Y) ||
      (A->P1.Z < B->P0.Z) || (A->P0.Z > B->P1.Z) )
  {
    return false;
  }
  return true;
}

b32 AABBRay( v3 const & RayOrigin, v3 const & Direction, aabb3f const & AABB, r32* tMin, r32* tMax)
{
  // IEEE x/0 is defined to be:
  //    +inf if x > 0,
  //    -inf if x < 0,
  //    undefined if x == 0;
  r32 DirFracX = 1.0f/Direction.X;
  r32 DirFracY = 1.0f/Direction.Y;
  r32 DirFracZ = 1.0f/Direction.Z;

  r32 t1 = (AABB.P0.X - RayOrigin.X) * DirFracX;
  r32 t2 = (AABB.P1.X - RayOrigin.X) * DirFracX;
  r32 t3 = (AABB.P0.Y - RayOrigin.Y) * DirFracY;
  r32 t4 = (AABB.P1.Y - RayOrigin.Y) * DirFracY;
  r32 t5 = (AABB.P0.Z - RayOrigin.Z) * DirFracZ;
  r32 t6 = (AABB.P1.Z - RayOrigin.Z) * DirFracZ;

  *tMin = Maximum(Maximum( Minimum(t1, t2), Minimum(t3,t4) ), Minimum(t5,t6));
  *tMax = Minimum(Minimum( Maximum(t1, t2), Maximum(t3,t4) ), Maximum(t5,t6));

#if 0
  if (tMax < 0)
  {
    t = tMax;
    return false;
  }

  if (tMin > tMax)
  {
    t = tMax;
    return false;
  }

  t = tMin;
  return true;
#else

  b32 Result = (*tMax >= 0) && (*tMin <= *tMax);
  return Result;
#endif
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

aabb3f TransformAABB( const aabb3f& AABB, const m4& TransMat )
{
  v3 Min = V3( TransMat * V4(AABB.P0,1));
  v3 Max = Min;
  v3 AABBVertices[8] = {};
  GetAABBVertices(&AABB, AABBVertices);
  for( u32 Index = 0; Index < ArrayCount(AABBVertices); ++Index )
  {
    const v3 Point = V3( TransMat * V4(AABBVertices[Index], 1) );

    Min.X = (Point.X < Min.X) ? Point.X : Min.X;
    Min.Y = (Point.Y < Min.Y) ? Point.Y : Min.Y;
    Min.Z = (Point.Z < Min.Z) ? Point.Z : Min.Z;

    Max.X = (Point.X > Max.X) ? Point.X : Max.X;
    Max.Y = (Point.Y > Max.Y) ? Point.Y : Max.Y;
    Max.Z = (Point.Z > Max.Z) ? Point.Z : Max.Z;
  }
  aabb3f Result = AABB3f(Min,Max);
  return Result;
}