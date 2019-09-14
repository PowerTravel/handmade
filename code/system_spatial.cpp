
#include "vector_math.h"
#include "utility_macros.h"
#include "aabb.h"

#if 0
struct closest_vertex_pair
{
	v3 P0;
	v3 P1;
};

//** AABB3f Intersection Functions **//

// 6 Types of interactions  
//		(V,V):   Trivial (Pv,Pv) (Unimplemented)
//      (V,E):   (Pv, Pe = o + [(v-o) * u] u)  v=Pv, u = d-o, d,o = end points of Edge.
//      (V,F):   (Pv, Pf = v - [(v-f) * n] n)  v=Pv, n = FaceNormal, f = arbitrary point on F.
// 		(E1,E2): (Pe1 = o1 + [ (o2-o1) * (u1-ku2) ) / (1-k^2) ] u1;   k = u1*u2
//                With Pe1 in hand we apply the (V,E) case to find Pe2
//      (E,F):   -> {(O,F),(D,F),(E,E1),(E,E2),...,(E,En)},  O,V is Enpoints of E; E1,E2 ... En are edges of F (Unimplemented)
//		(F1,F2): -> {(E01,F2),(E11,F2), ... ,(En0,F2),(F1,E02),(F1,E12) ... ,(F1,En2) } (Unimplemented)

// 		Redundancy:
//      (V,V) -> (V,E)
//		(E,F) -> (V,F)
//		(F,F) -> (E,F) -> (V,F), (E,F)

// 		3 Elementary intersections
//      (V,E)
//		(V,F)
// 		(E,E)

v3 VertexVertex( v3 VertexA, v3 VertexB )
{
	v3 Result = (VertexA + VertexB)/2;
	return Result;
}


closest_vertex_pair VertexEdge( v3& Vertex, v3& EedgePointA, v3& EdgePointB)
{
	v3 o = EedgePointA;
	v3 d = EdgePointB;
	v3 u = Normalize(d-o);
	r32 uNorm = Norm(d-o);
	v3 v = Vertex;

	r32 ProjectionScalar = (v-o)*u;
	v3 ClosestPointOnEdge = {};
	if(ProjectionScalar <= 0)
	{
		ClosestPointOnEdge = o;
	}else if(ProjectionScalar >= uNorm )
	{
		ClosestPointOnEdge = d;
	}else{
		ClosestPointOnEdge = o + ProjectionScalar * u;
	}

	closest_vertex_pair Result = {};
	Result.P0 = v;
	Result.P1 = ClosestPointOnEdge;
	return Result;
}

closest_vertex_pair VertexEdge( aabb_feature_vertex& Vertex, aabb_feature_edge& Edge )
{
	return VertexEdge( Vertex, Edge.P0, Edge.P1);
}

b32 IsVertexInsideFace( aabb_feature_vertex& Vertex, v3& FaceNormal, aabb_feature_face& Face )
{
	u32 FacePointCount = ArrayCount(Face.P);
	Assert(FacePointCount == 4);
	for( u32 IndexP0 = 0;
		IndexP0 < FacePointCount;
		++IndexP0 )
	{
		u32 IndexP1 = IndexP0+1;
		if( IndexP1 == FacePointCount )
		{
			IndexP1 = 0;
		}

		v3 v = Vertex - Face.P[IndexP0];
		v3 e = Face.P[IndexP1]   - Face.P[IndexP0];
		v3 x = CrossProduct(e,v);
		if( (x * FaceNormal) <= 0 )
		{
			return false;
		}
	}

	return true;
}


closest_vertex_pair VertexFace( aabb_feature_vertex& Vertex, aabb_feature_face& Face )
{
	v3 FaceEdge0 = Face.P1 - Face.P0;
	v3 FaceEdge1 = Face.P2 - Face.P1;
	v3 FaceNormal = Normalize( CrossProduct( FaceEdge0, FaceEdge1 ) );

	r32 ProjectionScalar = (Vertex - Face.P0) * FaceNormal;

	v3 ClosestPoint = Vertex - ProjectionScalar * FaceNormal;

	closest_vertex_pair Result = {};
	if( IsVertexInsideFace( ClosestPoint, FaceNormal, Face ) )
	{
		Result.P0 = Vertex;
		Result.P1 = ClosestPoint;

	}else{

		Result = VertexEdge(Vertex, Face.P0, Face.P1);;
		r32 ShortestContactDistance = Norm( Result.P1 - Result.P0 );
		
		closest_vertex_pair ClosestPointCandidates = VertexEdge(Vertex, Face.P1, Face.P2);
		r32 ContactDistance = Norm( ClosestPointCandidates.P1 - ClosestPointCandidates.P0 );
		if( ContactDistance < ShortestContactDistance )
		{
			ShortestContactDistance = ContactDistance;
			Result = ClosestPointCandidates;
		}
		
		ClosestPointCandidates = VertexEdge(Vertex, Face.P2, Face.P3);
		ContactDistance = Norm( ClosestPointCandidates.P1 - ClosestPointCandidates.P0 );
		if( ContactDistance < ShortestContactDistance )
		{
			ShortestContactDistance = ContactDistance;
			Result = ClosestPointCandidates;
		}
		
		ClosestPointCandidates = VertexEdge(Vertex, Face.P3, Face.P0);
		ContactDistance = Norm( ClosestPointCandidates.P1 - ClosestPointCandidates.P0 );
		if( ContactDistance < ShortestContactDistance )
		{
			ShortestContactDistance = ContactDistance;
			Result = ClosestPointCandidates;
		}
	}
	
	return Result;
}


