#pragma once

#include "intrinsics.h"

union v2
{
  struct{
     r32 X, Y;
  };
   r32 E[2];
};

union v3
{
  struct{
     r32 X, Y, Z;
  };
   r32 E[3];
};

union v4
{
  struct{
     r32 X, Y, Z, W;
  };
  r32 E[4];
};

union m3
{
//  Row Dominant indexing
//   0,  1,  2,
//   3,  4,  5,
//   6,  7,  8,
  struct{
    v3 r0, r1, r2;
  };
   r32 E[9];
};

union m4
{
//  Row Dominant indexing
//   0,  1,  2,  3,
//   4,  5,  6,  7,
//   8,  9, 10, 11,
//  12, 13, 14, 15
  struct{
    v4 r0, r1, r2, r3;
  };
   r32 E[16];
};

inline b32
Equals(const r32 A, const r32 B, const r32 Tol = 10E-9 )
{
  return Abs(A-B) < Tol;
}

// Supported operations:
// v2 * v2 (dot)
// r * v2,
// v2 * r
// v2 *= r

// v2 / r
// v2 /= r

// v2 + v2,
// v2 += v2

// v2 - v2,
// v2 -= v2.

// r norm
// v2 normalize

// v2 Abs

// bool v2 == v2
// bool v2 !- v2

inline v2
V2( const r32& X, const r32& Y )
{
  v2 Result = {X,Y};
  return(Result);
};

inline v2
V2( const v3& R )
{
  v2 Result = {R.X,R.Y};
  return(Result);
};

inline v2
V2( const v4& R )
{
  v2 Result = {R.X,R.Y};
  return(Result);
};

inline r32
operator*( const v2& A, const v2& B )
{
   r32 Result = A.X*B.X + A.Y*B.Y;
  return(Result);
}

inline v2
operator*( const r32 s, const v2& A )
{
  v2 Result;
  Result.X = s*A.X;
  Result.Y = s*A.Y;
  return(Result);
}

inline v2
operator*( const v2& A, const r32 s )
{
  v2 Result;
  Result = s*A;
  return(Result);
}

inline v2&
operator*=( v2& A, const r32 s )
{
  A = s*A;
  return A;
}

inline v2
operator/( const v2& A, const r32 s )
{
  v2 Result;
  r32 inverse = 1/s;
  Result = inverse*A;
  return Result;
}

inline v2&
operator/=( v2& A, const r32 s )
{
  A = A/s;
  return A;
}

inline v2
operator+( const v2& A, const v2& B )
{
  v2 Result;
  Result.X = A.X + B.X;
  Result.Y = A.Y + B.Y;
  return Result;
}

inline v2&
operator+=( v2& A, const v2& B )
{
  A = A+B;
  return A;
}


inline v2
operator-( const v2& A, const v2& B )
{
  v2 Result;
  Result.X = A.X - B.X;
  Result.Y = A.Y - B.Y;
  return Result;
}

inline v2
operator-( const v2& A )
{
  v2 Result;
  Result.X = - A.X;
  Result.Y = - A.Y;
  return Result;
}

inline v2&
operator-=( v2& A, const v2& B )
{
  A = A-B;
  return A;
}

inline b32
operator==( const v2& A, const v2& B )
{
  return Equals( A.X, B.X ) && Equals(A.Y, B.Y );
}

inline b32
operator!=( const v2& A, const v2& B )
{
  return !( A == B );
}

inline r32
NormSq( const v2& A )
{
  r32 Result;
  Result = A*A;
  return Result;
}

inline r32
Norm( const v2& A )
{
   r32 Result;
  Result = Sqrt( A*A );
  return Result;
}

inline v2
Normalize( const v2& A )
{
  v2 Result;
  r32 N = Norm(A);
  Result = ( Abs(N) > 10E-7 ) ? A / N : V2(0,0);
  return Result;
}

inline r32
Determinant( const v2& Row0, const v2& Row1 )
{
  r32 Result = Row0.X * Row1.Y - Row0.Y * Row1.X;
  return Result;
}

inline v2
Abs( const v2& V )
{
  v2 Result = V2(Abs(V.X), Abs(V.Y) );
  return Result;
}

