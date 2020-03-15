#include "math/vector_math.h"


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
		r32 tol = 0.000001;
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
		r32 tol = 0.0000001;
		r32 A = xy.E[i];
		r32 B = GT.E[i];
		r32 diff = (A - B);
		Assert( ( diff < tol ) && ( diff > -tol ) );
	}
}



void VerifyVertexEdge()
{
	v3 V,E0,E1;
	closest_vertex_pair R;


	V = V3(1,1,0);
	E0 = V3(0,0,0);
	E1 = V3(2,0,0); 
	R = VertexEdge(V, E0, E1);
	Assert( R.P0 == V3(1,1,0) && R.P1 == V3(1,0,0) );
	Assert( ( (E1-E0)*(R.P0-R.P1) - 1 ) < 1E-10 );


	// On the line
	V = V3(-1,0,0);
	R = VertexEdge(V, E0, E1);
	Assert( R.P0 == V3(-1,0,0) && R.P1 == V3(0,0,0) );

	V = V3(0,0,0);
	R = VertexEdge(V, E0, E1);
	Assert( R.P0 == V3(0,0,0) && R.P1 == V3(0,0,0) );

	V = V3(1,0,0);
	R = VertexEdge(V, E0, E1);
	Assert( R.P0 == V3(1,0,0) && R.P1 == V3(1,0,0) );

	V = V3(2,0,0);
	R = VertexEdge(V, E0, E1);
	Assert( R.P0 == V3(2,0,0) && R.P1 == V3(2,0,0) );

	V = V3(3,0,0);
	R = VertexEdge(V, E0, E1);
	Assert( R.P0 == V3(3,0,0) && R.P1 == V3(2,0,0) );


	// Above  and parallel to the edge
	V = V3(-1,1,0);
	R = VertexEdge(V, E0, E1);
	Assert( R.P0 == V3(-1,1,0) && R.P1 == V3(0,0,0) );

	V = V3(0,1,0);
	R = VertexEdge(V, E0, E1);
	Assert( R.P0 == V3(0,1,0) && R.P1 == V3(0,0,0) );
	r32 Tol = 1E-10;
	r32 Diff = Abs( (E1-E0)*(R.P0-R.P1));
	Assert( Diff < Tol );

	V = V3(1,1,0);
	R = VertexEdge(V, E0, E1);
	Assert( R.P0 == V3(1,1,0) && R.P1 == V3(1,0,0) );
	Diff = Abs( (E1-E0)*(R.P0-R.P1));
	Assert( Diff < Tol );

	V = V3(2,1,0);
	R = VertexEdge(V, E0, E1);
	Assert( R.P0 == V3(2,1,0) && R.P1 == V3(2,0,0) );
	Diff = Abs( (E1-E0)*(R.P0-R.P1));
	Assert( Diff < Tol );

	V = V3(3,1,0);
	R = VertexEdge(V, E0, E1);
	Assert( R.P0 == V3(3,1,0) && R.P1 == V3(2,0,0) );

	// Perpendicular to the edge
	V = V3(1,0,-1);
	R = VertexEdge(V, E0, E1);
	Assert( R.P0 == V3(1,0,-1) && R.P1 == V3(1,0,0) );
	Diff = Abs( (E1-E0)*(R.P0-R.P1));
	Assert( Diff < Tol );

	V = V3(1,0,0);
	R = VertexEdge(V, E0, E1);
	Assert( R.P0 == V3(1,0,0) && R.P1 == V3(1,0,0) );
	Diff = Abs( (E1-E0)*(R.P0-R.P1));
	Assert( Diff < Tol );

	V = V3(1,0,1);
	R = VertexEdge(V, E0, E1);
	Assert( R.P0 == V3(1,0,1) && R.P1 == V3(1,0,0) );
	Diff = Abs( (E1-E0)*(R.P0-R.P1));
	Assert( Diff < Tol );


	E0 = V3(-1,4,7);
	E1 = V3(7,-5,2);
	V = V3(2,0,2);
	R = VertexEdge(V, E0, E1);
	Assert( R.P0 == V3(2,0,2) && R.P1 == V3(3,-0.5,4.5) );

	// Verifies that the line between the two closest points is perpendicular to the Edge.
	// This is only true if the projected closest point is on the Edge.
	Diff = Abs( (E1-E0)*(R.P0-R.P1));
	Assert( Diff < Tol );
	// Verifies that P1 Lies on the edge E1-E0
	Diff = Normalize(E1-E0)*Normalize(R.P1-E0) - 1;
	Assert( Diff < Tol );

}