closest_vertex_pair EdgeEdge( v3 EdgeAStart, v3 EdgeAEnd, v3 EdgeBStart, v3 EdgeBEnd )
{
	Assert(EdgeAStart != EdgeAEnd);
	Assert(EdgeBStart != EdgeBEnd);

	v3& o1 = EdgeAStart;
	v3& d1 = EdgeAEnd;
	v3 u1 = Normalize(d1-o1);

	v3& o2 = EdgeBStart;
	v3& d2 = EdgeBEnd;
	v3 u2 = Normalize(d2-o2);

	r32 k = u1*u2;


	closest_vertex_pair Result = {};
	
	r32 Tol = 10E-7;

	// Lines are not parallel, unique point exists
	if( Abs(Abs(k)-1) >= Tol )
	{
		r32 Scalar = ( (o2-o1) * (u1 - k * u2) ) / (1 - k*k);

		// This point is the intersection point between the RAYS A and B
		v3 ClosestPointOnRay = o1 + Scalar * u1;
		

		// Check if ClosestPointOnRay lies on the LINE SEGMENT A or B
		v3 no1 = ClosestPointOnRay - o1;
		r32 so1 = no1 * u1;
		v3 nd1 = ClosestPointOnRay - d1;
		r32 sd1 = nd1 * u1;

		v3 no2 = ClosestPointOnRay - o2;
		r32 so2 = no2 * u2;
		v3 nd2 = ClosestPointOnRay - d2;
		r32 sd2 = nd2 * u2;


		b32 IsOnLineSegmentA = ( (so1 >= 0) && (sd1 <= 0) );

		b32 IsOnLineSegmentB = ( (so2 >= 0) && (sd2 <= 0) );

		if( IsOnLineSegmentA && IsOnLineSegmentB )
		{
			// Point is on Line Segment A
			Result = VertexEdge( ClosestPointOnRay, EdgeBStart, EdgeBEnd );
		}else{
			
			// Point is not on any of the line segments
			closest_vertex_pair A0B = VertexEdge( EdgeAStart, EdgeBStart, EdgeBEnd);
			r32 ShortestContactDistance = Norm( A0B.P1 - A0B.P0 );
			Result = A0B;

			closest_vertex_pair A1B = VertexEdge( EdgeAEnd,   EdgeBStart, EdgeBEnd);
			r32 ContactDistance = Norm( A1B.P1 - A1B.P0 );
			if( ContactDistance < ShortestContactDistance )
			{
				ShortestContactDistance = ContactDistance;
				Result = A1B;
			}

			closest_vertex_pair B0A = VertexEdge( EdgeBStart, EdgeAStart, EdgeAEnd);
			ContactDistance = Norm( B0A.P1 - B0A.P0 );
			if( ContactDistance < ShortestContactDistance )
			{
				ShortestContactDistance = ContactDistance;
				Result.P0 = B0A.P1;
				Result.P1 = B0A.P0;
			}

			closest_vertex_pair B1A = VertexEdge( EdgeBEnd,   EdgeAStart, EdgeAEnd);
			ContactDistance = Norm( B1A.P1 - B1A.P0 );
			if( ContactDistance < ShortestContactDistance )
			{
				ShortestContactDistance = ContactDistance;
				Result.P0 = B1A.P1;
				Result.P1 = B1A.P0;
			}
		}

	// Lines are parallel, may not have a unique contact point
	}else{

		if(k < 0)
		{
			// Note (Jakob): This aligns A with B.
			//				 Not sure if it's necessary, all unit tests
			//				 works without it but I can't know for certain
			// 				 if omitting this step will cause unforseen 
			//				 asymmetry problems so just for saftey we align them.
			v3 TmpPoint = EdgeBStart;
			EdgeBStart = EdgeBEnd;
			EdgeBEnd = TmpPoint;
		}

		closest_vertex_pair A0B = VertexEdge( EdgeAStart, EdgeBStart, EdgeBEnd);
		v3 A0Bn = A0B.P1-A0B.P0;
		b32 A0Bp = Abs(A0Bn * u1) < Tol;

		closest_vertex_pair A1B = VertexEdge( EdgeAEnd,   EdgeBStart, EdgeBEnd);
		v3 A1Bn = A1B.P1-A1B.P0;
		b32 A1Bp = Abs(A1Bn * u1) < Tol;

		closest_vertex_pair B0A = VertexEdge( EdgeBStart, EdgeAStart, EdgeAEnd);
		v3 B0An = B0A.P1-B0A.P0;
		b32 B0Ap = Abs(B0An * u2) < Tol;

		closest_vertex_pair B1A = VertexEdge( EdgeBEnd,   EdgeAStart, EdgeAEnd);
		v3 B1An = B1A.P1-B1A.P0;
		b32 B1Ap = Abs(B1An * u2) < Tol;

		// Five different cases when lines are parallel
		if( ! (A0Bp || A1Bp || B0Ap || B1Ap) )
		{
			// Separate:
			//           |--A--|
			//  |--B--|

			r32 A0BMinDist = Norm( A0Bn );
			r32 A1BMinDist = Norm( A1Bn );
			if( A0BMinDist < A1BMinDist )
			{
				Result = A0B;
			}else{
				Result = A1B;
			}
		}else if(A0Bp && A1Bp){

			//	A inside B
			//      |----A----|
			//  |--------B--------|
			Result.P0 = (A0B.P0 + A1B.P0)/2;
			Result.P1 = (A0B.P1 + A1B.P1)/2;
		}else if(B0Ap && B1Ap){

			//  B inside A
			//  |--------A--------|
			//      |----B----|
			Result.P0 = (B0A.P1 + B1A.P1)/2;
			Result.P1 = (B0A.P0 + B1A.P0)/2;
		}else if(A0Bp && B1Ap){

			// B to the left of A
			//      |----A----|
			// |----B----|
			Result.P0 = (A0B.P0 + B1A.P1)/2;
			Result.P1 = (A0B.P1 + B1A.P0)/2;
		}else if(B0Ap && A1Bp){

			// B to the Right of A
			// |----A----|
			//      |----B----|
			Result.P0 = (B0A.P1 + A1B.P0)/2; 
			Result.P1 = (B0A.P0 + A1B.P1)/2;
		}else{
			INVALID_CODE_PATH
		}

	}

	return Result;
}


