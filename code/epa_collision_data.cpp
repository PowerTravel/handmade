#include "epa_collision_data.h"
#include "gjk_narrow_phase.h"
#include "memory.h"
#include "halfedge_mesh.h"

/*
 * A Tetrahedron is made up of 4 Points assembling into 4 Triangles.
 * The normals of the triangles should point outwards.
 * This function Fixes the winding of the tetrahedron such that
 * Dot( v03, Cross( v01, v02 )) < 0; (Tripple Product)
 * Or in words: Define the bottom triangle to be {p0,p1,p2} with Normal : (p1-p0) x (p2-p0).
 * The Tetrahedron is then considered wound CCW if Dot( p3-p0 , Normal ) < 0.
 * If Dot( p3-p0 , Normal ) == 0 Our p3 lies on the plane of the bottom triangle and
 * we need another point to  be p3.
 */

void FixWindingCCW(gjk_support Support[4])
{
  TIMED_FUNCTION();
  // Fix Winding so that all triangles go ccw
  v3 v01 = Support[1].S - Support[0].S;
  v3 v02 = Support[2].S - Support[0].S;
  v3 v03 = Support[3].S - Support[0].S;
  const r32 Determinant = v03 * CrossProduct(v01,v02);
  Assert(Abs(Determinant) > 10E-7);
  
  if(Determinant > 0.f)
  {
  	// Swap first and third Support
    gjk_support Tmp = Support[0];
    Support[0] = Support[3];
    Support[3] = Tmp;
  }
}

epa_face* GetClosestFaceToOrigin(epa_mesh* Mesh, r32* ShortestDistance)
{
  *ShortestDistance = R32Max;
  epa_face* Result = 0;
  epa_face* Face = Mesh->Faces;
  while(Face)
  {
    r32 Distance = GetDistanceToFace(Face);
    if(Distance < *ShortestDistance)
    {
      Result = Face;
      *ShortestDistance = Distance;
    }
    Face = Face->Next;
  }
  return Result;
}

internal epa_mesh *
CreateSimplexMesh(memory_arena* Arena, gjk_simplex* Simplex )
{
  gjk_support Tetrahedron[4] = {Simplex->SP[0],Simplex->SP[1],Simplex->SP[2],Simplex->SP[3]};
  FixWindingCCW(Tetrahedron);
  epa_mesh* Result = InitializeMesh(Arena, &Tetrahedron[0], &Tetrahedron[1], &Tetrahedron[2]);
  FillHole(Result, &Tetrahedron[3]);
  
  return Result;
}

void PopulateContactData(const m4* AModelMat, const m4* BModelMat, epa_face* Face,   r32 DistanceToClosestFace, contact_data* ContactData)
{
  gjk_support A = Face->Edge->TargetVertex->P;
  gjk_support B = Face->Edge->Next->TargetVertex->P;
  gjk_support C = Face->Edge->Next->Next->TargetVertex->P;
  v3 FaceNormal = GetNormal(Face);
  v3 P = FaceNormal * DistanceToClosestFace;
  
  v3 Coords = GetBaryocentricCoordinates(A.S,B.S,C.S,FaceNormal,P);
  
  //Assert(((Coords.E[0] >= 0) && (Coords.E[0] <= 1) &&
  //        (Coords.E[1] >= 0) && (Coords.E[1] <= 1) &&
  //        (Coords.E[2] >= 0) && (Coords.E[2] <= 1)));
  
  v3 InterpolatedSupportA = Coords.E[0] * A.A + Coords.E[1] * B.A + Coords.E[2] * C.A;
  v3 InterpolatedSupportB = Coords.E[0] * A.B + Coords.E[1] * B.B + Coords.E[2] * C.B;
  
  v3 Tangent1 = {};
  v3 Tangent2 = {};
  getOrthronormalVectorPair(FaceNormal, &Tangent1, &Tangent2);
  
  ContactData->A_ContactWorldSpace = V3(*AModelMat * V4(InterpolatedSupportA,1));
  ContactData->B_ContactWorldSpace = V3(*BModelMat * V4(InterpolatedSupportB,1));
  ContactData->A_ContactModelSpace = InterpolatedSupportA;
  ContactData->B_ContactModelSpace = InterpolatedSupportB;
  ContactData->ContactNormal       = FaceNormal;
  ContactData->TangentNormalOne    = Tangent1;
  ContactData->TangentNormalTwo    = Tangent2;
  ContactData->PenetrationDepth    = DistanceToClosestFace;
  
  Assert(Norm(FaceNormal)>0.5f);
}

