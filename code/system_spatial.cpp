#include "entity_components.h"
#include "handmade_tile.h"
#include "utility_macros.h"
#include "math/aabb.h"
#include "dynamic_aabb_tree.h"
#include "gjk_narrow_phase.h"
#include "epa_collision_data.h"

#define NEW_CONTACT_THRESHOLD 0.01f
#define PERSISTENT_CONTACT_THRESHOLD 0.01f
#define WARM_STARTING_FRACTION 1.0f

#define FRICTIONAL_COEFFICIENT 0.2f
#define BAUMGARTE_COEFFICIENT  0.3f
#define RESTITUTION_COEFFICIENT 0.f
#define SLOP 0.01f

#define SLOVER_ITERATIONS 24


internal inline void ScaleV12( r32 Scal, v3 const * V, v3* Result )
{
  Result[0] = V[0]*Scal;
  Result[1] = V[1]*Scal;
  Result[2] = V[2]*Scal;
  Result[3] = V[3]*Scal;
}

internal inline void MultiplyDiagonalM12V12( m3 const * M, v3 const * V, v3* Result )
{
  Result[0] = M[0] * V[0];
  Result[1] = M[1] * V[1];
  Result[2] = M[2] * V[2];
  Result[3] = M[3] * V[3];
}

internal inline r32 DotProductV12xV12( v3* A, v3* B)
{
  r32 Result = (A[0] * B[0]) + (A[1] * B[1]) + (A[2] * B[2]) + (A[3] * B[3]);
  return Result;
}

internal inline r32
getBaumgarteCoefficient(r32 dt, r32 Scalar, r32 PenetrationDepth, r32 Slop)
{
  r32 k = BranchlessArithmatic((PenetrationDepth - Slop) > 0, (PenetrationDepth  - Slop) , 0);
  r32 Baumgarte = - (Scalar / dt) * k;
  return Baumgarte;
}

internal inline r32
getRestitutionCoefficient(v3 V[], r32 Scalar, v3 Normal, r32 Slop)
{
  r32 ClosingSpeed = ((V[0] + V[1] + V[2] + V[3]) * Normal);
  ClosingSpeed = BranchlessArithmatic((ClosingSpeed - Slop) > 0, (ClosingSpeed - Slop) , 0);
  r32 Restitution = Scalar * ClosingSpeed;
  return Restitution;
}

internal r32 GetLambda(  v3 V[], v3 J[], v3 InvMJ[], r32 BaumgarteCoefficient, r32 RestitutionCoefficient)
{
  r32 Bias = BaumgarteCoefficient + RestitutionCoefficient;
  r32 Numerator   = -(DotProductV12xV12( J, V) + Bias);
  r32 Denominator =   DotProductV12xV12( J, InvMJ);
  
  r32 Result = Numerator / Denominator;
  return Result;
}

v3 ClosestPointOnEdge(const v3& EdgeStart, const v3& EdgeEnd, const v3& Point)
{
  const v3 EdgeDirection = Normalize(EdgeEnd-EdgeStart);
  const r32 EdgeLength = Norm(EdgeEnd-EdgeStart);
  const r32 ProjectionScalar = (Point-EdgeStart)*EdgeDirection;
  
  v3 ClosestPointOnEdge = {};
  if(ProjectionScalar <= 0)
  {
    ClosestPointOnEdge = EdgeStart;
  }else if(ProjectionScalar >= EdgeLength )
  {
    ClosestPointOnEdge = EdgeEnd;
  }else{
    ClosestPointOnEdge = EdgeStart + ProjectionScalar * EdgeDirection;
  }
  
  return ClosestPointOnEdge;
}

void CreatePositionalJacobianConstraint(v3 const & ra, v3 const & rb,
                                        v3 const & Normal,
                                        m3 const * M_Inv,
                                        v3* J, v3* InvMJ)
{
  J[0] = -Normal;
  J[1] = -CrossProduct(ra, Normal);
  J[2] = Normal;
  J[3] = CrossProduct(rb, Normal);
  MultiplyDiagonalM12V12(M_Inv, J, InvMJ);
}

// Normal points from ContactPoint A to ContactPoint B
// TangentOne Cross TangentTwo = Normal
// Normals and tangents are ofcourse in World Space [WS]
inline contact_data_cache
CreateDataCashe(v3 ContactPoint_A_WS, v3 ObjectCenter_A_WS,
                v3 ContactPoint_B_WS, v3 ObjectCenter_B_WS,
                v3 Normal, v3 TangentOne, v3 TangentTwo,
                r32 MassA, r32 MassB, m3 I_Inv_A, m3 I_Inv_B)
{
  contact_data_cache CachedData = {};
  const v3& ra = ContactPoint_A_WS - ObjectCenter_A_WS;
  const v3& rb = ContactPoint_B_WS - ObjectCenter_B_WS;
  
  r32 ma = MassA;
  r32 mb = MassB;
  
  m3 M_Inv[4] = {};
  M_Inv[0] =  M3(1/ma,0,0,
                 0,1/ma,0,
                 0,0,1/ma);
  M_Inv[1] = I_Inv_A;
  M_Inv[2] =  M3(1/mb,0,0,
                 0,1/mb,0,
                 0,0,1/mb);
  M_Inv[3] = I_Inv_B;
  
  CreatePositionalJacobianConstraint(ra, rb, Normal, M_Inv, CachedData.J, CachedData.InvMJ);
  CreatePositionalJacobianConstraint(ra, rb, TangentOne, M_Inv, CachedData.Jn1, CachedData.InvMJn1);
  CreatePositionalJacobianConstraint(ra, rb, TangentTwo, M_Inv, CachedData.Jn2, CachedData.InvMJn2);
  
  return CachedData;
}

inline contact_data_cache
CreateDataCashe( contact_data Contact, component_spatial* SpatialA, component_spatial* SpatialB,
                component_dynamics* DynamicsA, component_dynamics* DynamicsB, component_collider* ColliderA, component_collider* ColliderB)

{
  m3 Empty{};
  contact_data_cache CachedData = CreateDataCashe(Contact.A_ContactWorldSpace, SpatialA->Position,
                                                  Contact.B_ContactWorldSpace, SpatialB->Position,
                                                  Contact.ContactNormal,Contact.TangentNormalOne,
                                                  Contact.TangentNormalTwo,
                                                  DynamicsA ? DynamicsA->Mass : R32Max,
                                                  DynamicsB ? DynamicsB->Mass : R32Max,
                                                  DynamicsA ? DynamicsA->I_inv : Empty,
                                                  DynamicsB ? DynamicsB->I_inv : Empty);
  return CachedData;
}