// Supported operations:
//  r32 v3 * v3 (dot)
// v3 =  r32 * v3,
// v3 = v3 *  r32
// v3 *=  r32

// v3 = v3 /  r32
// v3 /=  r32

// v3 = v3 + v3,
// v3 += v3

// v3 = v3 - v3,
// v3 = -v3
// v3 -= v3.

// r norm
// v3 = normalize
// v3 = Cross

// v3 Abs(v3)

// bool v3 == v3
// bool v3 !- v3


inline v3
V3( const r32 X, const r32 Y, const r32 Z )
{
  v3 Result = {X,Y,Z};
  return(Result);
};

inline v3
V3( const v2& A, const r32 Z=0 )
{
  v3 Result = {A.X,A.Y,Z};
  return(Result);
};

inline v3
V3( const v4& A )
{
  v3 Result = {A.X, A.Y, A.Z};
  return(Result);
};

inline r32
operator*( const v3& A, const v3& B )
{
  r32 Result = 0;
  Result = A.X*B.X + A.Y*B.Y + A.Z*B.Z;
  return Result;
}

inline v3
operator*( const r32 A, const v3& B )
{
  v3 Result = {};
  Result.X = A*B.X;
  Result.Y = A*B.Y;
  Result.Z = A*B.Z;
  return Result;
}

inline v3
operator*( const v3& B, const r32 s )
{
  v3 Result = {};
  Result = s*B;
  return Result;
}

inline v3&
operator*=( v3& V, const r32 s )
{
  V = s*V;
  return V;
}

inline v3
operator/( const v3& V, const r32 s )
{
  r32 sInv = 1/s;
  v3 Result = sInv*V;
  return Result;
}

inline v3&
operator/=( v3& V, const r32 s )
{
  V = V / s;
  return V;
}

inline v3
operator+( const v3& A, const v3& B )
{
  v3 Result = {};
  Result.X = A.X + B.X;
  Result.Y = A.Y + B.Y;
  Result.Z = A.Z + B.Z;
  return Result;
}

inline v3&
operator+=( v3& A, const v3& B )
{
  A = A+B;
  return A;
}

inline v3
operator-( const v3& A, const v3& B )
{
  v3 Result;
  Result.X = A.X - B.X;
  Result.Y = A.Y - B.Y;
  Result.Z = A.Z - B.Z;
  return Result;
}

inline v3
operator-( const v3& A )
{
  v3 Result = {};
  Result.X = - A.X;
  Result.Y = - A.Y;
  Result.Z = - A.Z;
  return Result;
}


inline v3&
operator-=( v3& A, const v3& B )
{
  A = A-B;
  return A;
}

inline r32
NormSq( const v3& A )
{
  r32 Result;
  Result = A*A;
  return Result;
}

inline r32
Norm( const v3& A )
{
  r32 Result = 0;
  Result =  (r32) Sqrt( A*A );
  return Result;
}

inline v3
Normalize( const v3& V )
{
  v3 Result = {};
  r32 N = Norm(V);
  Result = ( Abs(N) > 10E-7 )? V / N : V3(0,0,0);
  return Result;
}


inline v3
Abs( const v3& V )
{
  v3 Result = V3( Abs(V.X), Abs(V.Y), Abs( V.Z ) );
  return Result;
}

inline v3
CrossProduct( const v3& A, const v3& B )
{
  v3 Result = {};
  Result.X = A.Y * B.Z - A.Z * B.Y;
  Result.Y = A.Z * B.X - A.X * B.Z;
  Result.Z = A.X * B.Y - A.Y * B.X;
  return Result;
}

inline r32
Determinant( const v3& Row0, const v3& Row1, const v3& Row2 )
{
  v3  Cross = CrossProduct(Row1,Row2);
  r32 Result = Row0 * Cross;
  return Result;
}

inline b32
operator==( const v3& A, const v3& B )
{
  return Equals(A.X, B.X) && Equals(A.Y, B.Y) && Equals(A.Z, B.Z);
}

inline b32
operator!=( const v3& A, const v3& B )
{
  return !( A == B );
}

