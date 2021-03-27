#define PERSISTENT_CONTACT_THRESHOLD 0.01f
#define NEW_CONTACT_THRESHOLD 0.01f

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
  
  gjk_collision_result NarrowPhaseResult = GJKCollisionDetection(&ModelMatrixA, &MeshA,
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
      
    }else{
      
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


void ColliderSystemUpdate( world* World )
{
  world_contact_chunk* WorldContacts =  World->ContactManifolds;
  
  RemoveInvalidContactPoints( WorldContacts->FirstManifold );
  
  World->BroadPhaseTree = BuildBroadPhaseTree( );
  
  u32 BroadPhaseResultCount = 0;
  broad_phase_result_stack* const BroadPhaseResultStack = GetCollisionPairs( &World->BroadPhaseTree, &BroadPhaseResultCount );
  
  GenerateContactPoints( WorldContacts, BroadPhaseResultCount, BroadPhaseResultStack );
  
  RemoveNonIntersectingManifolds(WorldContacts);
}
