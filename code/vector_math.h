#ifndef MATH_H
#define MATH_H

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
	return ( A.X == B.X ) && (A.Y == B.Y ); 
}

inline b32 
operator!=( const v2& A, const v2& B )
{
	return !( A == B ); 
}

inline r32 
Norm( const v2& A )
{
	 r32 Result;
	Result =  ( r32) sqrt( A*A ); 
	return Result;
}

inline v2 Normalize( const v2& A )
{
	v2 Result;
	Result = A / Norm(A);
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
	r32 Result;
	Result = A.X*B.X + A.Y*B.Y + A.Z*B.Z;
	return Result;	
}

inline v3 
operator*( const r32 A, const v3& B )
{
	v3 Result;
	Result.X = A*B.X;
	Result.Y = A*B.Y;
	Result.Z = A*B.Z;
	return Result;	
}

inline v3 
operator*( const v3& B, const r32 s )
{
	v3 Result;
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
	v3 Result;
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
	v3 Result;
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
Norm( const v3& A )
{
	r32 Result;
	Result =  (r32) Sqrt( A*A ); 
	return Result;
}

inline v3 
Normalize( const v3& V )
{
	v3 Result;
	Result = V / Norm(V);
	return Result;
}

inline v3 
CrossProduct( const v3& A, const v3& B )
{
	v3 Result;
	Result.X = A.Y * B.Z - A.Z * B.Y;
	Result.Y = A.Z * B.X - A.X * B.Z;
	Result.Z = A.X * B.Y - A.Y * B.X;
	return Result;
}

inline b32 
operator==( const v3& A, const v3& B )
{
	return ( A.X == B.X ) && (A.Y == B.Y ) == (A.Z ==  B.Z); 
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
	return ( A.X == B.X ) && (A.Y == B.Y ) && (A.Z == B.Z ) && (A.W == B.W ); 
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
Norm( const v4& R )
{
	 r32 Result;
	Result = ( r32) Sqrt(R.X*R.X + R.Y*R.Y + R.Z*R.Z + R.W*R.W);
	return Result;
}

inline v4 
Normalize( const v4& R )
{
	v4 Result;
	Result = R / Norm(R); 
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
	m4 BT = Transpose(B);
	m4 Result = M4( A.r0 * BT.r0, A.r0 * BT.r1, A.r0 * BT.r2, A.r0 * BT.r3,
		            A.r1 * BT.r0, A.r1 * BT.r1, A.r1 * BT.r2, A.r1 * BT.r3,
		            A.r2 * BT.r0, A.r2 * BT.r1, A.r2 * BT.r2, A.r2 * BT.r3,
		            A.r3 * BT.r0, A.r3 * BT.r1, A.r3 * BT.r2, A.r3 * BT.r3);
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


#endif