// Supported operations:
//  r32 = v4 * v4 (dot)
// v4 = r * v4,
// v4 = v4 * r
// v4 *= r

// v4 = v4 / r
// v4 /= r

// v4 = v4 + v4,
// v4 += v4

// v4 = v4 - v4,
// v4 -= v4.
// v4 = -v4

// bool v4 == v4
// bool v4 !- v4

// v4 Abs(v4)

//  r32 norm
// v4 normalize

inline v4
V4( const r32 X, const r32 Y, const r32 Z, const r32 W)
{
  v4 Result = {X,Y,Z,W};
  return(Result);
};

inline v4
V4( const v3& R, const r32 W = 1 )
{
  v4 Result = {R.X,R.Y,R.Z,W};
  return(Result);
};

inline v4
V4( const v2& R, const r32 Z = 0, const r32 W = 1 )
{
  v4 Result = {R.X,R.Y, Z,W};
  return(Result);
};


inline r32
operator*( const v4& A, const v4& B )
{
  r32 Result = A.X*B.X + A.Y*B.Y + A.Z*B.Z + A.W*B.W;
  return Result;
}

inline v4
operator*( const r32 A, const v4& B )
{
  v4 Result;
  Result.X = A*B.X;
  Result.Y = A*B.Y;
  Result.Z = A*B.Z;
  Result.W = A*B.W;
  return Result;
}

inline v4
operator*( const v4& R, const r32 s )
{
  v4 Result;
  Result = s*R;
  return Result;
}

inline v4&
operator*=( v4& R, const r32 s )
{
  R = s*R;
  return R;
}

inline v4
operator/( const v4& R, const r32 s )
{
  v4 Result;
  r32 inverse = 1/s;
  Result = inverse*R;
  return Result;
}

inline v4&
operator/=( v4& R, const r32 s )
{
  R = R/s;
  return R;
}

inline v4
operator+( const v4& A, const v4& B )
{
  v4 Result;
  Result.X = A.X + B.X;
  Result.Y = A.Y + B.Y;
  Result.Z = A.Z + B.Z;
  Result.W = A.W + B.W;
  return Result;
}

inline v4&
operator+=( v4& A, const v4& B)
{
  A = A+B;
  return A;
}


inline v4
operator-( const v4& A,  const v4& B)
{
  v4 Result;
  Result.X = A.X - B.X;
  Result.Y = A.Y - B.Y;
  Result.Z = A.Z - B.Z;
  Result.W = A.W - B.W;
  return Result;
}

inline v4
operator-( const v4& A)
{
  v4 Result;
  Result.X = - A.X;
  Result.Y = - A.Y;
  Result.Z = - A.Z;
  Result.W = - A.W;
  return Result;
}

inline b32
operator==( const v4& A, const v4& B )
{
  return Equals(A.X, B.X) && Equals(A.Y, B.Y) && Equals(A.Z, B.Z) && Equals(A.W, B.W);
}

inline b32
operator!=( const v4& A, const v4& B )
{
  return !( A == B );
}


inline v4&
operator-=( v4& A, const v4& B )
{
  A = A-B;
  return A;
}

inline r32
NormSq( const v4& R )
{
  r32 Result;
  Result = R*R;
  return Result;
}

inline r32
Norm( const v4& R )
{
   r32 Result;
  Result =  Sqrt(R.X*R.X + R.Y*R.Y + R.Z*R.Z + R.W*R.W);
  return Result;
}

inline v4
Normalize( const v4& R )
{
  v4 Result;
  r32 N = Norm(R);
  Result = ( Abs(N) > 10E-7 )? R / N : V4(0,0,0,0);
  return Result;
}

inline v4
Abs( const v4& V )
{
  v4 Result = V4( Abs(V.X), Abs(V.Y), Abs(V.Z), Abs(V.W) );
  return Result;
}

inline m4
M4Identity()
{
  m4 Result = {
    1,0,0,0,
    0,1,0,0,
    0,0,1,0,
    0,0,0,1};

  return(Result);
}

