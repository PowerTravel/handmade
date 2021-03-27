#pragma once

#include "vector_math.h"

inline m4
AffineInverse( const m4& A )
{
//  Exploit that A is affine:
//
//  A = [ M   b  ]
//      [ 0   1  ]
//
//  where A is 4x4, M is 3x3, b is 3x1, and the bottom row is (0,0,0,1), then
//
//  inv(A) = [ inv(M)   -inv(M) * b ]
//           [   0            1     ]

  // Assert that matrix is affine and x is a point
  Assert( A.E[12] == 0 );
  Assert( A.E[13] == 0 );
  Assert( A.E[14] == 0 );
  Assert( A.E[15] == 1 );

  r32 Det = A.E[0] * (A.E[ 5] * A.E[10] - A.E[ 9] * A.E[ 6]) +
            A.E[1] * (A.E[ 8] * A.E[ 6] - A.E[ 4] * A.E[10]) +
            A.E[2] * (A.E[ 4] * A.E[ 9] - A.E[ 8] * A.E[ 5]);

  Assert( Abs(Det) > 0.000001 );

  m4 Adj = M4( A.E[ 5] * A.E[10] - A.E[ 6] * A.E[ 9], A.E[ 2] * A.E[ 9] - A.E[10] * A.E[ 1],  A.E[ 1] * A.E[ 6] - A.E[ 2] * A.E[ 5], 0,
               A.E[ 6] * A.E[ 8] - A.E[10] * A.E[ 4], A.E[ 0] * A.E[10] - A.E[ 2] * A.E[ 8],  A.E[ 2] * A.E[ 4] - A.E[ 6] * A.E[ 0], 0,
               A.E[ 4] * A.E[ 9] - A.E[ 5] * A.E[ 8], A.E[ 1] * A.E[ 8] - A.E[ 9] * A.E[ 0],  A.E[ 0] * A.E[ 5] - A.E[ 1] * A.E[ 4], 0,
                               0,                                     0,                                      0,                   0);
  m4 Inv = ( 1 / Det ) * Adj;

  Inv.E[ 3] = -Inv.E[0]*A.E[3]-Inv.E[1]*A.E[7]-Inv.E[ 2]*A.E[11];
  Inv.E[ 7] = -Inv.E[4]*A.E[3]-Inv.E[5]*A.E[7]-Inv.E[ 6]*A.E[11];
  Inv.E[11] = -Inv.E[8]*A.E[3]-Inv.E[9]*A.E[7]-Inv.E[10]*A.E[11];
  Inv.E[15] = 1;

  return Inv;
}

inline m4
RigidInverse( const m4& A )
{
  m4 Result = M4(   A.E[0], A.E[4], A.E[ 8], -A.E[0] * A.E[3] - A.E[4] * A.E[7] - A.E[8]  * A.E[11],
                    A.E[1], A.E[5], A.E[ 9], -A.E[1] * A.E[3] - A.E[5] * A.E[7] - A.E[9]  * A.E[11],
                    A.E[2], A.E[6], A.E[10], -A.E[2] * A.E[3] - A.E[6] * A.E[7] - A.E[10] * A.E[11],
                         0,      0,       0,                           1);

  return Result;
}

inline v4
QuaternionConjugate( const v4 Q )
{
  v4 Result = V4( -Q.X, -Q.Y, -Q.Z, Q.W );
  return Result;
}

inline v4
QuaternionInverse( const v4 Q )
{
  r32 QNormSquared = Q.X*Q.X + Q.Y*Q.Y + Q.Z*Q.Z + Q.W*Q.W;
  Assert(QNormSquared > 10E-10);
  v4 QConjugate = QuaternionConjugate(Q);
  return (QConjugate/QNormSquared);
}

inline v4
QuaternionFromMatrix(const m4& M)
{
  // Note(Jakob): Untested function, Quaternion representation may be scewy
  // Q1 = (s,x,y,z) vs Q2 = (x,y,z,s). Q2 is what we should be using
  r32 qw = Sqrt(1.f + Index(M,0,0) + Index(M,1,1) + Index(M,2,2)) * 0.5f;
  r32 qx = (Index(M,2,1) - Index(M,1,2))/(4*qw);
  r32 qy = (Index(M,0,2) - Index(M,2,0))/(4*qw);
  r32 qz = (Index(M,1,0) - Index(M,0,1))/(4*qw);
  v4 Result = V4(qx,qy,qz,qw);
  return (Result);
}

inline m4
QuaternionAsMatrix( const v4& Quaternion )
{
  r32 x,y,z;

  x = 1-2*(Quaternion.Y*Quaternion.Y + Quaternion.Z*Quaternion.Z);
  y = 2*(Quaternion.X*Quaternion.Y - Quaternion.Z*Quaternion.W);
  z = 2*(Quaternion.X*Quaternion.Z + Quaternion.Y*Quaternion.W);
  v4 r_1 = V4(x,y,z,0.0f);

  x = 2*(Quaternion.X*Quaternion.Y + Quaternion.Z*Quaternion.W);
  y = 1-2*(Quaternion.X*Quaternion.X + Quaternion.Z*Quaternion.Z);
  z = 2*(Quaternion.Y*Quaternion.Z - Quaternion.X*Quaternion.W);
  v4 r_2 = V4(x,y,z,0.0f);

  x = 2*(Quaternion.X*Quaternion.Z - Quaternion.Y*Quaternion.W);
  y = 2*(Quaternion.Y*Quaternion.Z + Quaternion.X*Quaternion.W);
  z = 1-2*(Quaternion.X*Quaternion.X + Quaternion.Y*Quaternion.Y);
  v4 r_3 = V4(x,y,z,0.0f);

  v4 r_4 = V4(0.0f,0.0f,0.0f,1.0f);

  return M4(r_1,r_2,r_3,r_4);
}