contact_data* IsExistingContact(contact_manifold* Manifold, contact_data* NewContact)
{
  contact_data* Result = 0;
  vector_list<contact_data>& OldContactList = Manifold->Contacts;
  contact_data* OldContact = OldContactList.First();
  while (OldContact)
  {
    const v3 rALocal  = NewContact->A_ContactModelSpace - OldContact->A_ContactModelSpace;
    const v3 rBLocal  = NewContact->B_ContactModelSpace - OldContact->B_ContactModelSpace;
    const v3 rAGlobal = NewContact->A_ContactWorldSpace - OldContact->A_ContactWorldSpace;
    const v3 rBGlobal = NewContact->B_ContactWorldSpace - OldContact->B_ContactWorldSpace;
    r32 nsrla = Norm(rALocal);
    r32 nsrlb = Norm(rBLocal);
    r32 nsrga = Norm(rAGlobal);
    r32 nsrgb = Norm(rBGlobal);
    
    const b32 MatchesLocalA  = Norm(rALocal)  < NEW_CONTACT_THRESHOLD;
    const b32 MatchesLocalB  = Norm(rBLocal)  < NEW_CONTACT_THRESHOLD;
    const b32 MatchesGlobalA = Norm(rAGlobal) < NEW_CONTACT_THRESHOLD;
    const b32 MatchesGlobalB = Norm(rBGlobal) < NEW_CONTACT_THRESHOLD;
    
    if (MatchesLocalA && MatchesLocalB)
    {
      Result = OldContact;
      break;
    }
    OldContact = OldContactList.Next(OldContact);
  }
  
  return Result;
}

void PutDeepestContactFirst(vector_list<contact_data>* Contacts, m4 const & ModelMatrixA, m4 const & ModelMatrixB)
{
  contact_data* First = Contacts->First();
  
  contact_data* Contact = First;
  r32 Depth = 0;
  contact_data* Deepest = 0;
  while (Contact)
  {
    const v3 PossiblePointA = V3( ModelMatrixA * V4(Contact->A_ContactModelSpace,1));
    const v3 PossiblePointB = V3( ModelMatrixB * V4(Contact->B_ContactModelSpace,1));
    const r32 PenetrationDepth = (PossiblePointB - PossiblePointA)*Contact->ContactNormal;
    if(Depth >= PenetrationDepth)
    {
      Deepest = Contact;
      Depth = PenetrationDepth;
    }
    Contact = Contacts->Next(Contact);
  }
  Assert(Deepest);
  if (Deepest != First)
  {
    Contacts->Swap(Deepest, First);
  }
}

void FormLongestLine(vector_list<contact_data>* Contacts, m4 const & ModelMatrixA)
{
  contact_data* First = Contacts->First();
  const v3 FirstPoint = V3( ModelMatrixA * V4(First->A_ContactModelSpace,1));
  
  contact_data* Second = Contacts->Next(First);
  
  contact_data* Contact = Second;
  r32 Length = 0;
  contact_data* Furthest = 0;
  while (Contact)
  {
    const v3 PossiblePointA = V3( ModelMatrixA * V4(Contact->A_ContactModelSpace,1));
    
    r32 TestLength = Norm(PossiblePointA - FirstPoint);
    if(TestLength >= Length)
    {
      Furthest = Contact;
      Length = TestLength;
    }
    Contact = Contacts->Next(Contact);
  }
  if (Furthest != Second)
  {
    Contacts->Swap(Furthest, Second);
  }
}

void FormWidestTriangle(vector_list<contact_data>* Contacts, m4 const & ModelMatrixA)
{
  
  contact_data* First = Contacts->First();
  const v3 FirstPoint = V3( ModelMatrixA * V4(First->A_ContactModelSpace,1));
  
  contact_data* Second = Contacts->Next(First);
  const v3 SecondPoint = V3( ModelMatrixA * V4(Second->A_ContactModelSpace,1));
  
  contact_data* Third = Contacts->Next(Second);
  
  contact_data* Contact = Third;
  
  r32 Length = 0;
  contact_data* Furthest = 0;
  while (Contact)
  {
    const v3 PossiblePoint = V3( ModelMatrixA * V4(Contact->A_ContactModelSpace,1));
    const v3 ClosestPoint = ClosestPointOnEdge(FirstPoint, SecondPoint, PossiblePoint);
    
    const r32 TestLength = Norm(PossiblePoint - ClosestPoint);
    if(TestLength >= Length)
    {
      Furthest = Contact;
      Length = TestLength;
    }
    Contact = Contacts->Next(Contact);
  }
  Assert(Furthest);
  if (Furthest != Third)
  {
    Contacts->Swap(Furthest, Third);
  }
}


void FurthestFromTriangle(vector_list<contact_data>* Contacts, m4 const & ModelMatrixA)
{
  contact_data* Result = 0;
  
  contact_data* First = Contacts->First();
  const v3 FirstPoint = V3( ModelMatrixA * V4(First->A_ContactModelSpace,1));
  
  contact_data* Second = Contacts->Next(First);
  const v3 SecondPoint = V3( ModelMatrixA * V4(Second->A_ContactModelSpace,1));
  
  contact_data* Third = Contacts->Next(Second);
  const v3 ThirdPoint = V3( ModelMatrixA * V4(Third->A_ContactModelSpace,1));
  
  contact_data* Fourth = Contacts->Next(Third);
  
  contact_data* Contact = Fourth;
  
  r32 Length = 0;
  contact_data* Furthest = 0;
  while (Contact)
  {
    const v3 PossiblePoint = V3( ModelMatrixA * V4(Contact->A_ContactModelSpace,1));;
    const v3 Normal = GetPlaneNormal(FirstPoint,SecondPoint,ThirdPoint);
    const v3 ProjectedPoint = ProjectPointOntoPlane(PossiblePoint, FirstPoint, Normal);
    const v3 Coords = GetBaryocentricCoordinates(FirstPoint, SecondPoint, ThirdPoint, Normal, ProjectedPoint);
    const b32 InsideTriangle = (Coords.E[0] >= 0) && (Coords.E[0] <= 1) &&
      (Coords.E[1] >= 0) && (Coords.E[1] <= 1) &&
      (Coords.E[2] >= 0) && (Coords.E[2] <= 1);
    if(!InsideTriangle)
    {
      r32 CumuLength = Norm(PossiblePoint - FirstPoint) + Norm(PossiblePoint - SecondPoint) + Norm(PossiblePoint - ThirdPoint);
      if(CumuLength >= Length)
      {
        Length = CumuLength;
        Furthest = Contact;
      }
    }
    Contact = Contacts->Next(Contact);
  }
  
  if(Furthest)
  {
    if(Furthest != Fourth)
    {
      Contacts->Swap(Furthest, Fourth);  
    }
    Contacts->PopBack();
  }else{
    Contacts->PopBack();
    Contacts->PopBack();
  }
}