inline void
AssertIdentity( const m4& I, const r32 epsilon = 0.00001)
{
  Assert( Abs( I.E[0]  -1 ) < epsilon );
  Assert( Abs( I.E[1]     ) < epsilon );
  Assert( Abs( I.E[2]     ) < epsilon );
  Assert( Abs( I.E[3]     ) < epsilon );
  Assert( Abs( I.E[4]     ) < epsilon );
  Assert( Abs( I.E[5]  -1 ) < epsilon );
  Assert( Abs( I.E[6]     ) < epsilon );
  Assert( Abs( I.E[7]     ) < epsilon );
  Assert( Abs( I.E[8]     ) < epsilon );
  Assert( Abs( I.E[9]     ) < epsilon );
  Assert( Abs( I.E[10] -1 ) < epsilon );
  Assert( Abs( I.E[11]    ) < epsilon );
  Assert( Abs( I.E[12]    ) < epsilon );
  Assert( Abs( I.E[13]    ) < epsilon );
  Assert( Abs( I.E[14]    ) < epsilon );
  Assert( Abs( I.E[15] -1 ) < epsilon );
}

inline m3
M3(  const r32 a11, const r32 a12, const r32 a13,
     const r32 a21, const r32 a22, const r32 a23,
     const r32 a31, const r32 a32, const r32 a33)
{
  m3 Result = {
    a11, a12, a13,
    a21, a22, a23,
    a31, a32, a33 };

  return(Result);
};

inline m3
M3( const v3& R0, const v3& R1, const v3& R2 )
{
  m3 Result = {R0, R1, R2};

  return(Result);
};

inline m3
M3( const m4 M)
{
  m3 Result = {V3(M.r0), V3(M.r1), V3(M.r2)};

  return(Result);
};


inline m3
M3Identity()
{
  m3 Result = {
    1,0,0,
    0,1,0,
    0,0,1};
  return(Result);
}


inline m3
operator*( const r32 a, const m3& M )
{
  m3 Result =  M3( M.r0 * a,
                   M.r1 * a,
                   M.r2 * a);
  return Result;
}

inline m3
operator*( const m3& M, r32& a )
{
  m3 Result = a*M;
  return Result;
}

inline m3
Transpose( const m3& A )
{
  m3 Result;

  Result = M3( V3( A.E[0], A.E[ 3], A.E[ 6]),
               V3( A.E[1], A.E[ 4], A.E[ 7]),
               V3( A.E[2], A.E[ 5], A.E[ 8]));

  return Result;
}

inline m3
operator*( const m3& A, const m3& B )
{
  m3 BT = Transpose(B);
  m3 Result = M3( A.r0 * BT.r0, A.r0 * BT.r1, A.r0 * BT.r2,
                  A.r1 * BT.r0, A.r1 * BT.r1, A.r1 * BT.r2,
                  A.r2 * BT.r0, A.r2 * BT.r1, A.r2 * BT.r2);
  return Result;
}

inline m3
operator-( const m3& A,  const m3& B)
{
  m3 Result;
  Result.r0 = A.r0 - B.r0;
  Result.r1 = A.r1 - B.r1;
  Result.r2 = A.r2 - B.r2;
  return Result;
}

inline v3
operator*( const m3& M, const v3& b )
{
  v3 Result =  V3( M.r0 * b,
                   M.r1 * b,
                   M.r2 * b);
  return Result;
}


/*
inline v4
Column( const m3& M, const u32 Column )
{
  Assert(Column < 4);
  v4 Result = V4(M.E[ Column ], M.E[4+Column], M.E[8+Column], M.E[12+Column]);
  return Result;
}

inline void
Column( m3& M, const u32 Column, const v4& ColumnValue )
{
  Assert(Column < 4);
  M.E[ Column ] = ColumnValue.E[0];
  M.E[4+Column] = ColumnValue.E[1];
  M.E[8+Column] = ColumnValue.E[2];
  M.E[12+Column]= ColumnValue.E[3];
}

inline v3
Row( const m3& M, const u32 Row )
{
  Assert(Row < 3);
  u32 Base = 3*Row;
  v3 Result = V3(M.E[Base], M.E[Base+1], M.E[Base+2], M.E[Base+3]);
  return Result;
}

inline void
Row( m3& M, const u32 Row, const v3& RowValue )
{
  Assert(Row < 3);
  u32 Base = 3*Row;
  M.E[ Base  ] = RowValue.E[0];
  M.E[ Base+1] = RowValue.E[1];
  M.E[ Base+2] = RowValue.E[2];
}

inline r32
Index( const m3& M, const u32 Row, const u32 Column )
{
  Assert(Row < 3);
  Assert(Column < 3);
  r32 Result = M.E[3*Row+Column];
  return Result;
}

inline void
Index( m3& M, const u32 Row, const u32 Column, const r32 Value )
{
  Assert(Row < 3);
  Assert(Column < 3);
  M.E[3*Row+Column] = Value;
}
*/