void VerifyVertexFace(  )
{
	aabb_feature_face Face = {};

	// Square X-Y Plane Z=0
	Face.P[0] = V3(-1,-1,0);
	Face.P[1] = V3(1,-1,0);
	Face.P[2] = V3(1,1,0);
	Face.P[3] = V3(-1,1,0);

	// Center Same Plane
	v3 Vertex = V3(0,0,0);
	closest_vertex_pair R = VertexFace( Vertex, Face );
	Assert( R.P0 == V3(0,0,0) && R.P1 == V3(0,0,0) );

	// Corners Same Plane On Edges
	Vertex = V3(-1,-1, 0);
	R = VertexFace( Vertex, Face );
	Assert( R.P0 == Vertex && R.P1 == Vertex );

	Vertex = V3( 1,-1, 0);
	R = VertexFace( Vertex, Face );
	Assert( R.P0 == Vertex && R.P1 == Vertex );

	Vertex = V3( 1, 1, 0);
	R = VertexFace( Vertex, Face );
	Assert( R.P0 == Vertex && R.P1 == Vertex );

	Vertex = V3(-1, 1, 0);
	R = VertexFace( Vertex, Face );
	Assert( R.P0 == Vertex && R.P1 == Vertex );


	// Corners Same Plane Outside
	Vertex = V3(-2,-2, 0);
	R = VertexFace( Vertex, Face );
	Assert( R.P0 == Vertex && R.P1 == V3(-1,-1,0) );

	Vertex = V3( 2,-2, 0);
	R = VertexFace( Vertex, Face );
	Assert( R.P0 == Vertex && R.P1 == V3(1,-1,0) );

	Vertex = V3( 2, 2, 0);
	R = VertexFace( Vertex, Face );
	Assert( R.P0 == Vertex && R.P1 == V3(1,1,0) );

	Vertex = V3(-2, 2, 0);
	R = VertexFace( Vertex, Face );
	Assert( R.P0 == Vertex && R.P1 == V3(-1,1,0) );

	// Corners Same Plane Inside
	Vertex = V3(-0.5,-0.5, 0);
	R = VertexFace( Vertex, Face );
	Assert( R.P0 == Vertex && R.P1 == Vertex );

	Vertex = V3( 0.5,-0.5, 0);
	R = VertexFace( Vertex, Face );
	Assert( R.P0 == Vertex && R.P1 == Vertex );

	Vertex = V3( 0.5, 0.5, 0);
	R = VertexFace( Vertex, Face );
	Assert( R.P0 == Vertex && R.P1 == Vertex );

	Vertex = V3(-0.5, 0.5, 0);
	R = VertexFace( Vertex, Face );
	Assert( R.P0 == Vertex && R.P1 == Vertex );


	// Tangent With Edges
	Vertex = V3( -1, -2, 0);
	R = VertexFace( Vertex, Face );
	Assert( R.P0 == Vertex && R.P1 == V3(-1,-1,0) );

	Vertex = V3( -2, -1, 0);
	R = VertexFace( Vertex, Face );
	Assert( R.P0 == Vertex && R.P1 == V3(-1,-1,0) );

	Vertex = V3( 2, -1, 0);
	R = VertexFace( Vertex, Face );
	Assert( R.P0 == Vertex && R.P1 == V3( 1,-1,0) );

	Vertex = V3( 1, -2, 0);
	R = VertexFace( Vertex, Face );
	Assert( R.P0 == Vertex && R.P1 == V3( 1,-1,0) );


	// Center One Up
	Vertex = V3(0,0,1);
	R = VertexFace( Vertex, Face );
	Assert( R.P0 == Vertex && R.P1 == V3(0,0,0) );

	// Corners One Up
	Vertex = V3(-1,-1, 1);
	R = VertexFace( Vertex, Face );
	Assert( R.P0 == Vertex && R.P1 == V3(-1,-1,0) );

	Vertex = V3( 1,-1, 1);
	R = VertexFace( Vertex, Face );
	Assert( R.P0 == Vertex && R.P1 == V3( 1,-1,0) );

	Vertex = V3( 1, 1, 1);
	R = VertexFace( Vertex, Face );
	Assert( R.P0 == Vertex && R.P1 == V3(1, 1,0) );

	Vertex = V3(-1, 1, 1);
	R = VertexFace( Vertex, Face );
	Assert( R.P0 == Vertex && R.P1 == V3(-1, 1,0) );


	// Corners Same Plane One Up
	Vertex = V3(-0.5,-0.5, 1);
	R = VertexFace( Vertex, Face );
	Assert( R.P0 == Vertex && R.P1 == V3(-0.5,-0.5, 0) );

	Vertex = V3( 0.5,-0.5, 1);
	R = VertexFace( Vertex, Face );
	Assert( R.P0 == Vertex && R.P1 == V3( 0.5,-0.5, 0) );

	Vertex = V3( 0.5, 0.5, 1);
	R = VertexFace( Vertex, Face );
	Assert( R.P0 == Vertex && R.P1 == V3( 0.5, 0.5, 0) );

	Vertex = V3(-0.5, 0.5, 1);
	R = VertexFace( Vertex, Face );
	Assert( R.P0 == Vertex && R.P1 == V3(-0.5, 0.5, 0) );



	// Center One Down
	Vertex = V3(0,0,1);
	R = VertexFace( Vertex, Face );
	Assert( R.P0 == Vertex && R.P1 == V3(0,0,0) );

	// Corners One Down
	Vertex = V3(-1,-1,-1);
	R = VertexFace( Vertex, Face );
	Assert( R.P0 == Vertex && R.P1 == V3(-1,-1, 0) );
	Vertex = V3( 1,-1,-1);
	R = VertexFace( Vertex, Face );
	Assert( R.P0 == Vertex && R.P1 == V3( 1,-1, 0) );
	Vertex = V3( 1, 1,-1);
	R = VertexFace( Vertex, Face );
	Assert( R.P0 == Vertex && R.P1 == V3( 1, 1,0) );
	Vertex = V3(-1, 1,-1);
	R = VertexFace( Vertex, Face );
	Assert( R.P0 == Vertex && R.P1 == V3(-1, 1,0) );


	// Corners Same Plane Inside, one up
	Vertex = V3(-0.5,-0.5, -1);
	R = VertexFace( Vertex, Face );
	Assert( R.P0 == Vertex && R.P1 == V3(-0.5,-0.5, 0) );

	Vertex = V3( 0.5,-0.5, -1);
	R = VertexFace( Vertex, Face );
	Assert( R.P0 == Vertex && R.P1 == V3( 0.5,-0.5, 0) );

	Vertex = V3( 0.5, 0.5, -1);
	R = VertexFace( Vertex, Face );
	Assert( R.P0 == Vertex && R.P1 == V3( 0.5, 0.5, 0) );

	Vertex = V3(-0.5, 0.5, -1);
	R = VertexFace( Vertex, Face );
	Assert( R.P0 == Vertex && R.P1 == V3(-0.5, 0.5, 0) );

	#if 0
	// Triangle at an angle
	Face = list<v3>(Arena);
	Face.InsertAfter( V3(1,0,0) );
	Face.InsertAfter( V3(0,1,0) );
	Face.InsertAfter( V3(0,0,1) );
	v3 FaceEdge = V3(0,1,0) - V3(1,0,0);

	Vertex = V3(1,1,1);
	R = VertexFace( Vertex, Face );
	v3 DiffVec = R.P1 - V3(1/3.f, 1/3.f, 1/3.f);
	r32 Diff = Norm(DiffVec);
	Assert( R.P0 == Vertex && Diff < 10E-5 );
	v3 PerpVec = Normalize(R.P1 - R.P0);
	Diff = Abs(FaceEdge*PerpVec);
	Assert( Diff < 10E-5 );

	Vertex = V3(-1,-1,-1);
	R = VertexFace( Vertex, Face );
	DiffVec = R.P1 - V3(1/3.f, 1/3.f, 1/3.f);
	Diff = Norm(DiffVec);
	Assert( R.P0 == Vertex && Diff < 10E-5 );
	PerpVec = Normalize(R.P1 - R.P0);
	Diff = Abs(FaceEdge*PerpVec);
	Assert( Diff < 10E-5 );

	Vertex = V3(2,0,0);
	R = VertexFace( Vertex, Face );
	Assert( R.P0 == Vertex && R.P1 == V3(1,0,0) );

	Vertex = V3(0,2,0);
	R = VertexFace( Vertex, Face );
	Assert( R.P0 == Vertex && R.P1 == V3(0,1,0) );

	Vertex = V3(0,0,2);
	R = VertexFace( Vertex, Face );
	Assert( R.P0 == Vertex && R.P1 == V3(0,0,1) );

	#endif 
}

