#ifndef HANDMADE_MATH_H
#define HANDMADE_MATH_H

#include <math.h>

union v2
{
	struct{
		real32 X, Y;
	};
	real32 E[2];
};

v2 V2(real32 X, real32 Y){
	v2 Result = {X,Y};
	return(Result);
};

inline v2
operator*(real32 A, v2 B)
{
	v2 Result;
	Result.X = A*B.X;
	Result.Y = A*B.Y;
	return Result;	
}

inline v2
operator*(v2 B, real32 A)
{
	v2 Result;
	Result = A*B;
	return Result;	
}

inline v2&
operator*=(v2& B, real32 A)
{
	B = A*B;
	return B;
}

inline v2
operator+(v2 A, v2 B)
{
	v2 Result;
	Result.X = A.X + B.X;
	Result.Y = A.Y + B.Y;
	return Result;
}

inline v2&
operator+=(v2& A, v2 B)
{
	A = A+B;
	return A;
}


inline v2
operator-(v2 A, v2 B)
{
	v2 Result;
	Result.X = A.X - B.X;
	Result.Y = A.Y - B.Y;
	return Result;
}

inline v2&
operator-=(v2& A, v2 B)
{
	A = A-B;
	return A;
}

inline real32
DotProduct(v2 A, v2 B)
{
	real32 Result = A.X*B.X + A.Y*B.Y;
	return Result;
}

union v3
{
	struct{
		real32 X, Y, Z;
	};
	real32 E[3];
};

v3 V3(real32 X, real32 Y, real32 Z){
	v3 Result = {X,Y,Z};
	return(Result);
};

inline v3
operator*(real32 A, v3 B)
{
	v3 Result;
	Result.X = A*B.X;
	Result.Y = A*B.Y;
	Result.Z = A*B.Z;
	return Result;	
}

inline v3
operator*(v3 B, real32 A)
{
	v3 Result;
	Result = A*B;
	return Result;	
}

inline v3&
operator*=(v3& B, real32 A)
{
	B = A*B;
	return B;
}

inline v3
operator+(v3 A, v3 B)
{
	v3 Result;
	Result.X = A.X + B.X;
	Result.Y = A.Y + B.Y;
	Result.Z = A.Z + B.Z;
	return Result;
}

inline v3&
operator+=(v3& A, v3 B)
{
	A = A+B;
	return A;
}


inline v3
operator-(v3 A, v3 B)
{
	v3 Result;
	Result.X = A.X - B.X;
	Result.Y = A.Y - B.Y;
	Result.Z = A.Z - B.Z;
	return Result;
}

inline v3&
operator-=(v3& A, v3 B)
{
	A = A-B;
	return A;
}

union v4
{
	struct{
		real32 X, Y, Z, W;
	};
	real32 E[4];
};


v4 V4(real32 X, real32 Y, real32 Z, real32 W){
	v4 Result = {X,Y,Z,W};
	return(Result);
};

inline real32
operator*(v4 A, v4 B)
{
	real32 Result = A.X*B.X + A.Y*B.Y + A.Z*B.Z + A.W*B.W;
	return Result;	
}

inline v4
operator*(real32 A, v4 B)
{
	v4 Result;
	Result.X = A*B.X;
	Result.Y = A*B.Y;
	Result.Z = A*B.Z;
	Result.W = A*B.W;
	return Result;	
}

inline v4
operator*(v4 B, real32 A)
{
	v4 Result;
	Result = A*B;
	return Result;	
}

inline v4&
operator*=(v4& B, real32 A)
{
	B = A*B;
	return B;
}

inline v4
operator+(v4 A, v4 B)
{
	v4 Result;
	Result.X = A.X + B.X;
	Result.Y = A.Y + B.Y;
	Result.Z = A.Z + B.Z;
	Result.Z = A.W + B.W;
	return Result;
}

inline v4&
operator+=(v4& A, v4 B)
{
	A = A+B;
	return A;
}


inline v4
operator-(v4 A, v4 B)
{
	v4 Result;
	Result.X = A.X - B.X;
	Result.Y = A.Y - B.Y;
	Result.Z = A.Z - B.Z;
	Result.W = A.W - B.W;
	return Result;
}

inline v4&
operator-=(v4& A, v4 B)
{
	A = A-B;
	return A;
}

real32 norm( v4 A )
{
	return (real32) Sqrt(A.X*A.X + A.Y*A.Y + A.Z * A.Z + A.W*A.W);
}

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
	real32 E[16];
};