closest_vertex_pair EdgeEdge( aabb_feature_edge& Edge0,  aabb_feature_edge& Edge1 )
{
	return EdgeEdge( Edge0.P0, Edge0.P1, Edge1.P0, Edge1.P1 );
}

/*
Not Tested
closest_vertex_pair EdgeFace( v3 EdgeStart, v3 EdgeEnd, list<v3>* Face )
{
	Assert(Face->GetSize() >= 3);
	closest_vertex_pair  ClosestPoints   = VertexFace(EdgeStart, Face);

	r32 ShortestContactDistance = Norm( ClosestPoints.P1 - ClosestPoints.P0 );

	closest_vertex_pair  ClosestPointCandidates = VertexFace(EdgeEnd, Face);
	r32 ShortestDistanceCandidate = Norm( ClosestPointCandidates.P1 - ClosestPointCandidates.P0 );

	ClosestPoints           = (ShortestDistanceCandidate < ShortestContactDistance) ? ClosestPointCandidates     : ClosestPoints;
	ShortestContactDistance = (ShortestDistanceCandidate < ShortestContactDistance) ? ShortestDistanceCandidate  : ShortestContactDistance; 

	Face->First();
	v3 FacePoint1 = Face->Get();
	Face->Next();
	v3 FacePoint2 = Face->Get();
	Face->Next();
	while( !Face->IsEnd() )
	{
		ClosestPointCandidates = EdgeEdge(EdgeStart, EdgeEnd, FacePoint1, FacePoint2);

		r32 ContactDistance = Norm( ClosestPointCandidates.P1 - ClosestPointCandidates.P0 );

		if( ContactDistance <  ShortestContactDistance )
		{
			ShortestContactDistance = ContactDistance;
			ClosestPoints = ClosestPointCandidates;
		}
		
		FacePoint1 = FacePoint2;
		FacePoint2 = Face->Get();
		Face->Next();
	}

	return ClosestPoints;
}

closest_vertex_pair FaceFace( list<v3>* Face1, list<v3>* Face2 )
{
	Assert(Face1->GetSize() >= 3);
	Assert(Face2->GetSize() >= 3);

	Face1->First();
	v3 FacePoint11 = Face1->Get();
	Face1->Next();
	v3 FacePoint12 = Face1->Get();
	Face1->Next();

	Face2->First();
	v3 FacePoint21 = Face2->Get();
	Face2->Next();
	v3 FacePoint22 = Face2->Get();
	Face2->Next();

	closest_vertex_pair ClosestPoints = {};
	r32 ClosestDistance = 10e10;

	while( !Face1->IsEnd() )
	{

		while( !Face2->IsEnd() )
		{

			closest_vertex_pair ClosestPointCandidates = EdgeEdge(FacePoint11, FacePoint12, FacePoint21, FacePoint22);

			 /* Find a metric for min Distance, See uniqueness Porblem in book */
			r32 ContactDistance = Norm( ClosestPointCandidates.P1 - ClosestPointCandidates.P0 );


			if( ContactDistance <  ClosestDistance )
			{
				ClosestDistance = ContactDistance;
				ClosestPoints = ClosestPointCandidates;
			}
			
			FacePoint21 = FacePoint22;
			FacePoint22 = Face2->Get();
			Face2->Next();
		}


		FacePoint11 = FacePoint12;
		FacePoint12 = Face1->Get();
		Face1->Next();
	}

	return ClosestPoints;
}
*/
u32 GetMaxVelocityIndex(v3 V)
{
	u32 Result = 0;
	r32 Val = V.X;
	if( Abs(V.Y) > Abs(V.X) )
	{
		Result = 1;
		Val = V.Y;
	}

	if(Abs(Val) < Abs(V.Z))
	{
		Result = 2;
		Val = V.Z;	
	}

	return Result;
}