internal PLATFORM_WORK_QUEUE_CALLBACK(DoCollisionDetectionWork)
{
  TIMED_FUNCTION();
  entity_manager* EM = GlobalGameState->EntityManager;
  contact_manifold* Manifold = (contact_manifold*) Data;
  
  component_spatial* SpatialA   = (component_spatial*)  GetComponent(EM, Manifold->EntityIDA, COMPONENT_FLAG_SPATIAL);
  component_spatial* SpatialB   = (component_spatial*)  GetComponent(EM, Manifold->EntityIDB, COMPONENT_FLAG_SPATIAL);
  component_collider* ColliderA = (component_collider*) GetComponent(EM, Manifold->EntityIDA, COMPONENT_FLAG_COLLIDER);
  component_collider* ColliderB = (component_collider*) GetComponent(EM, Manifold->EntityIDB, COMPONENT_FLAG_COLLIDER);
  component_dynamics* DynamicsA = (component_dynamics*) GetComponent(EM, Manifold->EntityIDA, COMPONENT_FLAG_DYNAMICS);
  component_dynamics* DynamicsB = (component_dynamics*) GetComponent(EM, Manifold->EntityIDB, COMPONENT_FLAG_DYNAMICS);
  m4 ModelMatrixA = SpatialA->ModelMatrix;
  m4 ModelMatrixB = SpatialB->ModelMatrix;
  collider_mesh MeshA = GetColliderMesh(GlobalGameState->AssetManager, ColliderA->Object);
  collider_mesh MeshB = GetColliderMesh(GlobalGameState->AssetManager, ColliderB->Object);
  
  gjk_collision_result NarrowPhaseResult = GJKCollisionDetection(
                                                                 &ModelMatrixA, &MeshA,
                                                                 &ModelMatrixB, &MeshB);
  
  // Todo: Should we give each thread it's own Transient Arena?
  // Note: GlobalGamestate->TransientArena is NOT thread safe, don't use it in any PLATFORM_WORK_QUEUE_CALLBACK.
  memory_arena Arena = {};
  temporary_memory TempMem = BeginTemporaryMemory(&Arena);
  
  if (NarrowPhaseResult.ContainsOrigin)
  {
    contact_data NewContact = EPACollisionResolution(&Arena,
                                                     &ModelMatrixA, &MeshA,
                                                     &ModelMatrixB, &MeshB,
                                                     &NarrowPhaseResult.Simplex);
    
    u32 MatchingContactIndex = 0;
    contact_data* MatchingContact = IsExistingContact(Manifold, &NewContact);
    if (MatchingContact)
    {
      // If the found point is an existing contact, we update the old contact
      *MatchingContact = NewContact;
      MatchingContact->Cache = CreateDataCashe(NewContact, SpatialA, SpatialB, DynamicsA, DynamicsB, ColliderA, ColliderB);
    }else{
      NewContact.Cache = CreateDataCashe(NewContact, SpatialA, SpatialB, DynamicsA, DynamicsB, ColliderA, ColliderB);
      Manifold->Contacts.PushBack(NewContact);
      if (Manifold->Contacts.Size() == Manifold->MaxContactCount+1)
      {
        PutDeepestContactFirst(&Manifold->Contacts, ModelMatrixA, ModelMatrixB);
        FormLongestLine(&Manifold->Contacts, ModelMatrixA);
        FormWidestTriangle(&Manifold->Contacts, ModelMatrixA);
        FurthestFromTriangle(&Manifold->Contacts, ModelMatrixA);
        Assert(Manifold->Contacts.Size() < (Manifold->MaxContactCount+1));
      }
    }
  }
  
  EndTemporaryMemory(TempMem);
}

internal void RemoveInvalidContactPoints( contact_manifold* FirstManifold )
{
  TIMED_FUNCTION();
  contact_manifold* Manifold = FirstManifold;
  entity_manager* EM = GlobalGameState->EntityManager;
  while (Manifold)
  {
    component_spatial* SpatialA = (component_spatial*) GetComponent(EM, Manifold->EntityIDA, COMPONENT_FLAG_SPATIAL);
    component_spatial* SpatialB = (component_spatial*) GetComponent(EM, Manifold->EntityIDB, COMPONENT_FLAG_SPATIAL);
    m4 ModelMatrixA = SpatialA->ModelMatrix;
    m4 ModelMatrixB = SpatialB->ModelMatrix;
    
    vector_list<contact_data>* Contacts = &Manifold->Contacts;
    contact_data* Contact = Contacts->First();
    while (Contact)
    {
      const v3 LocalToGlobalA = V3( ModelMatrixA * V4(Contact->A_ContactModelSpace,1));
      const v3 LocalToGlobalB = V3( ModelMatrixB * V4(Contact->B_ContactModelSpace,1));
      
      const v3 rAB = LocalToGlobalB - LocalToGlobalA;
      
      const v3 rA  = Contact->A_ContactWorldSpace - LocalToGlobalA;
      const v3 rB  = Contact->B_ContactWorldSpace - LocalToGlobalB;
      
      const r32 normDot = Contact->ContactNormal * rAB;
      const b32 stillPenetrating = normDot <= 0.0f;
      
      const r32 lenDiffA = NormSq(rA);
      const r32 lenDiffB = NormSq(rB);
      
      // keep contact point if the collision pair is still colliding at this point
      // and the local positions are not too far from the global
      // positions original acquired from collision detection
      if (stillPenetrating && ((lenDiffA < PERSISTENT_CONTACT_THRESHOLD) && (lenDiffB < PERSISTENT_CONTACT_THRESHOLD)) )
      {
        Contact->Persistent = true;
        Contact = Contacts->Next(Contact);
      }else{
        contact_data* ContactToRemove = Contact;       
        Contact = Contacts->Next(Contact);
        ContactToRemove->Persistent = false;
        Contacts->Pop(ContactToRemove);
      }
    }
    Manifold = Manifold->Next;
  }
}

