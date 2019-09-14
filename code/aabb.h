#ifndef RECT_H
#define RECT_H

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
// 	1  [F]aces
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
 *        				V3_________________________________V2
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

v3 GetAABB2fEdgeNormal( u32 EdgeIndex )
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

void GetAABBVertices( aabb2f* AABB, aabb_feature_vertex* AABBVertices )
{
	AABBVertices[0] = V3( AABB->P0.X, AABB->P0.Y, 0 );
	AABBVertices[1] = V3( AABB->P1.X, AABB->P0.Y, 0 );
	AABBVertices[2] = V3( AABB->P1.X, AABB->P1.Y, 0 );
	AABBVertices[3] = V3( AABB->P0.X, AABB->P1.Y, 0 );
}

list<aabb_feature_vertex> GetAABBVertices( memory_arena* Arena, aabb2f* AABB )
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


list<aabb_feature_edge> GetAABBEdges( memory_arena* Arena, aabb2f* AABB )
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

aabb_feature_face GetAABBFace( aabb2f* AABB )
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
// 	6  [F]aces
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
b32 HasWidth( aabb3f* AABB )
{
	r32 Tol = 1E-5;
	b32 HasWidth  = Abs( AABB->P1.X - AABB->P0.X) > Tol;
	return HasWidth;
}
b32 LacksWidth( aabb3f* AABB )
{
	return ! HasWidth(AABB);
}

b32 HasHeight( aabb3f* AABB )
{
	r32 Tol = 1E-5;
	b32 HasHeight = Abs( AABB->P1.Y - AABB->P0.Y) > Tol;
	return HasHeight;
}
b32 LacksHeight( aabb3f* AABB )
{
	return ! HasHeight(AABB);
}
b32 HasDepth( aabb3f* AABB )
{
	r32 Tol = 1E-5;
	b32 HasDepth  = Abs( AABB->P1.Z - AABB->P0.Z) > Tol;
	return HasDepth;
}
b32 LacksDepth( aabb3f* AABB )
{
	return !HasDepth(AABB);
}

void GetAABBVertices(aabb3f* AABB, aabb_feature_vertex* AABBVertices)
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
GetAABBCenter( const aabb3f AABB )
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

	v3 PenetrationDepth = ABSeparation - ContactSeparation;
	
	return PenetrationDepth;	
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
	Result.Type = AABB_CONTACT_TYPE_SEPARATE;

	Result.PenetrationDepth  = GetPenetrationDepth( A, B );

	if( ( Result.PenetrationDepth.X < 0 ) || 
		( Result.PenetrationDepth.Y < 0 ) ||
		( Result.PenetrationDepth.Z < 0 ) )
	{
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
bool SweeptAABB( aabb3f& a, aabb3f& b, v3& v, v3& outVel, v3& hitNormal )
{
    //Initialise out info
    outVel = v;
    hitNormal = V3(0,0,0);

	if( (v.X == 0) && (v.Y == 0) )
	{
		return false;
	}

    // Treat b as stationary, so invert v to get relative velocity
	v3 rv = -v;

    r32 hitTime = 0.0f;
    r32 outTime = 1.0f;
    v3 overlapTime = V3(0,0,0);

    // A is traveling to the right relative B
    if( rv.X < 0 )
    {	
    	// A is to the right of B ( They are separating )
        if( b.P1.X <= a.P0.X )
        { 
        	return false;
        }

        // Left edge of A is to the left of Bs right edge, potentially overlapping and closing in on eachother.
        if( b.P1.X > a.P0.X ) 
        {
    		outTime = Minimum( (a.P0.X - b.P1.X) / rv.X, outTime );
    	}

        // A is to the left of B with no overlap and closing in on eachother.
        if( a.P1.X <= b.P0.X )
        {
            overlapTime.X = (a.P1.X - b.P0.X) / rv.X;
            hitTime = Maximum(overlapTime.X, hitTime);
        }
    }
    // A is traveling to the left relative B
    else if( rv.X > 0 )
    {
    	// A is to the left of B ( They are separating )
        if( b.P0.X >= a.P1.X )
        {
        	return false;
        }

        // Right edge of A is to the right of Bs left edge, potentially overlapping and closing in on eachother.
        if( a.P1.X > b.P0.X ) 
        { 
        	outTime = Minimum( (a.P1.X - b.P0.X) / rv.X, outTime );
        }

        // A is to the right of B with no overlap and closing in on eachother.
        if( b.P1.X <= a.P0.X )
        {
            overlapTime.X = (a.P0.X - b.P1.X) / rv.X;
            hitTime = Maximum(overlapTime.X, hitTime);
        }
    }

    if( hitTime > outTime )
    {
    	return false;
    }

    //=================================

    // A is traveling up relative to B
    if( rv.Y < 0 )
    {
    	// A is above B and separating
        if( b.P1.Y <= a.P0.Y )
        {
        	return false;
        }

        // Bottom of A is below top of B, potentially overlapping and closing in on eachother.
        if( b.P1.Y > a.P0.Y ) 
        { 
        	outTime = Minimum( (a.P0.Y - b.P1.Y) / rv.Y, outTime );
        }

        // A is below B with no overlap and closing in on eachother.
        if( a.P1.Y <= b.P0.Y )
        {
            overlapTime.Y = (a.P1.Y - b.P0.Y) / rv.Y;
            hitTime = Maximum(overlapTime.Y, hitTime);
        }           
    }
    // A is traveling down relative to B
    else if( rv.Y > 0 )
    {
    	// A is below B and separating
        if( b.P0.Y >= a.P1.Y )
        {
        	return false;
        }
       
        // Top of A is above bottom of B, potentially overlapping and closing in on eachother.
        if( a.P1.Y > b.P0.Y ) 
        { 
        	outTime = Minimum( (a.P1.Y - b.P0.Y) / rv.Y, outTime );
        }

        // A is above B with no overlap and closing in on eachother.
        if( b.P1.Y <= a.P0.Y )
        {
            overlapTime.Y = (a.P0.Y - b.P1.Y) / rv.Y;
            hitTime = Maximum(overlapTime.Y, hitTime);
        }
    }

    if( hitTime > outTime )
    {
    	return false;
    }

    // Scale resulting velocity by normalized hit time
    outVel = -rv * hitTime;
	r32 Tol = 0.01;

    // Hit normal is along axis with the highest overlap time
	if( overlapTime.X > overlapTime.Y )
    {
    	if( rv.X  > 0 )
    	{
    		hitNormal = V3(1, 0, 0);
    	}else if(rv.X  < 0){
    		hitNormal = V3(-1, 0, 0);
    	}else{
    		Assert(0);
    	}
    }
    else if( overlapTime.X < overlapTime.Y )
    {
    	if( rv.Y  > 0 )
    	{
    		hitNormal = V3(0, 1, 0);
    	}else if( rv.Y  < 0 ){
    		hitNormal = V3(0, -1, 0);
    	}else{
    		Assert(0);
    	}
	}else{
		if( rv.X  > 0 )
    	{
    		hitNormal = V3(1, 0, 0);
    	}else if(rv.X  < 0){
    		hitNormal = V3(-1, 0, 0);
    	}else if( rv.Y  > 0 )
    	{
    		hitNormal = V3(0, 1, 0);
    	}else if( rv.Y  < 0 ){
    		hitNormal = V3(0, -1, 0);
		}else{
			Assert(0);
		}		
	}

    return true;
}



#endif // AABB_H