v3 GetCollisionNormal(v3& Velocity, aabb_feature_vertex& PrincipalContactPoint, aabb_feature_face& Face)
{
	aabb_feature_edge IntersectionEdge = {};
	r32 ShortestDistance = 10E10;

	if( Velocity * V3(1,0,0) != 0)
	{

		// Left
		aabb_feature_edge FaceEdge = {};				
		FaceEdge.P0 = Face.P3;
		FaceEdge.P1 = Face.P0;
		closest_vertex_pair ContactPointCandidate = VertexEdge( PrincipalContactPoint, FaceEdge );
		r32 ShortestDistanceCandidate = Norm(ContactPointCandidate.P1 - ContactPointCandidate.P0);
		if( ShortestDistanceCandidate < ShortestDistance)
		{
			ShortestDistance = ShortestDistanceCandidate;
			IntersectionEdge = FaceEdge;
		}

	
		// Right
		FaceEdge.P0 = Face.P1;
		FaceEdge.P1 = Face.P2;
		ContactPointCandidate = VertexEdge( PrincipalContactPoint, FaceEdge );
		ShortestDistanceCandidate  = Norm(ContactPointCandidate.P1 - ContactPointCandidate.P0);
		if( ShortestDistanceCandidate < ShortestDistance)
		{
			ShortestDistance = ShortestDistanceCandidate;
			IntersectionEdge = FaceEdge;
		}
	}

	if( Velocity * V3(0,1,0) != 0)
	{
		// Bot
		aabb_feature_edge FaceEdge = {};		
		FaceEdge.P0 = Face.P0;
		FaceEdge.P1 = Face.P1;
		closest_vertex_pair ContactPointCandidate = VertexEdge( PrincipalContactPoint, FaceEdge );
		r32 ShortestDistanceCandidate  = Norm( ContactPointCandidate.P1 - ContactPointCandidate.P0);
		if( ShortestDistanceCandidate < ShortestDistance)
		{
			ShortestDistance = ShortestDistanceCandidate;
			IntersectionEdge = FaceEdge;
		}

		// Top
		FaceEdge.P0 = Face.P2;
		FaceEdge.P1 = Face.P3;
		ContactPointCandidate = VertexEdge( PrincipalContactPoint, FaceEdge );
		ShortestDistanceCandidate  = Norm( ContactPointCandidate.P1 - ContactPointCandidate.P0);
		if( ShortestDistanceCandidate < ShortestDistance)
		{
			ShortestDistance = ShortestDistanceCandidate;
			IntersectionEdge = FaceEdge;
		}
	}
	

	v3 Edge = (IntersectionEdge.P1 - IntersectionEdge.P0);
	v3 CollisionNormal = CrossProduct( Edge, V3(0,0,1));

	Assert( CollisionNormal != V3(0,0,0) );

	return CollisionNormal;

}

aabb_feature_vertex GetPrincipalContactPoint(aabb_feature_face& FaceA, aabb_feature_face& FaceB)
{

	b32 ContactPointFound = false;
	aabb_feature_vertex PrincipalContactPoint = {};
	for( u32 i = 0; i < ArrayCount( FaceA.P ); ++i )
	{
		closest_vertex_pair ContactPoints = VertexFace( FaceA.P[i], FaceB );
		if(Norm(ContactPoints.P1 - ContactPoints.P0) == 0)
		{
			PrincipalContactPoint = FaceA.P[i];
			ContactPointFound = true;
			break;
		}
	}

	Assert(ContactPointFound);
	return PrincipalContactPoint;
}