internal void DoWarmStarting( contact_manifold* FirstManifold  )
{
  TIMED_FUNCTION();
  contact_manifold* Manifold = FirstManifold;
  while (Manifold)
  {
    vector_list<contact_data>* Contacts = &Manifold->Contacts;
    contact_data* Contact = Contacts->First();
    
    // Warm starting
    while(Contact)
    {
      r32 WarmStartingFraction = WARM_STARTING_FRACTION;
      if(Contact->Persistent)
      {
        v3 DeltaV[4] = {};
        v3 DeltaV1[4] = {};
        v3 DeltaV2[4] = {};
        
        contact_data_cache* Cache = &Contact->Cache;
        ScaleV12(WarmStartingFraction * Cache->AccumulatedLambda,   Cache->InvMJ,   DeltaV);
        ScaleV12(WarmStartingFraction * Cache->AccumulatedLambdaN1, Cache->InvMJn1, DeltaV);
        ScaleV12(WarmStartingFraction * Cache->AccumulatedLambdaN2, Cache->InvMJn2, DeltaV);
        Cache->AccumulatedLambda = 0;
        Cache->AccumulatedLambdaN1 = 0;
        Cache->AccumulatedLambdaN2 = 0;
        
        component_dynamics* DynamicsA = (component_dynamics*) GetComponent(GlobalGameState->EntityManager, Manifold->EntityIDA, COMPONENT_FLAG_DYNAMICS);
        component_dynamics* DynamicsB = (component_dynamics*) GetComponent(GlobalGameState->EntityManager, Manifold->EntityIDB, COMPONENT_FLAG_DYNAMICS);
        if(DynamicsA)
        {
          DynamicsA->LinearVelocity  += (DeltaV[0] + DeltaV1[0] + DeltaV2[0]);
          DynamicsA->AngularVelocity += (DeltaV[1] + DeltaV1[1] + DeltaV2[1]);
        }
        if(DynamicsB)
        {
          DynamicsB->LinearVelocity  += (DeltaV[2] + DeltaV1[2] + DeltaV2[2]);
          DynamicsB->AngularVelocity += (DeltaV[3] + DeltaV1[3] + DeltaV2[3]);
        }
      }
      Contact = Contacts->Next(Contact);
    }
    Manifold = Manifold->Next;
  }
}

inline void IntegrateVelocities( r32 dt )
{
  TIMED_FUNCTION();
  
  ScopedTransaction(GlobalGameState->EntityManager);
  
  component_result* ComponentList = GetComponentsOfType(GlobalGameState->EntityManager, COMPONENT_FLAG_DYNAMICS);
  while(Next(GlobalGameState->EntityManager, ComponentList))
  {
    component_spatial*   S = (component_spatial*) GetComponent(GlobalGameState->EntityManager,  ComponentList, COMPONENT_FLAG_SPATIAL);
    component_collider*  C = (component_collider*) GetComponent(GlobalGameState->EntityManager, ComponentList, COMPONENT_FLAG_COLLIDER);
    component_dynamics*  D = (component_dynamics*) GetComponent(GlobalGameState->EntityManager, ComponentList, COMPONENT_FLAG_DYNAMICS);
    
    // Forward euler
    // TODO: Investigate other more stable integration methods
    v3 Gravity = V3(0,-9.82,0);
    v3 ExternalForce = Gravity*D->Mass;
    v3 LinearImpulse = dt * ExternalForce;
    v3 DeltaV_Linear = LinearImpulse/D->Mass;
    D->LinearVelocity += DeltaV_Linear;
    
    m3 RotMat = M3(QuaternionAsMatrix( S->Rotation ));
    m3 RotMat_Inv = Transpose(RotMat);
    
    v3 ExternalTorqueWorldCoord = V3(0,0,0);
    v3 AngularImpulseWorldCoord = dt * ExternalTorqueWorldCoord;
    
#if USE_ANGULAR_VEL_OBJECT_SPACE
    // If objects angular velocity vector is given in object-coordinates
    v3 DeltaV_Angular = D->I_inv * RotMat_Inv * AngularImpulseWorldCoord;
#else
    // If objects angular velocity vector is given in world-coordinates
    v3 DeltaV_Angular = RotMat * D->I_inv * RotMat_Inv * AngularImpulseWorldCoord;
#endif
    
    D->AngularVelocity += DeltaV_Angular;
  }
}

internal void
GenerateContactPoints( world_contact_chunk* ContactChunk, u32 BroadPhaseResultCount, broad_phase_result_stack* const BroadPhaseResultStack )
{
  TIMED_FUNCTION();
  broad_phase_result_stack* ColliderPair = BroadPhaseResultStack;
  contact_manifold** WorkArray = (contact_manifold**) PushArray(GlobalGameState->TransientArena, BroadPhaseResultCount, contact_manifold* );
  contact_manifold** WorkSlot = WorkArray;
  ContactChunk->FirstManifold = 0;
  while( ColliderPair )
  {
    contact_manifold* Manifold = FindManifoldSlot(ContactChunk, ColliderPair->EntityIDA, ColliderPair->EntityIDB);
    Assert(Manifold);
    
    // Push the contact to the list.
    Manifold->Next = ContactChunk->FirstManifold;
    ContactChunk->FirstManifold = Manifold;
    
    // Add it to the work array
    *WorkSlot = Manifold;
#if MULTI_THREADED
    Platform.PlatformAddEntry(Platform.HighPriorityQueue, DoCollisionDetectionWork, (void*) *WorkSlot);
#endif
    ColliderPair = ColliderPair->Previous;
    ++WorkSlot;
  }
  
#if MULTI_THREADED
  Platform.PlatformCompleteWorkQueue(Platform.HighPriorityQueue);
#else
  u32 CollisionWorkIndex = 0;
  while(CollisionWorkIndex < BroadPhaseResultCount)
  {
    DoCollisionDetectionWork(Platform.HighPriorityQueue, (void*) *(WorkArray + CollisionWorkIndex++));
  }
#endif
}

internal void RemoveNonIntersectingManifolds(world_contact_chunk* ContactChunk)
{
  TIMED_FUNCTION();
  contact_manifold** ManifoldPtr = &ContactChunk->FirstManifold;
  while(*ManifoldPtr)
  {
    contact_manifold* Manifold = *ManifoldPtr;
    if(Manifold->Contacts.IsEmpty())
    {
      // Remove manifolds that are not colliding
      *ManifoldPtr = Manifold->Next;
      Manifold->Next = 0;
    }else{
      //DoWarmStarting( World );
      Assert(Manifold != Manifold->Next);
      ManifoldPtr = &(Manifold->Next);
    }
  }
}