inline m4
M4( const r32 a11, const r32 a12, const r32 a13, const r32 a14,
    const r32 a21, const r32 a22, const r32 a23, const r32 a24,
    const r32 a31, const r32 a32, const r32 a33, const r32 a34,
    const r32 a41, const r32 a42, const r32 a43, const r32 a44)
{
  m4 Result = {
    a11, a12, a13, a14,
    a21, a22, a23, a24,
    a31, a32, a33, a34,
    a41, a42, a43, a44 };

  return(Result);
};

inline m4
M4( const v4& R0, const v4& R1, const v4& R2, const v4& R3 )
{
  m4 Result = {R0, R1, R2, R3};

  return(Result);
};

inline m4
operator*( const r32 a, const m4& M )
{
  m4 Result =  M4( M.r0 * a,
             M.r1 * a,
             M.r2 * a,
             M.r3 * a);
  return Result;
}

inline m4
operator*( const m4& M, r32& a )
{
  m4 Result = a*M;
  return Result;
}

inline m4
Transpose( const m4& A )
{
  m4 Result;

  Result = M4( V4( A.E[0], A.E[ 4], A.E[ 8], A.E[12] ) ,
               V4( A.E[1], A.E[ 5], A.E[ 9], A.E[13] ) ,
               V4( A.E[2], A.E[ 6], A.E[10], A.E[14] ) ,
               V4( A.E[3], A.E[ 7], A.E[11], A.E[15] ) );

  return Result;
}

inline m4
operator*( const m4& A, const m4& B )
{
  //TIMED_FUNCTION();
  m4 BT = Transpose(B);
  m4 Result = M4( A.r0 * BT.r0, A.r0 * BT.r1, A.r0 * BT.r2, A.r0 * BT.r3,
                A.r1 * BT.r0, A.r1 * BT.r1, A.r1 * BT.r2, A.r1 * BT.r3,
                A.r2 * BT.r0, A.r2 * BT.r1, A.r2 * BT.r2, A.r2 * BT.r3,
                A.r3 * BT.r0, A.r3 * BT.r1, A.r3 * BT.r2, A.r3 * BT.r3);
  return Result;
}


inline m4
operator-( const m4& A,  const m4& B)
{
  m4 Result;
  Result.r0 = A.r0 - B.r0;
  Result.r1 = A.r1 - B.r1;
  Result.r2 = A.r2 - B.r2;
  Result.r3 = A.r3 - B.r3;
  return Result;
}

inline v4
operator*( const m4& M, const v4& b )
{
  v4 Result =  V4( M.r0 * b,
             M.r1 * b,
             M.r2 * b,
             M.r3 * b);
  return Result;
}

inline v4
Column( const m4& M, const u32 Column )
{
  Assert(Column < 4);
  v4 Result = V4(M.E[ Column ], M.E[4+Column], M.E[8+Column], M.E[12+Column]);
  return Result;
}

inline void
Column( m4& M, const u32 Column, const v4& ColumnValue )
{
  Assert(Column < 4);
  M.E[ Column ] = ColumnValue.E[0];
  M.E[4+Column] = ColumnValue.E[1];
  M.E[8+Column] = ColumnValue.E[2];
  M.E[12+Column]= ColumnValue.E[3];
}

inline v4
Row( const m4& M, const u32 Row )
{
  Assert(Row < 4);
  u32 Base = 4*Row;
  v4 Result = V4(M.E[Base], M.E[Base+1], M.E[Base+2], M.E[Base+3]);
  return Result;
}