v3 GetPointOfIntersection(v3& PrincipalContactPoint, v3& DeltaR, aabb3f& A, aabb3f& B)
{
	v3 ACenter = A.P0 + (A.P1-A.P0)/2;
	v3 BCenter = B.P0 + (B.P1-B.P0)/2;
	
	v3 SidesB = (B.P1-B.P0)/2;

	u32 MaxIndex = GetMaxVelocityIndex(DeltaR);
	
	r32 AplhaPos = ( PrincipalContactPoint.E[MaxIndex] + SidesB.E[MaxIndex] - BCenter.E[MaxIndex] )/DeltaR.E[MaxIndex];
	r32 AplhaNeg = ( PrincipalContactPoint.E[MaxIndex] - SidesB.E[MaxIndex] - BCenter.E[MaxIndex] )/DeltaR.E[MaxIndex];

	r32 Alpha = 0;
	if( AplhaNeg < 0 )
	{
		Alpha = AplhaPos;
	}else{
		Alpha = AplhaNeg;
	}

	Assert(Alpha != 0);

	v3 P = PrincipalContactPoint - Alpha * DeltaR;

	return P;
}

#endif

inline aabb3f 
MergeAABB( const aabb3f& A, const aabb3f& B, v3 Envelope = {})
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


list< aabb3f > GetOverlappingWallTiles(memory_arena* Arena, tile_map* TileMap, aabb3f* BoundingBox, v3 CollisionEnvelope = {} )
{
	aabb3f EnvelopedBoundingBox = *BoundingBox;
	EnvelopedBoundingBox.P0 -= CollisionEnvelope;
	EnvelopedBoundingBox.P1 += CollisionEnvelope;

	list< tile_map_position > TilesToTest = list< tile_map_position >( Arena );
	GetIntersectingTiles(TileMap, &TilesToTest, &EnvelopedBoundingBox);

	list< aabb3f > IntersectingWallTiles = list< aabb3f >( Arena );
	for(TilesToTest.First(); 
	   !TilesToTest.IsEnd();
	    TilesToTest.Next() )
	{
		tile_map_position TilePosition = TilesToTest.Get();
		tile_contents Content = GetTileContents(TileMap, TilePosition);
		if( Content.Type == TILE_TYPE_WALL )
		{
			aabb3f WallTile = GetTileAABB( TileMap, TilePosition );
			IntersectingWallTiles.InsertAfter(WallTile);
		}
	}

	return IntersectingWallTiles;
}


b32 FindEarliestCollision( aabb3f& A, list<aabb3f>& BList, v3& dR, v3& OutFirstCollision, v3& OutCollisionNormal )
{
	r32 FirstHitPercentage = 1;
	b32 Intersection = false;

	r32 OvneOverdRNorm = 1/Norm(dR);

	for(BList.First();
	   !BList.IsEnd();
		BList.Next() )
	{
		v3 dROut = {};
		v3 HitNormal = {};
		
		aabb3f B = BList.Get();
		aabb_contact Contact = AABBContact(A, B);
		if( Contact.Type != AABB_CONTACT_TYPE_SEPARATE )
		{
			continue;
		}else{
			if( SweeptAABB( A, B, dR, dROut, HitNormal ) )
			{
				r32 HitPercentage = Norm( dROut ) * OvneOverdRNorm;
				if( (HitPercentage < FirstHitPercentage) && (HitPercentage >= 0) )
				{
					FirstHitPercentage = HitPercentage;
					OutFirstCollision = dROut;
					OutCollisionNormal = HitNormal;
					Intersection = true;
				}
			}			
		}

	}

	return Intersection;
}

struct timestep_progression 
{
	v3 P0;
	v3 P1;
	v3 Velocity;
	r32 DistanceToTravel;
	r32 DistanceTraveled;
};

timestep_progression ForwardEuler( r32 dt, r32 Mass, v3 Position, v3 Velocity, v3 ExternalForce, v3 (*ForceEquation)(v3&,v3&,v3&) )
{
	timestep_progression Result = {};
	
	v3 f  = {};
	if(ForceEquation)
	{
		f = ForceEquation(Position, Velocity, ExternalForce);
	}

	v3 Acceleration = f * Mass;
	Result.Velocity = Velocity + dt * Acceleration;

	Result.P0 = Position;
	Result.P1 = Position + dt * Result.Velocity;

	Result.DistanceToTravel = Norm(Result.P1 - Result.P0);
	Result.DistanceTraveled = 0;
	return Result;
}