internal void
SolveNonPenetrationConstraints(r32 dtForFrame, contact_manifold* FirstManifold)
{
  TIMED_FUNCTION();
  contact_manifold* Manifold = FirstManifold;
  while(Manifold)
  {
    Assert(!Manifold->Contacts.IsEmpty());
    component_spatial* SpatialA = GetSpatialComponent(Manifold->EntityIDA);
    component_spatial* SpatialB = GetSpatialComponent(Manifold->EntityIDB);
    component_dynamics* DynamicsA = GetDynamicsComponent(Manifold->EntityIDA);
    component_dynamics* DynamicsB = GetDynamicsComponent(Manifold->EntityIDB);
    
    vector_list<contact_data>* Contacts = &Manifold->Contacts;
    contact_data* Contact = Contacts->First();
    while(Contact)
    {
      v3 V[4] = {};
      if(DynamicsA)
      {
        V[0] = DynamicsA->LinearVelocity;
        V[1] = DynamicsA->AngularVelocity;
      }
      if(DynamicsB)
      {
        V[2] = DynamicsB->LinearVelocity;
        V[3] = DynamicsB->AngularVelocity;
      }
      
      contact_data_cache* Cache = &Contact->Cache;
      
      v3 ContactNormal     = Contact->ContactNormal;
      v3 ContactPointDiff  = V3(SpatialA->ModelMatrix * V4(Contact->A_ContactModelSpace,1)) -
        V3(SpatialB->ModelMatrix * V4(Contact->B_ContactModelSpace,1));
      r32 PenetrationDepth = ContactPointDiff * ContactNormal;
      
      r32 Restitution       = getRestitutionCoefficient(V, RESTITUTION_COEFFICIENT, ContactNormal, SLOP);
      r32 Baumgarte         = getBaumgarteCoefficient(dtForFrame, BAUMGARTE_COEFFICIENT,  PenetrationDepth, SLOP);
      r32 Lambda            = GetLambda( V, Cache->J, Cache->InvMJ, Baumgarte, Restitution);
      r32 OldCumulativeLambda = Cache->AccumulatedLambda;
      Cache->AccumulatedLambda += Lambda;
      Cache->AccumulatedLambda = Maximum(0, Cache->AccumulatedLambda);
      r32 LambdaDiff = Cache->AccumulatedLambda - OldCumulativeLambda;
      
      v3 DeltaV[4] = {};
      ScaleV12(LambdaDiff, Cache->InvMJ, DeltaV);
      
      if(LambdaDiff > 0)
      {
        if(DynamicsA)
        {
          DynamicsA->LinearVelocity  += DeltaV[0];
          DynamicsA->AngularVelocity += DeltaV[1];
        }
        if(DynamicsB)
        {
          DynamicsB->LinearVelocity  += DeltaV[2];
          DynamicsB->AngularVelocity += DeltaV[3];
        }
      }
      
      Contact = Contacts->Next(Contact);
    }
    Manifold = Manifold->Next;
  }
}

internal void
SolveFrictionalConstraints( contact_manifold* FirstManifold )
{
  TIMED_FUNCTION();
  contact_manifold* Manifold = FirstManifold;
  while(Manifold)
  {
    Assert(!Manifold->Contacts.IsEmpty());
    component_dynamics* DynamicsA = GetDynamicsComponent(Manifold->EntityIDA);
    component_dynamics* DynamicsB = GetDynamicsComponent(Manifold->EntityIDB);
    vector_list<contact_data>* Contacts = &Manifold->Contacts;
    contact_data* Contact = Contacts->First();
    while(Contact)
    {
      v3 V[4] = {};
      if(DynamicsA)
      {
        V[0] = DynamicsA->LinearVelocity;
        V[1] = DynamicsA->AngularVelocity;
      }
      if(DynamicsB)
      {
        V[2] = DynamicsB->LinearVelocity;
        V[3] = DynamicsB->AngularVelocity;
      }
      
      contact_data_cache* Cache = &Contact->Cache;
      
      r32 Kf = FRICTIONAL_COEFFICIENT;
      r32 ClampRange = Kf * Cache->AccumulatedLambda;
      
      r32 LambdaN1 = GetLambda( V, Cache->Jn1, Cache->InvMJn1, 0, 0);
      r32 LambdaN2 = GetLambda( V, Cache->Jn2, Cache->InvMJn2, 0, 0);
      
      r32 OldCumulativeLambdaN1 = Cache->AccumulatedLambdaN1;
      r32 OldCumulativeLambdaN2 = Cache->AccumulatedLambdaN2;
      Cache->AccumulatedLambdaN1 = Clamp(Cache->AccumulatedLambdaN1 + LambdaN1, -ClampRange, ClampRange);
      Cache->AccumulatedLambdaN2 = Clamp(Cache->AccumulatedLambdaN2 + LambdaN2, -ClampRange, ClampRange);
      
      r32 LambdaDiffN1 = Cache->AccumulatedLambdaN1 - OldCumulativeLambdaN1;
      r32 LambdaDiffN2 = Cache->AccumulatedLambdaN2 - OldCumulativeLambdaN2;
      
      v3 DeltaV1[4] = {};
      v3 DeltaV2[4] = {};
      
      ScaleV12(LambdaDiffN1, Cache->InvMJn1, DeltaV1);
      ScaleV12(LambdaDiffN2, Cache->InvMJn2, DeltaV2);
      
      if (!(Equals(Abs(LambdaDiffN1),0) && Equals(Abs(LambdaDiffN2),0)))
      {
        if(DynamicsA)
        {
          DynamicsA->LinearVelocity  += (DeltaV1[0] + DeltaV2[0]);
          DynamicsA->AngularVelocity += (DeltaV1[1] + DeltaV2[1]);
        }
        if(DynamicsB)
        {
          DynamicsB->LinearVelocity  += (DeltaV1[2] + DeltaV2[2]);
          DynamicsB->AngularVelocity += (DeltaV1[3] + DeltaV2[3]);
        }
      }
      
      Contact = Contacts->Next(Contact);
    }
    Manifold = Manifold->Next;
  }
}

