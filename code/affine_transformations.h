#ifndef AFFINE_TRANSFORMATIONS
#define AFFINE_TRANSFORMATIONS


inline m4
AffineInverse( const m4& A )
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
	m4 Result = M4(	  A.E[0], A.E[4], A.E[ 8], -A.E[0] * A.E[3] - A.E[4] * A.E[7] - A.E[8]  * A.E[11], 
					  A.E[1], A.E[5], A.E[ 9], -A.E[1] * A.E[3] - A.E[5] * A.E[7] - A.E[9]  * A.E[11], 
					  A.E[2], A.E[6], A.E[10], -A.E[2] * A.E[3] - A.E[6] * A.E[7] - A.E[10] * A.E[11], 
					       0,      0,       0,                           1);

	return Result;
}

inline m4 
QuatAsMatrix( const v4& Quaternion )
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

inline m4
GetRotationMatrix( const r32 Angle, const v4& axis)
{
	v4 Quaternion  = RotateQuaternion( Angle, axis );
	m4 R = QuatAsMatrix( Quaternion );

	return R;
}

inline void 
Translate( const v4& Translation, m4& M )
{
	M.E[3]  += Translation.X;
	M.E[7]  += Translation.Y;
	M.E[11] += Translation.Z;
}

inline void 
Rotate( const r32 Angle, const v4& Axis, m4& T )
{
	m4 RotMat = GetRotationMatrix( Angle, Axis );

	m4 tto = M4(1,0,0,-T.E[3],
				0,1,0,-T.E[7],
				0,0,1,-T.E[13],
				0,0,0,1);

	m4 bfo = M4(1,0,0, T.E[3],
				0,1,0, T.E[7],
				0,0,1, T.E[13],
				0,0,0,1);

	T =  bfo * RotMat * tto * T;
}

inline void 
Scale( const v4& Scale, m4& T ) 
{
	T.E[0]  *= Scale.X;
	T.E[5]  *= Scale.Y;
	T.E[10] *= Scale.Z;
}

inline v4 
PointMultiply( const m4& M, const v4& b )
{
	v4 Result = M*b;

	Assert(( Abs( Result.W ) > 10e-10 ));

	if( Result.W != 1 )
	{
		Result /= Result.W;
	}

	return Result;
}

#endif