void Reflect(v3 PointOfImpact, v3 CollisionNormal,  timestep_progression* TP )
{
	Assert(TP->DistanceToTravel > 0);
	Assert(TP->DistanceToTravel > TP->DistanceTraveled);
	Assert(Norm(CollisionNormal) == 1);

	r32  DistanceToCollision = Norm( PointOfImpact - TP->P0 );
	r32  RemainingDistance   = Norm( TP->P1 - PointOfImpact );

	v3 Perpendicular = ( TP->Velocity * CollisionNormal) * CollisionNormal;
	v3 Parallel  = TP->Velocity - Perpendicular;
 	v3 ReflectedVelocity = Parallel - Perpendicular;

	TP->DistanceTraveled += DistanceToCollision;
	TP->P0 = PointOfImpact;
	TP->P1 = PointOfImpact + RemainingDistance * Normalize( ReflectedVelocity );
	TP->Velocity = ReflectedVelocity;

	Assert( TP->DistanceTraveled <= TP->DistanceToTravel );
}

void Slide(v3 PointOfImpact, v3 CollisionNormal, timestep_progression* TP )
{
	Assert(TP->DistanceToTravel > 0);
	Assert(TP->DistanceToTravel > TP->DistanceTraveled);

	r32  DistanceToCollision = Norm( PointOfImpact - TP->P0 );
	r32  RemainingDistance   = Norm( TP->P1 - PointOfImpact );

	v3 Perpendicular = ( TP->Velocity * CollisionNormal) * CollisionNormal;
	v3 Parallel  = TP->Velocity - Perpendicular;

	TP->DistanceTraveled = TP->DistanceToTravel;
	r32 ParallelNorm = Norm( Parallel );
	if( ParallelNorm > 0 )
	{
	//	TP->P1 = PointOfImpact + ( RemainingDistance *  (1 / ParallelNorm );
	}else{
		TP->P1 = PointOfImpact;
	}
	
	TP->Velocity = Parallel;

	Assert( TP->DistanceTraveled <= TP->DistanceToTravel );
}

v3 HeroForceEquation( v3& Position, v3& Velocity, v3& ExternalForce )
{
	r32 WindDampingFactor = 5.f;
	r32 VelDot = Velocity * Velocity;
	v3 WindResistance = {};
	if(VelDot != 0)
	{
		WindResistance = WindDampingFactor * VelDot * Normalize(Velocity);
	}

	r32 FrictionDampingFactor = 10.f;
	v3 Friction = FrictionDampingFactor * Velocity;

	v3 Force = ExternalForce - WindResistance - Friction;

	return Force;
}

void DoReflectionCollision(memory_arena* Arena, tile_map* TileMap, entity* E, r32 dt)
{
	temporary_memory TempMem = BeginTemporaryMemory( Arena );

	component_spatial* S = E->SpatialComponent;

	Assert(S->Depth == 0);

	r32 Mass = 1;
	timestep_progression TP = ForwardEuler(dt, Mass, S->Position, S->Velocity, S->ExternalForce, HeroForceEquation);
	
	v3 Dim = V3(S->Width, S->Height, S->Depth);
	while( TP.DistanceTraveled < TP.DistanceToTravel )
	{
		aabb3f A0 = AABB3f( TP.P0-Dim/2, TP.P0+Dim/2 );
		aabb3f A1 = AABB3f( TP.P1-Dim/2, TP.P1+Dim/2 );
		aabb3f IntersectionBoundingBox = MergeAABB(A0, A1);
		
		v3 CollisionEnvelope = V3(1E-7,1E-7,0);
		list< aabb3f > PossibleCollisionAABBs = GetOverlappingWallTiles(Arena, TileMap, &IntersectionBoundingBox, CollisionEnvelope);
		v3 DistanceToCollision = {};
		v3 CollisionNormal = {};
		v3 dR = TP.P1-TP.P0;
		if( FindEarliestCollision( A0, PossibleCollisionAABBs, dR, DistanceToCollision, CollisionNormal ) )
		{
			v3 PointOfImpact = TP.P0 + DistanceToCollision;
			//if( Norm(TP.Velocity ) > 5 )
			//{
				Reflect( PointOfImpact, CollisionNormal, &TP );	
			//}else{
			//	Slide( PointOfImpact, CollisionNormal, &TP );
			//}
		}else{
			TP.DistanceTraveled += Norm(TP.P1-TP.P0);	
		}
	}

	r32 Speed = Norm(TP.Velocity);
	if(Speed < 0.05)
	{
		Speed = 0;
		TP.Velocity = {};
	}

	S->Position = TP.P1;
	S->Velocity = TP.Velocity;
	S->ExternalForce = {};

	EndTemporaryMemory(TempMem);
}