//inline interal void
//AngularVelocityFunction(v4& q0, v4 dq);
//{
//  const v4 q0 = Rotation;
//  const v4 r = V4(AngularVelocity.X, AngularVelocity.Y, AngularVelocity.Z,0);
//  const v4 q1 = 0.5f*QuaternionMultiplication(r, q0);
//}



inline void
TimestepVelocityRungeKutta4(const r32 DeltaTime, const v3 LinearVelocity, const v3 AngularVelocity, component_spatial* c )
{
  Assert(c);
#if 1
  // Note, This is wrong, why is it working?
  {
    auto dfdt_Lin = []( const v3& LinearVelocity )
    {
      return LinearVelocity;
    };
    v3 k1 = dfdt_Lin(LinearVelocity);
    v3 k2 = dfdt_Lin(LinearVelocity + 0.5f*DeltaTime*k1);
    v3 k3 = dfdt_Lin(LinearVelocity + 0.5f*DeltaTime*k2);
    v3 k4 = dfdt_Lin(LinearVelocity + DeltaTime*k4);
    c->Position += (1/6.f) * DeltaTime * (k1 + 2*k2 + 2*k3 + k4);
  }
#else
  {
    auto dfdt_Lin = [LinearVelocity]( v3 p )
    {
      return LinearVelocity;
    };
    v3 k1 = dfdt_Lin(c->Position);
    v3 k2 = dfdt_Lin(c->Position + 0.5f*DeltaTime*k1);
    v3 k3 = dfdt_Lin(c->Position + 0.5f*DeltaTime*k2);
    v3 k4 = dfdt_Lin(c->Position + DeltaTime*k4);
    c->Position += (1/6.f) * DeltaTime * (k1 + 2*k2 + 2*k3 + k4);
  }
#endif
  {
    auto dfdt_Rot = [&AngularVelocity]( const v4& Rotation )
    {
      const v4 q0 = Rotation;
      const v4 r = V4(AngularVelocity.X, AngularVelocity.Y, AngularVelocity.Z, 0);
      const v4 q1 = 0.5f*QuaternionMultiplication(r, q0);
      return q1;
    };
    v4 k1 = dfdt_Rot(c->Rotation);
    v4 k2 = dfdt_Rot(c->Rotation + 0.5f*DeltaTime*k1);
    v4 k3 = dfdt_Rot(c->Rotation + 0.5f*DeltaTime*k2);
    v4 k4 = dfdt_Rot(c->Rotation + DeltaTime*k4);
    c->Rotation += (1/6.f) * DeltaTime * (k1 + 2*k2 + 2*k3 + k4);
  }
}

inline void
TimestepVelocityForwardEuler(const r32 DeltaTime, const v3 LinearVelocity, const v3 AngularVelocity, component_spatial* c )
{
  c->Position += DeltaTime*LinearVelocity;
  
  const v4 q0 = c->Rotation;
  r32 Angle = DeltaTime * Norm(AngularVelocity);
  v3 Axis = Normalize(AngularVelocity);
  
#if USE_ANGULAR_VEL_OBJECT_SPACE
  v4 DeltaQ = RotateQuaternion( Angle , Axis );
#else
  v4 DeltaQ = QuaternionMultiplication( c->Rotation, QuaternionMultiplication( RotateQuaternion( Angle , Axis ) , QuaternionInverse(c->Rotation)));
#endif
  
  c->Rotation = QuaternionMultiplication(DeltaQ, c->Rotation);
}

inline internal void
IntegratePositions(r32 dtForFrame)
{
  TIMED_FUNCTION();
  
  ScopedTransaction(GlobalGameState->EntityManager);
  component_result* ComponentList = GetComponentsOfType(GlobalGameState->EntityManager, COMPONENT_FLAG_DYNAMICS);
  while(Next(GlobalGameState->EntityManager,ComponentList))
  {
    component_spatial* S = GetSpatialComponent(ComponentList);
    component_dynamics* D = GetDynamicsComponent(ComponentList);
#if 1
    TimestepVelocityForwardEuler( dtForFrame, D->LinearVelocity, D->AngularVelocity, S );
    S->Rotation = Normalize(S->Rotation);
#else
    TimestepVelocityRungeKutta4( dtForFrame, D->LinearVelocity, D->AngularVelocity, S );
    S->Rotation = Normalize(S->Rotation);
#endif
    UpdateModelMatrix(S);
  }
}