inline v4 VectorToPureQuaternion(const v3& Vector)
{
  v4 Result = V4(Vector,0);
  return Result;
}

inline v3
RotateQuaternion( const v4& Quaternion, const v3& Vector)
{
  v4 PureQuaternion = VectorToPureQuaternion(Vector);
  v4 Result = QuaternionMultiplication(PureQuaternion, Quaternion);
  Result = QuaternionMultiplication( QuaternionConjugate(Quaternion), Result);
  return V3(Result);
}

inline v4
RotateQuaternion( const r32 Angle, const v4& Axis )
{
  const r32 epsilon = 0.0000001f;

  r32 length = Norm( Axis );

  if( length < epsilon)
  {
    // ~zero Axis, so reset rotation to zero
    return V4(0,0,0,1);
  }

  r32 inversenorm = 1/length;
  r32 coshalfangle = Cos( (r32) 0.5 * Angle);
  r32 sinhalfangle = Sin( (r32) 0.5 * Angle);

  v4 Result = V4( Axis.X * sinhalfangle * inversenorm,
                  Axis.Y * sinhalfangle * inversenorm,
                  Axis.Z * sinhalfangle * inversenorm,
                  coshalfangle);

  return Result;
}

inline v4
GetRotation( const v3 & From, const v3 & To )
{
  v3 FromDir = Normalize(From);
  v3 ToDir = Normalize(To);
  r32 CosAngle = FromDir*ToDir;
  r32 Angle = ACos(CosAngle);
  v3 RotationAxis = CrossProduct(FromDir, ToDir);
  v4 Quaternion = RotateQuaternion( Angle, V4(RotationAxis,0) );
  return Quaternion;
}

inline v4
RotateQuaternion( const r32 Angle, const v3& Axis )
{
  return RotateQuaternion( Angle, V4(Axis,0) );
}

inline m4
GetRotationMatrix( const v4& Quaternion )
{
  m4 R = QuaternionAsMatrix( Quaternion );
  return R;
}

inline m4
GetRotationMatrix( const r32 Angle, const v4& axis)
{
  v4 Quaternion  = RotateQuaternion( Angle, axis );
  m4 R = QuaternionAsMatrix( Quaternion );

  return R;
}

inline void
Translate( const v4& Translation, m4& M )
{
  M.E[3]  += Translation.X;
  M.E[7]  += Translation.Y;
  M.E[11] += Translation.Z;
}

inline m4
GetTranslationMatrix( const v4& Translation )
{
  m4 Result = M4Identity();
  Translate( Translation, Result );
  return Result;
}

inline m4
GetTranslationMatrix( const v3& Translation )
{
  return GetTranslationMatrix( V4(Translation,1) );
}


inline v4
GetTranslationFromMatrix( const m4& M )
{
  v4 Result = V4( M.r0.E[3],
                  M.r1.E[3],
                  M.r2.E[3],1);
  return Result;
}

inline void
SetPositionToAffineMatrix( const v4& Position, m4& M )
{
  M.E[3]  = Position.X;
  M.E[7]  = Position.Y;
  M.E[11] = Position.Z;
}

inline void
Rotate( const r32 Angle, const v4& Axis, m4& T )
{
  const m4 RotMat = GetRotationMatrix( Angle, Axis );

  const m4 tto = M4(1,0,0,-T.E[3],
                    0,1,0,-T.E[7],
                    0,0,1,-T.E[11],
                    0,0,0, 1);

  const m4 bfo = M4(1,0,0, T.E[3],
                    0,1,0, T.E[7],
                    0,0,1, T.E[11],
                    0,0,0, 1);

  T = bfo * RotMat * tto * T;
}

inline m4
GetScaleMatrix( const v4& Scale )
{
  m4 ScaleMat = M4Identity();
  ScaleMat.E[0]  = Scale.X;
  ScaleMat.E[5]  = Scale.Y;
  ScaleMat.E[10] = Scale.Z;
  return ScaleMat;
}

inline void
Scale( const v4& Scale, m4& T )
{
  m4 ScaleMat = GetScaleMatrix( Scale );

  m4 tto = M4(1,0,0,-T.E[3],
              0,1,0,-T.E[7],
              0,0,1,-T.E[11],
              0,0,0,1);

  m4 bfo = M4(1,0,0, T.E[3],
              0,1,0, T.E[7],
              0,0,1, T.E[11],
              0,0,0,1);

  T =  bfo * ScaleMat * tto * T;
}

inline void
ScaleAroundOrigin( const v4& Scale, m4& T )
{
  T.E[0]  *= Scale.X;
  T.E[5]  *= Scale.Y;
  T.E[10] *= Scale.Z;
}

inline v4
PointMultiply( const m4& M, const v4& b )
{
  v4 Result = M*b;

//  Assert(( Abs( Result.W ) > 10e-10 ));

  if( Result.W != 1 )
  {
    Result /= Result.W;
  }

  return Result;
}