#if 0
void DoSlidingCollision(memory_arena* Arena, tile_map* TileMap, entity* E, r32 dt)
{
	temporary_memory TempMem = BeginTemporaryMemory( Arena );

	component_spatial* S = E->SpatialComponent;

	Assert(S->Depth == 0);

	r32 Mass = 1;
	timestep_progression TP = ForwardEuler(dt, Mass, S->Position, S->Velocity, S->ExternalForce, HeroForceEquation);
	v3 ADim = V3(S->Width, S->Height, S->Depth);

	v3 CollisionEnvelope = V3(1E-7,1E-7,0);
	aabb3f A0 = AABB3f( TP.P0-ADim/2, TP.P0+ADim/2 );
	aabb3f A1 = AABB3f( TP.P1-ADim/2, TP.P1+ADim/2 );
	aabb3f IntersectionBoundingBox = MergeAABB(A0, A1);

	list< aabb3f > PossibleCollisionAABBs =  GetOverlappingWallTiles(Arena, TileMap, &IntersectionBoundingBox, CollisionEnvelope);	
	v3 DistanceToCollision = {};
	v3 CollisionNormal = {};
	v3 dR = TP.P1-TP.P0;


	r32 FirstHitPercentage = 1;
	b32 Intersection = false;

	r32 OvneOverdRNorm = 1/Norm(dR);

	//v3 DeepestPenetration = V3(0,0,0);
	r32 Deeps = 0;
	r32 DeepestPenetration = 0;
	aabb3f OverlapBlock = {};
	u32 Axis = 0;

	// Find Deepest Penetration
	for(PossibleCollisionAABBs.First();
	   !PossibleCollisionAABBs.IsEnd();
		PossibleCollisionAABBs.Next() )
	{
		v3 ACenter = GetAABBCenter(A1);
		v3 ASide   = GetHalfSideLength(A1);

		aabb3f B = PossibleCollisionAABBs.Get();
		v3 BCenter = GetAABBCenter(B);
		v3 BSide   = GetHalfSideLength(B);

		v3 ContactSeparation = ASide + BSide;

		v3 Spearation = (BCenter - ACenter); // From A to B;
		Spearation.X = Abs(Spearation.X);
		Spearation.Y = Abs(Spearation.Y);
		Spearation.Z = Abs(Spearation.Z);

		v3 PenetrationDepth = ContactSeparation - Separation;

		r32 DeepestPenetrationForBlock = 0;
		if(( PenetrationDepth.X >= 0 ) &&
		   ( PenetrationDepth.Y >= 0 ) &&
		   ( PenetrationDepth.Z >= 0 ) )
		{
			u32 OverlapAxis = 0;
			if(DeepestPenetrationForBlock < PenetrationDepth.X)
			{
				DeepestPenetrationForBlock = PenetrationDepth.X;
				OverlapAxis = 1;
			}
			if(DeepestPenetrationForBlock < PenetrationDepth.Y)
			{
				DeepestPenetrationForBlock = PenetrationDepth.Y;
				OverlapAxis = 2;
			}
			if(DeepestPenetrationForBlock < PenetrationDepth.Z)
			{
				DeepestPenetrationForBlock = PenetrationDepth.Z;
				OverlapAxis = 3;
			}
		}

		if( DeepestPenetration < DeepestPenetrationForBlock )
		{
			DeepestPenetration = DeepestPenetrationForBlock;
			OverlapBlock = B;
			Axis = OverlapAxis;
		}

	}


	for(PossibleCollisionAABBs.First();
	   !PossibleCollisionAABBs.IsEnd();
		PossibleCollisionAABBs.Next() )
	{
		v3 dROut = {};
		v3 HitNormal = {};
		
		v3 ACenter = (A1.P1 + A1.P0) * 0.5;
		v3 ASide   = (A1.P1 - A1.P0) * 0.5;

		aabb3f B = PossibleCollisionAABBs.Get();
		v3 BCenter = (B.P1 + B.P0) * 0.5;
		v3 BSide   = (B.P1 - B.P0) * 0.5;

		v3 ABSeparation = BCenter - ACenter;
		v3 ABSeparationAbs = V3( Abs(ABSeparation.X), Abs(ABSeparation.Y), Abs(ABSeparation.Z));
		v3 ABMaxSeparation = BSide + ASide;

		v3 SeparationDiff = ABMaxSeparation - ABSeparationAbs;


		if( (Abs( ABSeparation.X ) < ABMaxSeparation.X ) && (Abs( ABSeparation.Y ) < ABMaxSeparation.Y ))
		{
			if( Abs( ABSeparation.X ) < ABMaxSeparation.X )
			{
				// A is to the left of B
				if( ABSeparation.X > 0 )
				{
					TP.P1.X -= SeparationDiff.X;
				// A is to the right of B
				}else{
					TP.P1.X += SeparationDiff.X;	
				}

				TP.Velocity.X = 0;
			}

			if( Abs( ABSeparation.Y ) < ABMaxSeparation.Y )
			{
				// A is below B
				if( ABSeparation.Y > 0 )
				{
					TP.P1.Y -= SeparationDiff.Y;
				// A is above B
				}else{
					TP.P1.Y += SeparationDiff.Y;
				}

				TP.Velocity.Y = 0;
			}
		}
/*
		if( Abs( ABSeparation.Z ) < ABMaxSeparation.Z )
		{
			if( ABSeparation.Z > 0 )
			{`
				CollisionNormal = V3( 0,0, -1);
			}else{
				CollisionNormal = V3( 0,0, 1);
			}
		}
*/

//		if( SweeptAABB( A, B, dR, dROut, HitNormal ) )
//		{
//			r32 HitPercentage = Norm( dROut ) * OvneOverdRNorm;
//			Assert( (HitPercentage <= FirstHitPercentage) && (HitPercentage >= 0) );
//			if( (HitPercentage <= FirstHitPercentage) && (HitPercentage >= 0) )
//			{
//				FirstHitPercentage = HitPercentage;
//				OutFirstCollision = dROut;
//				OutCollisionNormal = HitNormal;
//				Intersection = true;
//			}
//		}
	}

	   
	r32 Speed = Norm(TP.Velocity);
	if(Speed < 0.05)
	{
		Speed = 0;
		TP.Velocity = {};
	}

	S->Position = TP.P1;
	S->Velocity = TP.Velocity;
	S->ExternalForce = {};

	EndTemporaryMemory(TempMem);
}