void CastRay(game_input* GameInput, world* World )
{
  b32 MouseClicked = GameInput->MouseButton[PlatformMouseButton_Left].EndedDown;
  v2 MousePos = V2(GameInput->MouseX,GameInput->MouseY);
  if(MouseClicked)
  {
    game_window_size WindowSize = GameGetWindowSize();
    entity_manager* EM = GlobalGameState->EntityManager;
    ScopedTransaction(EM);
    component_result* ComponentList = GetComponentsOfType(EM, COMPONENT_FLAG_CAMERA);
    Assert(Next(EM, ComponentList));
    component_camera* Camera = (component_camera*) GetComponent(EM, ComponentList, COMPONENT_FLAG_CAMERA);
    
    ray Ray = GetRayFromCamera(Camera, MousePos);
    World->CastedRay = RayCast(GlobalGameState->TransientArena, &World->BroadPhaseTree, Ray.Origin, Ray.Direction);
    
    if(World->CastedRay.Hit)
    {
      World->PickedEntity.Active = true;
      World->PickedEntity.EntityID = World->CastedRay.EntityID;
      World->PickedEntity.Point = World->CastedRay.Intersection;
      World->PickedEntity.PointObjectSpace = World->CastedRay.IntersectionObjectSpace;
    }else{
      World->PickedEntity = {};
    }
  }  
  
  if(World->PickedEntity.Active)
  {
    component_spatial* Spatial = GetSpatialComponent(World->PickedEntity.EntityID);
    component_dynamics* Dynamics = GetDynamicsComponent(World->PickedEntity.EntityID);
    if(Dynamics)
    {
      // TODO(Jakob Hariz): Set up a positional constraitnt between mouse Point and Point Object Space
      v3 Up, Right, Forward;
      component_camera* Camera = GetActiveCamera();
      Assert(Camera);
      GetCameraDirections(Camera, &Up, &Right, &Forward);
      ray Ray = GetRayFromCamera(Camera, V2(GlobalGameState->Input->MouseX, GlobalGameState->Input->MouseY));
      r32 t = RaycastPlane(Ray.Origin, Ray.Direction, World->PickedEntity.Point, -Forward);
      World->PickedEntity.MousePointOnPlane = Ray.Origin + t*Ray.Direction;
      
      // WS = WorldSpace
      // OS = ObjectSpace
      v3 MousePoint_WS = World->PickedEntity.MousePointOnPlane;
      v3 ObjectPoint_OS = World->PickedEntity.PointObjectSpace;
      v3 ObjectPoint_WS = V3(Spatial->ModelMatrix * V4(ObjectPoint_OS));
      v3 ra = ObjectPoint_OS - Spatial->Position;
      v3 R = (MousePoint_WS - ObjectPoint_WS);
      v3 Normal = -Normalize(R);
      r32 Length = Norm(R);
      r32 ScaleFactor = 0.1f;
      v3 Velocity = ScaleFactor * Normal;;
      
      
      m3 M_Inv[4]{};
      
      r32 ma = Dynamics->Mass;
      
      M_Inv[0] =  M3(1/ma,0,0,
                     0,1/ma,0,
                     0,0,1/ma);
      M_Inv[1] = Dynamics->I_inv;
      M_Inv[2] = {};
      M_Inv[3] = {};
      
      v3 Jacobian[4]{};
      v3 InvMJ[4]{};
      
      CreatePositionalJacobianConstraint(V3(0,0,0), ra, Normal, M_Inv, Jacobian, InvMJ);
      
      r32 AccumulatedLambda = 0;
      for (u32 i = 0; i < SLOVER_ITERATIONS; ++i)
      {
        v3 V[4] = {};
        if(Dynamics)
        {
          V[0] = Dynamics->LinearVelocity;
          V[1] = Dynamics->AngularVelocity;
        }
        
        v3 ContactPointDiff  = R;
        r32 PenetrationDepth = 0;
        
        //r32 Restitution = getRestitutionCoefficient(V, RESTITUTION_COEFFICIENT, Normal, SLOP);
        r32 Baumgarte = getBaumgarteCoefficient(World->dtForFrame, BAUMGARTE_COEFFICIENT, Length*ScaleFactor, SLOP);
        r32 Lambda = GetLambda( V, Jacobian, InvMJ, Baumgarte, 0);
        r32 OldCumulativeLambda = AccumulatedLambda;
        AccumulatedLambda += Lambda;
        r32 LambdaDiff = AccumulatedLambda - OldCumulativeLambda;
        
        v3 DeltaV[4] = {};
        ScaleV12(LambdaDiff, InvMJ, DeltaV);
        
        if(Abs(LambdaDiff) > 0)
        {
          Dynamics->LinearVelocity  += DeltaV[0];
          Dynamics->AngularVelocity += DeltaV[1];
        }
      }
    }
  }
}

inline v3 ToLocal( component_spatial* Spatial, v3 GlobalPosition )
{
  v3 Result = V3(AffineInverse(Spatial->ModelMatrix)*V4(GlobalPosition,1));
  return Result;
}

inline v3 ToLocal( component_spatial* Spatial)
{
  v3 Result = ToLocal(Spatial, Spatial->Position);
  return Result;
}

inline v3 ToGlobal( component_spatial* Spatial, v3 LocalPosition )
{
  v3 Result = V3(Spatial->ModelMatrix * V4(LocalPosition,1));
  return Result;
}

inline v3 ToGlobal( component_spatial* Spatial )
{
  v3 Result = ToGlobal(Spatial, Spatial->Position);
  return Result;
}

struct distance_constraint
{
  v3 RA;
  v3 RB;
  v3 U;
  
  r32 Mass;
  r32 Impulse;
};

distance_constraint InitiateDistanceConstraint(component_spatial * SpatialA,
                                               component_spatial * SpatialB,
                                               component_dynamics* DynamicsA,
                                               component_dynamics* DynamicsB)
{
  distance_constraint Result = {};
  
  world* world = GlobalGameState->World;
  
  b32 Hard = true;
  r32 HardLength = 0.5;
  r32 SoftMinLength = 0.25;
  r32 SoftMaxLength = 0.75;
  
  // NOTE(Jakob): Global Constraint Settings
  r32 LinearSlop = 0.01;
  
  // NOTE(Jakob): Constraint Definition
  v3 LocalAnchorA = V3(1,1,1);
  v3 LocalAnchorB = V3(0,0,0);
  
  // Rigid Range
  r32 Length = Maximum( HardLength, LinearSlop);
  // Soft Range
  r32 MinLenght = Maximum(Hard ? HardLength : SoftMinLength, LinearSlop); 
  r32 MaxLength = Maximum(Hard ? HardLength : SoftMaxLength, LinearSlop);
  r32 Stiffness = 0.1;
  r32 Damping = 0.1;
  
  r32 Gamma = 0;
  r32 Bias = 0;
  r32 Impulse = 0;
  r32 LowerImpulse = 0;
  r32 UpperImpulse = 0;
  r32 CurrentLength = 0;
  
  r32 SoftMass = 0;
  
  // End of Constraint Definition
  
  v3 LocalCenterA = ToLocal(SpatialA);
  r32 InvMassA = 1.0f/DynamicsA->Mass;
  m3 InvIA = DynamicsA->I_inv;
  v3 CenterA = SpatialA->Position;
  v4 RotA = SpatialA->Rotation; // NOTE(Jakob): Quaternion
  //v3 LinearVelocityA = Velocities[0];
  //v3 AngularVelocityA = Velocities[1];
  
  
  v3 LocalCenterB = ToLocal(SpatialB);
  r32 InvMassB = 1.0f/DynamicsB->Mass;
  m3 InvIB = DynamicsB->I_inv;
  v3 CenterB = SpatialB->Position;
  v4 RotB = SpatialB->Rotation;
  //v3 LinearVelocityB = Velocities[2];
  //v3 AngularVelocityB = Velocities[3];
  
  Result.RA = ToGlobal(SpatialA, LocalAnchorA - LocalCenterA);
  Result.RB = ToGlobal(SpatialB, LocalAnchorB - LocalCenterB);
  Result.U = CenterB + Result.RB - CenterA - Result.RA;
  
  // NOTE(Jakob): Handle Singularity
  CurrentLength = Norm(Result.U);
  if(CurrentLength > LinearSlop)
  {
    Result.U *= 1.0f/Length;
  }else{
    
    Result.U = {};
    Result.Mass = 0;
    Result.Impulse = 0;
    LowerImpulse = 0;
    UpperImpulse = 0;
  }
  
  v3 CrossAU = CrossProduct(Result.RA,Result.U);
  v3 CrossBU = CrossProduct(Result.RB,Result.U);
  float InvMass = InvMassA + InvIA * CrossAU * CrossAU + InvMassB + InvIB * CrossBU * CrossBU; 
  Result.Mass = InvMass != 0.0f ? 1.0f/InvMass : 0.0f;
  
  if(Stiffness > 0.0f && MinLenght < MaxLength)
    
  {
    // Implement Later, Try hard range first
    INVALID_CODE_PATH;
    // Soft
    r32 C = CurrentLength - Length;
    r32 d = Damping;
    r32 k = Stiffness;
    
    float h = GlobalGameState->World->dtForFrame;
    
  }else{
    Gamma = 0;
    Bias = 0;
    SoftMass = 0;
  }
  
  float DoWarmStarting = false;
  if(DoWarmStarting)
  {
    // Scale the impulse to support a variable time step.
    //Impulse *= data.step.dtRatio;
		//LowerImpulse *= data.step.dtRatio;
		//UpperImpulse *= data.step.dtRatio;
    
		//b2Vec2 P = (m_impulse + m_lowerImpulse - m_upperImpulse) * m_u;
		//vA -= m_invMassA * P;
		//wA -= m_invIA * b2Cross(m_rA, P);
		//vB += m_invMassB * P;
		//wB += m_invIB * b2Cross(m_rB, P);
  }
  else
  {
    Result.Impulse = 0;
  }
  
  return Result;
}

