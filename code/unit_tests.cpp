#include "vector_math.h"


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