#else

void DoSlidingCollision(memory_arena* Arena, tile_map* TileMap, entity* E, r32 dt)
{
	temporary_memory TempMem = BeginTemporaryMemory( Arena );

	component_spatial* S = E->SpatialComponent;

	Assert(S->Depth == 0);

	v3 Dim = V3(S->Width, S->Height, S->Depth);


	v3 CollisionEnvelope = V3(1E-7,1E-7,0);
	aabb3f InitialPosition = AABB3f( S->Position-Dim/2, S->Position+Dim/2 );
	list< aabb3f > Verifications = GetOverlappingWallTiles(Arena, TileMap, &InitialPosition, CollisionEnvelope);
	for(Verifications.First();
	   !Verifications.IsEnd();
		Verifications.Next() )
	{
		aabb3f WallTile = Verifications.Get();
		aabb_contact Contact = AABBContact( InitialPosition, WallTile, CollisionEnvelope );
		if( Contact.Type == AABB_CONTACT_TYPE_PENETRATION )
		{
			Assert(false);
		}
	}

	r32 Mass = 1;
	timestep_progression TP = ForwardEuler(dt, Mass, S->Position, S->Velocity, S->ExternalForce, HeroForceEquation);

	while( TP.DistanceTraveled < TP.DistanceToTravel )
	{
		aabb3f A0 = AABB3f( TP.P0-Dim/2, TP.P0+Dim/2 );
		aabb3f A1 = AABB3f( TP.P1-Dim/2, TP.P1+Dim/2 );
		
		list< aabb3f > PossibleCollisionAABBs = GetOverlappingWallTiles(Arena, TileMap, &A1, CollisionEnvelope);
		v3 DistanceToCollision = {};
		v3 CollisionNormal = {};
		v3 dR = TP.P1-TP.P0;
		if( FindEarliestCollision( A0, PossibleCollisionAABBs, dR, DistanceToCollision, CollisionNormal ) )
		{
			v3 PointOfImpact = TP.P0 + DistanceToCollision;
			Slide( PointOfImpact, CollisionNormal, &TP );

		}else{
			TP.DistanceTraveled += Norm(TP.P1-TP.P0);	
		}
	}

	r32 Speed = Norm(TP.Velocity);
	if(Speed < 0.05)
	{
		Speed = 0;
		TP.Velocity = {};
	}

	S->Position = TP.P1;
	S->Velocity = TP.Velocity;
	S->ExternalForce = {};

	EndTemporaryMemory(TempMem);
}


#endif

void SpatialSystemUpdate( world* World )
{
	v3 Gravity = V3( 0, -9, 0 );
	r32 dt =  1/60.f;

	memory_arena* Arena = &World->Arena;
	tile_map* TileMap = &World->TileMap;
	
	for(u32 Index = 0;  Index < World->NrEntities; ++Index )
	{
		entity* E = &World->Entities[Index];

		if( E->Types & COMPONENT_TYPE_SPATIAL )
		{
			//DoReflectionCollision(Arena, TileMap,  E, dt);
			//DoSlidingCollision(Arena, TileMap,  E, dt);
		}
	}

	CheckArena(Arena);	
}