void SolveVelocityConstraints(component_dynamics* DynamicsA, component_dynamics* DynamicsB,  distance_constraint DistanceConstraint)
{
  
  r32 MinLenght = 1;
  r32 MaxLength = 0;
  if(MinLenght < MaxLength)
  {
    // Soft Constraint
  }
  else
  {
    v3 VelOfPointA =  DynamicsA->LinearVelocity + CrossProduct(DynamicsA->AngularVelocity, DistanceConstraint.RA);
    v3 VelOfPointB =  DynamicsB->LinearVelocity + CrossProduct(DynamicsB->AngularVelocity, DistanceConstraint.RB);
    r32 CDot = DistanceConstraint.U * (VelOfPointB - VelOfPointA);
    
    r32 Impulse = -DistanceConstraint.Mass * CDot;
    DistanceConstraint.Impulse += Impulse;
    
    v3 P = Impulse * DistanceConstraint.U;
    DynamicsA->LinearVelocity  -= (1.0f/DynamicsA->Mass) * P;
    DynamicsA->AngularVelocity -= DynamicsA->I_inv *CrossProduct(DistanceConstraint.RA, P);
    DynamicsB->LinearVelocity  += (1.0f/DynamicsB->Mass) * P; 
    DynamicsB->AngularVelocity += DynamicsB->I_inv *CrossProduct(DistanceConstraint.RB, P);
  }
}

void SolveDistanceVelocityConstraint()
{
  
  ScopedTransaction(GlobalGameState->EntityManager);
  component_result* ComponentList = GetComponentsOfType(GlobalGameState->EntityManager, COMPONENT_FLAG_DYNAMICS);
  
  component_spatial* SpatialA = 0;
  component_dynamics* DynamicsA = 0;
  
  while(Next(GlobalGameState->EntityManager , ComponentList))
  {
    SpatialA = (component_spatial*) GetComponent(GlobalGameState->EntityManager, ComponentList, COMPONENT_FLAG_SPATIAL);
    DynamicsA = (component_dynamics*) GetComponent(GlobalGameState->EntityManager, ComponentList, COMPONENT_FLAG_DYNAMICS);
  }
  Assert(SpatialA && DynamicsA);
  
  component_spatial SpatialB{};
  SpatialB.Scale = V3(1,1,1);
  SpatialB.ModelMatrix = M4Identity();
  SpatialB.Position = V3(0,0,0);
  SpatialB.Rotation = V4(0,0,0,1);
  
  component_dynamics DynamicsB = {};
  DynamicsB.LinearVelocity = V3(0,0,0);
  DynamicsB.AngularVelocity = V3(0,0,0);
  DynamicsB.ExternalForce = V3(0,0,0);
  DynamicsB.Mass = R32Max;
  DynamicsB.I= M3(R32Max, 0,0,
                  0,R32Max,0,
                  0,0,R32Max);
  DynamicsB.I_inv = {};
  
  distance_constraint D = InitiateDistanceConstraint
    (SpatialA, &SpatialB, DynamicsA, &DynamicsB);
  SolveVelocityConstraints(DynamicsA,&DynamicsB, D);
  //SolvePositionalConstraints(DynamicsA,&DynamicsB, D);
  
}

void SpatialSystemUpdate( world* World )
{
  TIMED_FUNCTION();
  
  
  //IntegrateVelocities(World->dtForFrame);
  //IntegratePositions(World->dtForFrame);
  
  
  world_contact_chunk* WorldContacts =  World->ContactManifolds;
  
  RemoveInvalidContactPoints( WorldContacts->FirstManifold );
  
  DoWarmStarting( WorldContacts->FirstManifold );
  
  World->BroadPhaseTree = BuildBroadPhaseTree( );
  
  u32 BroadPhaseResultCount = 0;
  broad_phase_result_stack* const BroadPhaseResultStack = GetCollisionPairs( &World->BroadPhaseTree, &BroadPhaseResultCount );
  
  GenerateContactPoints( WorldContacts, BroadPhaseResultCount, BroadPhaseResultStack );
  
  RemoveNonIntersectingManifolds(WorldContacts);
  
  BEGIN_BLOCK(SolveConstraints);
  if(WorldContacts->FirstManifold)
  {
    for (u32 i = 0; i < SLOVER_ITERATIONS; ++i)
    {
      // TODO: Process Constraints MultiThreaded
      SolveNonPenetrationConstraints(World->dtForFrame, WorldContacts->FirstManifold);
      SolveFrictionalConstraints(WorldContacts->FirstManifold);
    }
  }
  
  for (u32 i = 0; i < SLOVER_ITERATIONS; ++i)
  {
    // TODO: Process Constraints MultiThreaded
    SolveNonPenetrationConstraints(World->dtForFrame, WorldContacts->FirstManifold);
    SolveFrictionalConstraints(WorldContacts->FirstManifold);
  }
  
  CastRay(GlobalGameState->Input, World);
  
  //SolveDistanceVelocityConstraint();
  
  END_BLOCK(SolveConstraints);
  
  IntegrateVelocities(World->dtForFrame);
  IntegratePositions(World->dtForFrame);
  
}