inline void
Row( m4& M, const u32 Row, const v4& RowValue )
{
  Assert(Row < 4);
  u32 Base = 4*Row;
  M.E[ Base  ] = RowValue.E[0];
  M.E[ Base+1] = RowValue.E[1];
  M.E[ Base+2] = RowValue.E[2];
  M.E[ Base+3] = RowValue.E[3];
}

inline r32
Index( const m4& M, const u32 Row, const u32 Column )
{
  Assert(Row < 4);
  Assert(Column < 4);
  r32 Result = M.E[4*Row+Column];
  return Result;
}

inline void
Index( m4& M, const u32 Row, const u32 Column, const r32 Value )
{
  Assert(Row < 4);
  Assert(Column < 4);
  M.E[4*Row+Column] = Value;
}

inline v3
GetPlaneNormal(const v3& A, const v3& B, const v3& C)
{
  const v3 ab = B - A;
  const v3 ac = C - A;
  const v3 Result = Normalize( CrossProduct(ab,ac) );
  return Result;
}

inline v3
ProjectPointOntoPlane( const v3& PointToProject, const v3& PointOnPlane, const v3& PlaneNormal )
{
  const v3 o = (PointToProject - PointOnPlane);
  const v3 ProjectionToNormal = (o * PlaneNormal) * PlaneNormal;
  const v3 ProjectedPoint = PointToProject - ProjectionToNormal;
  return ProjectedPoint;
}

inline r32
RaycastPlane( const v3& RayOrigin, const v3& RayDirection, const v3& PointOnPlane, const v3& PlaneNormal)
{
  // Ray:   P = RayOrigin + t * RayDirection
  // Plane: (P - PointOnPlane) * PlaneNormal = 0
  // (RayOrigin + t*RayDirection - PointOnPlane) * PlaneNormal = 0;
  // RayOrigin * PlaneNormal + t*RayDirection * PlaneNormal - PointOnPlane* PlaneNormal = 0;
  // t * RayDirection * PlaneNormal = PointOnPlane * PlaneNormal - RayOrigin * PlaneNormal;
  // t = (PointOnPlane * PlaneNormal - RayOrigin * PlaneNormal) / (RayDirection * PlaneNormal)

  v3 RayToPlanePoint = PointOnPlane - RayOrigin;
  r32 Numerator = RayToPlanePoint * PlaneNormal;
  r32 Denominator = PlaneNormal * RayDirection;

  // The RayOrigin is on the plane and the ray goes parallel to the plane
  // Can probably be handled in some way, but cant be arsed to do it untill it becomes
  // a problem. This assert should notify me when it does become a problem.
  Assert(!(Equals(Numerator,0) && Equals(Denominator,0)));

  // P = RayOrigin + t * RayDirection;
  r32 t = Numerator / Denominator;
  return t;
}

inline b32
IsPointOnLinesegment(const v3& o, const v3& d, const v3& p)
{
  // o = origin of linesegment
  // d = destination of linesegment
  // p = point to check
  Assert(o != d);

  const v3 od = d-o;
  const v3 op = p-o;
  const v3 cross = CrossProduct(od,op);
  const r32 CrossLenSq = NormSq(cross);
  if( CrossLenSq > 10E-7)
  {
    return false;
  }

  r32 dotp = od*op;
  if(dotp < 0)
  {
    return false;
  }

  r32 LenSq = NormSq(od);
  if(dotp > LenSq)
  {
    return false;
  }

  return true;
}

v3 GetBaryocentricCoordinatesSlow(const v3& p0, const v3& p1, const v3& p2, const v3& normal, const v3& Point)
{
  r32 OneOverFaceArea = 1.f/(normal * CrossProduct( p1 - p0, p2 - p0));
  r32 SubAreaA = normal * CrossProduct( p2 - p1, Point - p1);
  r32 SubAreaB = normal * CrossProduct( p0 - p2, Point - p2);
  r32 SubAreaC = normal * CrossProduct( p1 - p0, Point - p0);
  r32 LambdaA = SubAreaA * OneOverFaceArea;
  r32 LambdaB = SubAreaB * OneOverFaceArea;
  r32 LambdaC = SubAreaC * OneOverFaceArea;
  v3 Result = V3(LambdaA, LambdaB, LambdaC);
  return Result;
}