m4 M4(real32 a11, real32 a12, real32 a13, real32 a14,
	  real32 a21, real32 a22, real32 a23, real32 a24,
	  real32 a31, real32 a32, real32 a33, real32 a34,
	  real32 a41, real32 a42, real32 a43, real32 a44)
{
	m4 Result = {
		a11, a12, a13, a14,
		a21, a22, a23, a24,
		a31, a32, a33, a34,
		a41, a42, a43, a44 };

	return(Result);
};

m4 M4(v4 R0, v4 R1, v4 R2, v4 R3 )
{
	m4 Result = {R0, R1, R2, R3};

	return(Result);
};

inline m4
operator*(real32 a, m4 M)
{
	m4 Result =  M4( M.r0 * a,
				     M.r1 * a, 
				     M.r2 * a, 
				     M.r3 * a);
	return Result;
}

inline m4
operator*( m4 M, real32 a)
{
	m4 Result = a*M;
	return Result;
}

inline m4 
Transpose( m4 A )
{
	m4 Result;

	Result = M4( V4( A.E[0], A.E[ 4], A.E[ 8], A.E[12] ) ,
		         V4( A.E[1], A.E[ 5], A.E[ 9], A.E[13] ) ,
		         V4( A.E[2], A.E[ 6], A.E[10], A.E[14] ) ,
		         V4( A.E[3], A.E[ 7], A.E[11], A.E[15] ) );

	return Result;
}

inline m4
operator*(m4 A, m4 B)
{
	m4 BT = Transpose(B);
	m4 Result = M4( A.r0 * BT.r0, A.r0 * BT.r1, A.r0 * BT.r2, A.r0 * BT.r3,
		            A.r1 * BT.r0, A.r1 * BT.r1, A.r1 * BT.r2, A.r1 * BT.r3,
		            A.r2 * BT.r0, A.r2 * BT.r1, A.r2 * BT.r2, A.r2 * BT.r3,
		            A.r3 * BT.r0, A.r3 * BT.r1, A.r3 * BT.r2, A.r3 * BT.r3);
	return Result;	
}

inline v4
operator*(m4 M, v4 b)
{
	v4 Result =  V4( M.r0 * b,
				     M.r1 * b, 
				     M.r2 * b, 
				     M.r3 * b);
	return Result;
}

inline m4
AffineInverse( m4 A )
{
//  Exploit that A is affine:
//
//	A = [ M   b  ]
//      [ 0   1  ]
//
//	where A is 4x4, M is 3x3, b is 3x1, and the bottom row is (0,0,0,1), then
//
//	inv(A) = [ inv(M)   -inv(M) * b ]
//         	 [   0            1     ]

	// Assert that matrix is affine and x is a point
	Assert( A.E[12] == 0 );
	Assert( A.E[13] == 0 );
	Assert( A.E[14] == 0 );
	Assert( A.E[15] == 1 );

	real32 Det = A.E[0] * (A.E[ 5] * A.E[10] - A.E[ 9] * A.E[ 6]) +
				 A.E[1] * (A.E[ 8] * A.E[ 6] - A.E[ 4] * A.E[10]) + 
				 A.E[2] * (A.E[ 4] * A.E[ 9] - A.E[ 8] * A.E[ 5]);
	
	Assert( ( Det > 0.000001 ) || ( Det < -0.000001 ) );

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


void U_M4Mul()
{

	m4 GT = M4(
		V4( 278, -528,  411,  112),
		V4(-164, -316, -289, -196),
		V4( 462, -116,   37,  423),
		V4(  22, -213,  425,  -58));

	m4 x = M4(
		V4( -4, 15, -20,-5),
		V4( -8, 15, 12, -15),
		V4( -9, -18, -13, 8),
		V4(  8, 11, -15,-4));

	m4 y = M4(
		V4( -17, 17, 11, -13 ),
		V4( -5, -4, 9, -8),
		V4( -15, 15, -18, -10),
		V4( 3, 20, 8, 4));

	m4 xy = x*y;

 	for (int i = 0; i < 16; ++i)
	{
		real32 tol = 0.0000001;
		real32 A = xy.E[i];
		real32 B = GT.E[i];
		real32 diff = (A - B);
		Assert( ( diff < tol ) && ( diff > -tol ) );
	}
}


void U_AffineInverse()
{
	m4 x = M4(
		V4( -4, 15, -20,-5),
		V4( -8, 15, 12, -15),
		V4( -9, -18, -13, 8),
		V4(  0, 0, 0, 1));

	m4 Inv = AffineInverse(x);
	m4 I = Inv * x;

 	for (int i = 0; i < 16; ++i)
	{
		real32 tol = 0.000001;
		if((i == 0) || (i == 5) || (i == 10) || (i == 15) )
		{
			Assert( Abs( 1 - I.E[i] ) < tol );
		}else{
			Assert( Abs( I.E[i] ) < tol );
		}
	}

}

#endif