void VerifyEdgeEdge(  )
{
	v3 A0  = V3( -4,0,0);
	v3 A1  = V3(  4,0,0);
	v3 A0H = V3( -4,1,0);
	v3 A1H = V3(  4,1,0);

	//         A0--------A1 
	// B0--B1
	v3 B0 = V3(-10, 0, 0);
	v3 B1 = V3(-6, 0, 0);
	closest_vertex_pair R0  = EdgeEdge( A0,  A1,  B0,  B1 );  // 0000
	closest_vertex_pair R1  = EdgeEdge( B0,  B1,  A0,  A1 );  // 0000
	closest_vertex_pair R0H = EdgeEdge( A0H, A1H, B0,  B1 );  // 0000
	closest_vertex_pair R1H = EdgeEdge( B0,  B1,  A0H, A1H ); // 0000

	Assert(R0.P0  == V3(-4,0,0) && R0.P1  == V3(-6,0,0) );
	Assert(R1.P0  == V3(-6,0,0) && R1.P1  == V3(-4,0,0) );
	Assert(R0H.P0 == V3(-4,1,0) && R0H.P1 == V3(-6,0,0) );
	Assert(R1H.P0 == V3(-6,0,0) && R1H.P1 == V3(-4,1,0) );


	//         A0--------A1 
	//     B0--B1
	B0  = V3(-8, 0, 0);
	B1  = V3(-4, 0, 0);
	R0  = EdgeEdge( A0,  A1,  B0,  B1 );  // 1001
	R1  = EdgeEdge( B0,  B1,  A0,  A1 );  // 0110
	R0H = EdgeEdge( A0H, A1H, B0,  B1 );  // 1001
	R1H = EdgeEdge( B0,  B1,  A0H, A1H ); // 0110
	Assert(R0.P0  == V3(-4,0,0) && R0.P1  == V3(-4,0,0) );
	Assert(R1.P0  == V3(-4,0,0) && R1.P1  == V3(-4,0,0) );
	Assert(R0H.P0 == V3(-4,1,0) && R0H.P1 == V3(-4,0,0) );
	Assert(R1H.P0 == V3(-4,0,0) && R1H.P1 == V3(-4,1,0) );

	//         A0--------A1 
	//      B0--B1
	B0  = V3(-7, 0, 0);
	B1  = V3(-3, 0, 0);
	R0  = EdgeEdge( A0,  A1,  B0,  B1 );  // 1001
	R1  = EdgeEdge( B0,  B1,  A0,  A1 );  // 0110
	R0H = EdgeEdge( A0H, A1H, B0,  B1 );  // 1001
	R1H = EdgeEdge( B0,  B1,  A0H, A1H ); // 0110
	Assert(R0.P0  == V3(-3.5, 0, 0) && R0.P1  == V3(-3.5, 0, 0) );
	Assert(R1.P0  == V3(-3.5, 0, 0) && R1.P1  == V3(-3.5, 0, 0) );
	Assert(R0H.P0 == V3(-3.5, 1, 0) && R0H.P1 == V3(-3.5, 0, 0) );
	Assert(R1H.P0 == V3(-3.5, 0, 0) && R1H.P1 == V3(-3.5, 1, 0) );

	//         A0--------A1 
	//       B0--B1
	B0  = V3(-6, 0, 0);
	B1  = V3(-2, 0, 0);
	R0  = EdgeEdge( A0,  A1,  B0,  B1 );// 1001
	R1  = EdgeEdge( B0,  B1,  A0,  A1 );// 0110
	R0H = EdgeEdge( A0H, A1H, B0,  B1 );// 1001
	R1H = EdgeEdge( B0,  B1,  A0H, A1H );// 0110
	Assert(R0.P0  == V3(-3,0,0) && R0.P1  == V3(-3,0,0) );
	Assert(R1.P0  == V3(-3,0,0) && R1.P1  == V3(-3,0,0) );
	Assert(R0H.P0 == V3(-3,1,0) && R0H.P1 == V3(-3,0,0) );
	Assert(R1H.P0 == V3(-3,0,0) && R1H.P1 == V3(-3,1,0) );


	//         A0--------A1 
	//        B0--B1
	B0  = V3(-5, 0, 0);
	B1  = V3(-1, 0, 0);
	R0  = EdgeEdge( A0,  A1,  B0,  B1 ); // 1001
	R1  = EdgeEdge( B0,  B1,  A0,  A1 ); // 0110
	R0H = EdgeEdge( A0H, A1H, B0,  B1 ); // 1001
	R1H = EdgeEdge( B0,  B1,  A0H, A1H );// 0110
	Assert(R0.P0  == V3(-2.5, 0, 0) && R0.P1  == V3(-2.5, 0, 0) );
	Assert(R1.P0  == V3(-2.5, 0, 0) && R1.P1  == V3(-2.5, 0, 0) );
	Assert(R0H.P0 == V3(-2.5, 1, 0) && R0H.P1 == V3(-2.5, 0, 0) );
	Assert(R1H.P0 == V3(-2.5, 0, 0) && R1H.P1 == V3(-2.5, 1, 0) );

	//         A0--------A1                           As
	//         B0--B1                                 Bs
	B0  = V3(-4, 0, 0);
	B1  = V3( 0, 0, 0);
	R0  = EdgeEdge( A0,  A1,  B0,  B1 );  // (A0,B0)  1011 
	R1  = EdgeEdge( B0,  B1,  A0,  A1 );  // (B0,A0)  1110
	R0H = EdgeEdge( A0H, A1H, B0,  B1 );  // (A0H,B0) 1011
	R1H = EdgeEdge( B0,  B1,  A0H, A1H ); // (B0,A0H) 1110
	// Does not work cause of Point selection
	Assert(R0.P0  == V3(-2,0,0) && R0.P1  == V3(-2,0,0) );
	Assert(R1.P0  == V3(-2,0,0) && R1.P1  == V3(-2,0,0) );
	Assert(R0H.P0 == V3(-2,1,0) && R0H.P1 == V3(-2,0,0) );
	Assert(R1H.P0 == V3(-2,0,0) && R1H.P1 == V3(-2,1,0) );

	//         A0--------A1 
	//          B0--B1
	B0  = V3(-3, 0, 0);
	B1  = V3( 1, 0, 0);
	R0  = EdgeEdge( A0,  A1,  B0,  B1 );  // ([-3.5,0,0], [-3,  0,0]) 0011
	R1  = EdgeEdge( B0,  B1,  A0,  A1 );  // ([-3  ,0,0], [-3.5,0,0]) 1100
	R0H = EdgeEdge( A0H, A1H, B0,  B1 );  // ([-3.5,1,0], [-3,  0,0]) 0011
	R1H = EdgeEdge( B0,  B1,  A0H, A1H ); // ([-3  ,0,0], [-3.5,1,0]) 1100
	// Does not work cause of averages.
 	Assert(R0.P0  == V3(-1,0,0) && R0.P1  == V3(-1,0,0) );
 	Assert(R1.P0  == V3(-1,0,0) && R1.P1  == V3(-1,0,0) );
 	Assert(R0H.P0 == V3(-1,1,0) && R0H.P1 == V3(-1,0,0) );
 	Assert(R1H.P0 == V3(-1,0,0) && R1H.P1 == V3(-1,1,0) );

  
	//         A0--------A1 
	//            B0--B1
	B0  = V3(-2, 0, 0);
	B1  = V3( 2, 0, 0);
	R0  = EdgeEdge( A0,  A1,  B0,  B1 );  // ([-3,0,0], [-2,0,0]) 0011
	R1  = EdgeEdge( B0,  B1,  A0,  A1 );  // ([-2,0,0], [-3,0,0]) 1100
	R0H = EdgeEdge( A0H, A1H, B0,  B1 );  // ([-3,1,0], [-2,0,0]) 0011
	R1H = EdgeEdge( B0,  B1,  A0H, A1H ); // ([-2,0,0], [-3,1,0]) 1100
	// Does not work cause of averages.
	Assert(R0.P0  == V3(0,0,0) && R0.P1  == V3(0,0,0) );
	Assert(R1.P0  == V3(0,0,0) && R1.P1  == V3(0,0,0) );
	Assert(R0H.P0 == V3(0,1,0) && R0H.P1 == V3(0,0,0) );
	Assert(R1H.P0 == V3(0,0,0) && R1H.P1 == V3(0,1,0) );

	//         A0--------A1 
	//              B0--B1
	B0  = V3(-1, 0, 0);
	B1  = V3( 3, 0, 0);
	R0  = EdgeEdge( A0,  A1,  B0,  B1 );  // ([1.5,0,0], [1,  0,0]) 0011
	R1  = EdgeEdge( B0,  B1,  A0,  A1 );  // ([1   0,0], [1.5,0,0]) 1100
	R0H = EdgeEdge( A0H, A1H, B0,  B1 );  // ([1.5,1,0], [1   0,0]) 0011
	R1H = EdgeEdge( B0,  B1,  A0H, A1H ); // ([1,  0,0], [1.5,1,0]) 1100
	// Does not work cause of averages.
	Assert(R0.P0  == V3(1,0,0) && R0.P1  == V3(1,0,0) );
	Assert(R1.P0  == V3(1,0,0) && R1.P1  == V3(1,0,0) );
	Assert(R0H.P0 == V3(1,1,0) && R0H.P1 == V3(1,0,0) );
	Assert(R1H.P0 == V3(1,0,0) && R1H.P1 == V3(1,1,0) );

  
	//         A0--------A1                     Ae
	//               B0--B1                     Bs
	B0  = V3( 0, 0, 0);
	B1  = V3( 4, 0, 0);
	R0  = EdgeEdge( A0,  A1,  B0,  B1 ); // 0111
	R1  = EdgeEdge( B0,  B1,  A0,  A1 ); // 1101
	R0H = EdgeEdge( A0H, A1H, B0,  B1 ); // 0111
	R1H = EdgeEdge( B0,  B1,  A0H, A1H );// 1101
	Assert(R0.P0  == V3( 2,0,0) && R0.P1  == V3( 2,0,0) );
	Assert(R1.P0  == V3( 2,0,0) && R1.P1  == V3( 2,0,0) );
	Assert(R0H.P0 == V3( 2,1,0) && R0H.P1 == V3( 2,0,0) );
	Assert(R1H.P0 == V3( 2,0,0) && R1H.P1 == V3( 2,1,0) );
 
	//         A0--------A1 
	//                B0--B1
	B0  = V3( 1, 0, 0);
	B1  = V3( 5, 0, 0);
	R0  = EdgeEdge( A0,  A1,  B0,  B1 );  // 0110
	R1  = EdgeEdge( B0,  B1,  A0,  A1 );  // 1001
	R0H = EdgeEdge( A0H, A1H, B0,  B1 );  // 0110
	R1H = EdgeEdge( B0,  B1,  A0H, A1H ); // 1001
	Assert(R0.P0  == V3( 2.5,0,0) && R0.P1  == V3( 2.5,0,0) );
	Assert(R1.P0  == V3( 2.5,0,0) && R1.P1  == V3( 2.5,0,0) );
	Assert(R0H.P0 == V3( 2.5,1,0) && R0H.P1 == V3( 2.5,0,0) );
	Assert(R1H.P0 == V3( 2.5,0,0) && R1H.P1 == V3( 2.5,1,0) );
  
	//         A0--------A1 
	//                 B0--B1
	B0  = V3( 2, 0, 0);
	B1  = V3( 6, 0, 0);
	R0  = EdgeEdge( A0,  A1,  B0,  B1 );  // 0110
	R1  = EdgeEdge( B0,  B1,  A0,  A1 );  // 1001
	R0H = EdgeEdge( A0H, A1H, B0,  B1 );  // 0110
	R1H = EdgeEdge( B0,  B1,  A0H, A1H ); // 1001
	Assert(R0.P0  == V3( 3,0,0) && R0.P1  == V3( 3,0,0) );
	Assert(R1.P0  == V3( 3,0,0) && R1.P1  == V3( 3,0,0) );
	Assert(R0H.P0 == V3( 3,1,0) && R0H.P1 == V3( 3,0,0) );
	Assert(R1H.P0 == V3( 3,0,0) && R1H.P1 == V3( 3,1,0) );

		//         A0--------A1 
	//                      B0--B1
	B0  = V3( 3, 0, 0);
	B1  = V3( 7, 0, 0);
	R0  = EdgeEdge( A0,  A1,  B0,  B1 );  // 0110
	R1  = EdgeEdge( B0,  B1,  A0,  A1 );  // 1001
	R0H = EdgeEdge( A0H, A1H, B0,  B1 );  // 0110
	R1H = EdgeEdge( B0,  B1,  A0H, A1H ); // 1001
	Assert(R0.P0  == V3(3.5,0,0) && R0.P1  == V3(3.5,0,0) );
	Assert(R1.P0  == V3(3.5,0,0) && R1.P1  == V3(3.5,0,0) );
	Assert(R0H.P0 == V3(3.5,1,0) && R0H.P1 == V3(3.5,0,0) );
	Assert(R1H.P0 == V3(3.5,0,0) && R1H.P1 == V3(3.5,1,0) );
  
	//         A0--------A1 
	//                   B0--B1
	B0  = V3( 4, 0, 0);
	B1  = V3( 6, 0, 0);
	R0  = EdgeEdge( A0,  A1,  B0,  B1 );  // 0110
	R1  = EdgeEdge( B0,  B1,  A0,  A1 );  // 1001
	R0H = EdgeEdge( A0H, A1H, B0,  B1 );  // 0110
	R1H = EdgeEdge( B0,  B1,  A0H, A1H ); // 1001
	Assert(R0.P0  == V3(4,0,0) && R0.P1  == V3(4,0,0) );
	Assert(R1.P0  == V3(4,0,0) && R1.P1  == V3(4,0,0) );
	Assert(R0H.P0 == V3(4,1,0) && R0H.P1 == V3(4,0,0) );
	Assert(R1H.P0 == V3(4,0,0) && R1H.P1 == V3(4,1,0) );
  
 	//         A0--------A1 
	//                      B0--B1
	B0  = V3( 6, 0, 0);
	B1  = V3( 10, 0, 0);
	R0  = EdgeEdge( A0,  A1,  B0,  B1 );  // 0000
	R1  = EdgeEdge( B0,  B1,  A0,  A1 );  // 0000
	R0H = EdgeEdge( A0H, A1H, B0,  B1 );  // 0000
	R1H = EdgeEdge( B0,  B1,  A0H, A1H ); // 0000
	Assert(R0.P0  == V3(4,0,0) && R0.P1  == V3(6,0,0) );
	Assert(R1.P0  == V3(6,0,0) && R1.P1  == V3(4,0,0) );
	Assert(R0H.P0 == V3(4,1,0) && R0H.P1 == V3(6,0,0) );
	Assert(R1H.P0 == V3(6,0,0) && R1H.P1 == V3(4,1,0) );


 	//         A0--------A1 
	//         B0--------B1
	B0  = V3( -4, 0, 0);
	B1  = V3(  4, 0, 0);
	R0  = EdgeEdge( A0,  A1,  B0,  B1 );  // 0000
	R1  = EdgeEdge( B0,  B1,  A0,  A1 );  // 0000
	R0H = EdgeEdge( A0H, A1H, B0,  B1 );  // 0000
	R1H = EdgeEdge( B0,  B1,  A0H, A1H ); // 0000
	Assert(R0.P0  == V3(0,0,0) && R0.P1  == V3(0,0,0) );
	Assert(R1.P0  == V3(0,0,0) && R1.P1  == V3(0,0,0) );
	Assert(R0H.P0 == V3(0,1,0) && R0H.P1 == V3(0,0,0) );
	Assert(R1H.P0 == V3(0,0,0) && R1H.P1 == V3(0,1,0) );

	B0 = V3(0,-4,0);
	B1 = V3(0, 4,0);
	R0  = EdgeEdge( A0,  A1,  B0,  B1 ); 
	R1  = EdgeEdge( B0,  B1,  A0,  A1 ); 
	R0H = EdgeEdge( A0H, A1H, B0,  B1 ); 
	R1H = EdgeEdge( B0,  B1,  A0H, A1H );
	Assert(R0.P0  == V3(0,0,0) && R0.P1  == V3(0,0,0) );
	Assert(R1.P0  == V3(0,0,0) && R1.P1  == V3(0,0,0) );
	Assert(R0H.P0 == V3(0,1,0) && R0H.P1 == V3(0,1,0) );
	Assert(R1H.P0 == V3(0,1,0) && R1H.P1 == V3(0,1,0) );


	A0  = V3(-2,  5,  5);
	A1  = V3( 2,  5,  5);
	A0H = V3(-2,  5,  6);
	A1H = V3( 2,  5,  6);

	//     B0(S)
	//    /|
	//   / |
	// B1S B1
	//        A0----A1
	B0  = V3( -4, 10, 5);
	B1  = V3( -4,  6, 5);
	v3 B0S = V3( -4, 10, 5);
	v3 B1S = V3( -8,  6, 5);

	R0  = EdgeEdge( A0,  A1,  B0,  B1 ); 
	R1  = EdgeEdge( B0,  B1,  A0,  A1 ); 
	closest_vertex_pair R2  = EdgeEdge( A1,  A0,  B0,  B1 ); 
	closest_vertex_pair R3  = EdgeEdge( B1,  B0,  A0,  A1 ); 
	R0H = EdgeEdge( A0H, A1H, B0,  B1 ); 
	R1H = EdgeEdge( B0,  B1,  A0H, A1H );
	closest_vertex_pair R0S  = EdgeEdge( A1,  A0,  B0S,  B1S ); 
	closest_vertex_pair R1S  = EdgeEdge( B1S,  B0S,  A0,  A1 ); 
	closest_vertex_pair R2S  = EdgeEdge( A1,  A0,  B0S+V3(12,0,0),  B1S+V3(12,0,0) ); 
	closest_vertex_pair R3S  = EdgeEdge( B0S+V3(6,0,0),  B1S+V3(6,0,0),  A0,  A1 ); 
	Assert(R0.P0  == V3(-2,5,5) && R0.P1  == V3(-4,6,5) );
	Assert(R1.P0  == V3(-4,6,5) && R1.P1  == V3(-2,5,5) );
	Assert(R2.P0  == V3(-2,5,5) && R2.P1  == V3(-4,6,5) );
	Assert(R3.P0  == V3(-4,6,5) && R3.P1  == V3(-2,5,5) );
	Assert(R0H.P0 == V3(-2,5,6) && R0H.P1 == V3(-4,6,5) );
	Assert(R1H.P0 == V3(-4,6,5) && R1H.P1 == V3(-2,5,6) );
	Assert(R0S.P0 == A0 && R0S.P1 == V3(-5.5,8.5,5) );
	Assert(R1S.P0 == V3(-5.5,8.5,5) && R1S.P1 == A0 );
	Assert(R2S.P0 == A1 && R2S.P1 == V3(4,6,5) );
	Assert(R3S.P0 == V3(-2,6,5) && R3S.P1 == A0 );




	// CrissCross meeting in middle
	A0 = V3(-1,-1,-1);
	A1 = V3( 1,1, 1);
	B0 = V3(-1,  1, -1);
	B1 = V3( 1, -1,  1);
	R0 = EdgeEdge( A0, A1, B0, B1 );
	r32 Diff1 = Norm(R0.P0);
	r32 Diff2 = Norm(R0.P1);
	r32 Tol = 10E-7;
	Assert( Diff1 < Tol && Diff2 < Tol);


	// CrissCross a bit above
	A0 = V3(-1,-1,-1);
	A1 = V3( 1,1, 1);
	B0 = V3(-1,  2,  1);
	B1 = V3( 1,  0, -1);
	R0 = EdgeEdge( A0, A1, B0, B1 );
	Assert(R0.P0 == V3(1/4.f, 1/4.f, 1/4.f)  )
	

}

void VerifyEdgeFace( memory_arena* Arena )
{

	temporary_memory TempMem = BeginTemporaryMemory( Arena );

	list<v3> Face = list<v3>(Arena);

	// Square X-Y Plane Z=0
	Face.InsertAfter( V3(-1,-1,0) );
	Face.InsertAfter( V3(1,-1,0) );
	Face.InsertAfter( V3(1,1,0) );
	Face.InsertAfter( V3(-1,1,0) );


	EndTemporaryMemory(TempMem);
	Assert(false);
}

void VerifyFaceFace( memory_arena* Arena )
{
	temporary_memory TempMem = BeginTemporaryMemory( Arena );

	list<v3> Face = list<v3>(Arena);

	// Square X-Y Plane Z=0
	Face.InsertAfter( V3(-1,-1,0) );
	Face.InsertAfter( V3(1,-1,0) );
	Face.InsertAfter( V3(1,1,0) );
	Face.InsertAfter( V3(-1,1,0) );

	
	EndTemporaryMemory(TempMem);
	Assert(false);
}