v3 GetBaryocentricCoordinates(const v3& p0, const v3& p1, const v3& p2, const v3& normal, const v3& Point)
{
  r32 OneOverFaceArea = 1.f/(normal * CrossProduct( p1 - p0, p2 - p0));
  r32 SubAreaA = normal * CrossProduct( p2 - p1, Point - p1);
  r32 SubAreaB = normal * CrossProduct( p0 - p2, Point - p2);

  r32 LambdaA = SubAreaA * OneOverFaceArea;
  r32 LambdaB = SubAreaB * OneOverFaceArea;
  r32 LambdaC = 1.f-(LambdaA + LambdaB);

  v3 Result = V3(LambdaA, LambdaB, LambdaC);
  return Result;
}

// Checks if the Vertex can be projected onto the Triangle.
inline b32
IsVertexInsideTriangle(const v3& VertexOnPlane, const v3& Normal, const v3& p0, const v3& p1, const v3& p2)
{
  const v3 Coords = GetBaryocentricCoordinates( p0, p1, p2, Normal, VertexOnPlane);
  const b32 InsideTriangle = (Coords.E[0] >= 0) && (Coords.E[0] <= 1) &&
                             (Coords.E[1] >= 0) && (Coords.E[1] <= 1) &&
                             (Coords.E[2] >= 0) && (Coords.E[2] <= 1);
  return InsideTriangle;
}

inline r32
LinearTransform(const r32 OutputMin, const r32 OutputMax, const r32 InputMin, const r32 InputMax, const r32 Input)
{
  const r32 M = OutputMin;
  const r32 K = (InputMax - InputMin) / (OutputMax - OutputMin);
  const r32 Result = K * Input + M;
  return Result;
}

inline v4
QuaternionMultiplication(const v4 r, const v4 q)
{
  v4 Result = {};
  Result.W = r.W*q.W - r.X*q.X - r.Y*q.Y - r.Z*q.Z;  // scalar
  Result.X = r.W*q.X + r.X*q.W - r.Y*q.Z + r.Z*q.Y;  // x-part (i)
  Result.Y = r.W*q.Y + r.X*q.Z + r.Y*q.W - r.Z*q.X;  // y-part (j)
  Result.Z = r.W*q.Z - r.X*q.Y + r.Y*q.X + r.Z*q.W;  // z-part (k)
  return Result;
}

inline v3
QuaternionToEuler(const v4 q)
{
  // roll (x-axis rotation)
  r32 sinr_cosp = 2 * (q.W * q.X + q.Y * q.Z);
  r32 cosr_cosp = 1 - 2 * (q.X * q.X + q.Y * q.Y);
  r32 roll = ATan2(sinr_cosp, cosr_cosp);

  // pitch (y-axis rotation)
  r32 sinp = 2 * (q.W * q.Y - q.Z * q.X);
  r32 pitch = 0;
  if (Abs(sinp) >= 1)
    pitch = sinp > 0 ? Pi32 / 2.f : -Pi32 / 2.f;
  else
    pitch = ASin(sinp);

  // yaw (z-axis rotation)
  r32 siny_cosp = 2 * (q.W * q.Z + q.X * q.Y);
  r32 cosy_cosp = 1 - 2 * (q.Y * q.Y + q.Z * q.Z);
  r32 yaw = ATan2(siny_cosp, cosy_cosp);

  v3 Result = V3(roll, pitch, yaw);
  return Result;
}