contact_data EPACollisionResolution(memory_arena* Arena,
                                    const m4* AModelMat, const collider_mesh* AMesh,
                                    const m4* BModelMat, const collider_mesh* BMesh,
                                    gjk_simplex* Simplex)
{
  TIMED_FUNCTION();
  
  BlowUpSimplex(AModelMat, AMesh,
                BModelMat, BMesh,
                Simplex);
  
  epa_mesh* Mesh = CreateSimplexMesh(Arena, Simplex);
  
  epa_face* ClosestFace = 0;
  const v3 Origin = {};
  r32 PreviousDistanceToClosestFace = R32Max;
  r32 DistanceToClosestFace = R32Max;
  
  u32 Tries = 0;
  const u32 TriesUntilGivingUp = 12;
  
  for(;;)
  {
    // Todo: Does the line from the origin to the face have to be parallel with the face normal?
    ClosestFace = GetClosestFaceToOrigin( Mesh, &DistanceToClosestFace);
    Tries = (Abs(DistanceToClosestFace - PreviousDistanceToClosestFace) > 10E-7) ? 0 : Tries+1;
    if(Tries > TriesUntilGivingUp)
    {
      break;
    }
    
    gjk_support SupportPoint = CsoSupportFunction( AModelMat, AMesh,
                                                  BModelMat, BMesh,
                                                  GetNormal(ClosestFace));
    
    
    if(!GrowMesh(Mesh, &SupportPoint))
    {
      break;
    }
    
    PreviousDistanceToClosestFace = DistanceToClosestFace;
    
  };
  Assert(ClosestFace);
  
  contact_data ContactData = {};
  PopulateContactData(AModelMat, BModelMat, ClosestFace, DistanceToClosestFace, &ContactData);
  
  return ContactData;
  
}

void ResetManifoldSlot(contact_manifold* ManifoldSlot, u32 ManifoldIndex, u32 EntityIDA, u32 EntityIDB)
{
  Assert(ManifoldSlot);
  Assert(!ManifoldSlot->EntityIDA);
  Assert(!ManifoldSlot->EntityIDB);
  Assert(ManifoldSlot->Contacts.Valid());
  
  ManifoldSlot->EntityIDA = EntityIDA;
  ManifoldSlot->EntityIDB = EntityIDB;
  ManifoldSlot->MaxContactCount = 4;
  ManifoldSlot->WorldArrayIndex = ManifoldIndex;
}

contact_manifold* FindManifoldSlot(world_contact_chunk* Manifolds, u32 EntityIDA, u32 EntityIDB)
{
  u32 CantorPair = GetCantorPair(EntityIDA, EntityIDB);
  u32 ManifoldIndex = CantorPair % Manifolds->MaxCount;
  u32 HashMapCollisions = 0;
  contact_manifold* ManifoldSlot = 0;
  while( HashMapCollisions < Manifolds->MaxCount )
  {
    contact_manifold* ManifoldArraySlot = Manifolds->ManifoldVector + ManifoldIndex;
    if(!ManifoldArraySlot->EntityIDA)
    {
      ManifoldSlot = ManifoldArraySlot;
      ResetManifoldSlot(ManifoldSlot, ManifoldIndex, EntityIDA, EntityIDB);
      break;
    }
    else if(((EntityIDA == ManifoldArraySlot->EntityIDA) && (EntityIDB == ManifoldArraySlot->EntityIDB)) ||
            ((EntityIDA == ManifoldArraySlot->EntityIDB) && (EntityIDB == ManifoldArraySlot->EntityIDA)))
    {
      ManifoldSlot = ManifoldArraySlot;
      Assert(ManifoldSlot->MaxContactCount == 4);
      Assert(ManifoldSlot->WorldArrayIndex == ManifoldIndex);
      break;
    }else{
      TIMED_BLOCK(ContactArrayCollisions);
      ++HashMapCollisions;
      Assert( !((EntityIDA == ManifoldArraySlot->EntityIDA) && (EntityIDB == ManifoldArraySlot->EntityIDB)) &&
             !((EntityIDB == ManifoldArraySlot->EntityIDA) && (EntityIDA == ManifoldArraySlot->EntityIDB)) );
      ManifoldIndex = (ManifoldIndex+1) % Manifolds->MaxCount;
    }
  }
  Assert(HashMapCollisions < Manifolds->MaxCount);
  return ManifoldSlot;
}