internal inline m4
GetRotationMatrix_X_ZHint(v3 ObjWorld_X, v3 ObjWorld_Z_Hint)
{
  ObjWorld_X = Normalize(ObjWorld_X);
  // ObjWorld_X is the x-axis (left) of the obeject in the worlds coordinate system
  // ObjWorld_Z_Hint is a hint of the z-axis (front facing) of the obeject in the worlds coordinate system
  // These will be used to find the up-direction of the object in world-coordinate system

  // Make sure ObjWorld_Z_Hint and ObjWorld_X are not parallel
  if( Abs( (ObjWorld_Z_Hint * ObjWorld_X) - 1.0f ) <  0.0001f )
  {
    return M4Identity();
  }

  // Z cross X = Y
  v3 ObjWorld_Y = Normalize(CrossProduct(ObjWorld_Z_Hint, ObjWorld_X));
  // X cross Y = Z
  v3 ObjWorld_Z = Normalize(CrossProduct(ObjWorld_X, ObjWorld_Y));

  // Rotates from WorldCoordinateSystem to NewCoordinateSystem
  // RotMat * V3(1,0,0) = xp
  // RotMat * V3(0,1,0) = yp
  // RotMat * V3(0,0,1) = zp
  m4 RotMat = M4( ObjWorld_X.X, ObjWorld_Y.X, ObjWorld_Z.X, 0,
                  ObjWorld_X.Y, ObjWorld_Y.Y, ObjWorld_Z.Y, 0,
                  ObjWorld_X.Z, ObjWorld_Y.Z, ObjWorld_Z.Z, 0,
                  0,   0,   0, 1);

  return RotMat;
}

internal inline m4
GetRotationMatrix_X_YHint(v3 ObjWorld_X, v3 ObjWorld_Y_Hint)
{
  ObjWorld_X = Normalize(ObjWorld_X);
  // ObjWorld_X is the x-axis (left) of the obeject in the worlds coordinate system
  // ObjWorld_Y_Hint is a hint of the y-axis (up) of the obeject in the worlds coordinate system
  // These will be used to find the up-direction of the object in world-coordinate system

  // Make sure ObjWorld_Y_Hint and ObjWorld_X are not parallel
  if( Abs( (ObjWorld_X * ObjWorld_Y_Hint) - 1.0f ) <  0.0001f )
  {
    return M4Identity();
  }

  // X cross Y = Z
  v3 ObjWorld_Z = Normalize(CrossProduct(ObjWorld_X, ObjWorld_Y_Hint));
  // Z cross X = Y
  v3 ObjWorld_Y = Normalize(CrossProduct(ObjWorld_Z, ObjWorld_X));

  // Rotates from WorldCoordinateSystem to NewCoordinateSystem
  // RotMat * V3(1,0,0) = xp
  // RotMat * V3(0,1,0) = yp
  // RotMat * V3(0,0,1) = zp
  m4 RotMat = M4( ObjWorld_X.X, ObjWorld_Y.X, ObjWorld_Z.X, 0,
                  ObjWorld_X.Y, ObjWorld_Y.Y, ObjWorld_Z.Y, 0,
                  ObjWorld_X.Z, ObjWorld_Y.Z, ObjWorld_Z.Z, 0,
                  0,   0,   0, 1);

  return RotMat;
}

/*
m2 Skew(v2 A)
{
  // NOTE(Jakob): Skew is a matrix representation of cross product such that
  //              AxB = Skew(A)*B.  Transpose(Skew(A)) = -Skew(A);
  m2 Result = M2(0,-A.Z,A.Y,
                 A.Z, 0, -A.X,
                 -A.Y, A.X,0);
  return Result;
}
*/
m3 Skew(v3 A)
{
  // NOTE(Jakob): Skew is a matrix representation of cross product such that
  //              AxB = Skew(A)*B.  Transpose(Skew(A)) = -Skew(A);
  m3 Result = M3(0,-A.Z,A.Y,
                 A.Z, 0, -A.X,
                 -A.Y, A.X,0);
  return Result;
}

// Assumes Normal is of length one.
// Returns two orthonormal vectors where Normal = Cross(Tangent1,Tangent2)
void getOrthronormalVectorPair(v3 Normal, v3* Tangent1, v3* Tangent2)
{
  Assert(Equals(Norm(Normal),1.0,10E-7));
  *Tangent1 = V3(0.0f, Normal.Z, -Normal.Y);
  if (Normal.X >= 0.57735f)
  {
    *Tangent1 = V3(Normal.Y, -Normal.X, 0.0f);
  }
  Normalize(*Tangent1);
  *Tangent2 = CrossProduct(Normal, *Tangent1);
}