#include "entity_components.h"
#include "utility_macros.h"
#include "math/aabb.h"
//#include "dynamic_aabb_tree.h"
//#include "gjk_narrow_phase.h"
//#include "epa_collision_data.h"

#define WARM_STARTING_FRACTION 0.21f

inline void IntegrateAllVelocities( r32 dt )
{
  TIMED_FUNCTION();

  BeginScopedEntityManagerMemory();

  component_result* ComponentList = GetComponentsOfType(GlobalGameState->EntityManager, COMPONENT_FLAG_DYNAMICS);
  while(Next(GlobalGameState->EntityManager, ComponentList))
  {
    component_spatial*   S = (component_spatial*) GetComponent(GlobalGameState->EntityManager,  ComponentList, COMPONENT_FLAG_SPATIAL);
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

    // Objects angular velocity vector is given in object-coordinates
    v3 DeltaV_Angular = D->I_inv * RotMat_Inv * AngularImpulseWorldCoord;

    D->AngularVelocity += DeltaV_Angular;

    // Here he also stores the position for continous collision


    // From https://github.com/erincatto/box2d/blob/master/src/dynamics/b2_island.cpp#L205
    // Not sure why he applies a damping to velocities (Air resistance? But that should be propto vel squared)
    // Apply damping.
    // ODE: dv/dt + c * v = 0
    // Solution: v(t) = v0 * exp(-c * t)
    // Time step: v(t + dt) = v0 * exp(-c * (t + dt)) = v0 * exp(-c * t) * exp(-c * dt) = v * exp(-c * dt)
    // v2 = exp(-c * dt) * v1
    // Pade approximation:
    // v2 = v1 * 1 / (1 + c * dt)
    // v *= 1.0f / (1.0f + h * b->m_linearDamping);
    // w *= 1.0f / (1.0f + h * b->m_angularDamping);
  }
}

inline internal void
IntegratePositions(r32 dtForFrame)
{
  TIMED_FUNCTION();

  BeginScopedEntityManagerMemory();
  component_result* ComponentList = GetComponentsOfType(GlobalGameState->EntityManager, COMPONENT_FLAG_DYNAMICS);
  while(Next(GlobalGameState->EntityManager,ComponentList))
  {
    component_spatial* S = GetSpatialComponent(ComponentList);
    component_dynamics* D = GetDynamicsComponent(ComponentList);

    S->Position += dtForFrame*D->LinearVelocity;
    const v4 q0 = S->Rotation;
    r32 Angle = dtForFrame * Norm(D->AngularVelocity);
    v3 Axis = Normalize(D->AngularVelocity);

    v4 DeltaQ = RotateQuaternion( Angle , Axis );
    S->Rotation = QuaternionMultiplication(DeltaQ, S->Rotation);
    S->Rotation = Normalize(S->Rotation);

    UpdateModelMatrix(S);
  }
}

struct dynamics_positions
{
  v3 Position;
  v4 Rotation;
};

struct dynamics_velocities
{
  v3 Linear;
  v3 Angular;
};

u32 GetAllPositionsAndVelocities( u32** Entities_Result, dynamics_positions** Positions_Result, dynamics_velocities** Velocities_Result)
{
  entity_manager* EM = GlobalGameState->EntityManager;
  memory_arena* Arena = GlobalGameState->TransientArena;

  BeginScopedEntityManagerMemory();
  component_result* ComponentList = GetComponentsOfType(GlobalGameState->EntityManager, COMPONENT_FLAG_COLLIDER);
  u32 EntityCount = ComponentList->EntityCount;
  u32* Entities = PushArray(Arena, EntityCount, u32);
  dynamics_positions* Positions = PushArray(Arena, EntityCount, dynamics_positions);
  dynamics_velocities* Velocities = PushArray(Arena, EntityCount, dynamics_velocities);

  u32 Index = 0;
  while(Next(GlobalGameState->EntityManager,ComponentList))
  {
    // NOTE(Jakob): Colliders are guaranteed to have a Spatial but not a Dynamics
    component_spatial* S = GetSpatialComponent(ComponentList);
    component_dynamics* D = GetDynamicsComponent(ComponentList);

    Entities[Index] = GetEntity(ToBptr(S));
    dynamics_positions* Position = Positions + Index;
    dynamics_velocities* Velocity = Velocities + Index;
    Position->Position = S->Position;
    Position->Rotation = S->Rotation;

    if(D)
    {
      Velocity->Linear = D->LinearVelocity;
      Velocity->Angular = D->AngularVelocity;
    }else{
      Velocity = {};
    }

    ++Index;
  }

  *Entities_Result = Entities;
  *Positions_Result = Positions;
  *Velocities_Result = Velocities;
  return EntityCount;
}

void StoreAllPositionsAndVelocities(u32 Count, u32* Entities, dynamics_positions* Positions, dynamics_velocities* Velocities)
{
  for (u32 i = 0; i < Count; i++)
  {
    component_spatial* S = GetSpatialComponent(Entities[i]);
    component_dynamics* D = GetDynamicsComponent(Entities[i]);

    Assert(S);
    S->Position = (Positions+i)->Position;
    S->Rotation = (Positions+i)->Rotation;

    if(D)
    {
      D->LinearVelocity = (Velocities+i)->Linear;
      D->AngularVelocity = (Velocities+i)->Angular;
    }

  }
}

u32 GetIndexOfEntity(u32 EntityCount, u32* Entities, u32 Entity)
{
  // TODO(Jakob): BruteFoce Search, slow, Make sure to not use this for 'final' version
  //              of the collision-matrix
  u32 Result = 0;
  for (u32 i = 0; i < EntityCount; i++)
  {
    if(Entities[i] == Entity)
    {
      Result = i;
      break;
    }
  }
  return Result;
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


internal inline void ScaleV12( r32 Scal, v3 const * V, v3* Result )
{
  Result[0] = V[0]*Scal;
  Result[1] = V[1]*Scal;
  Result[2] = V[2]*Scal;
  Result[3] = V[3]*Scal;
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
          DynamicsA->LinearVelocity  += (DeltaV[0] + DeltaV1[0] + DeltaV[0]);
          DynamicsA->AngularVelocity += (DeltaV[1] + DeltaV1[1] + DeltaV[1]);
        }
        if(DynamicsB)
        {
          DynamicsB->LinearVelocity  += (DeltaV[2] + DeltaV1[2] + DeltaV[2]);
          DynamicsB->AngularVelocity += (DeltaV[3] + DeltaV1[3] + DeltaV[3]);
        }
      }
      Contact = Contacts->Next(Contact);
    }
    Manifold = Manifold->Next;
  }
}


u32 GetContactCount(contact_manifold* ContactManifolds)
{
  contact_manifold* Manifold = ContactManifolds;
  u32 Result = 0;
  while (Manifold)
  {
    Result += Manifold->Contacts.Size();
    Manifold = Manifold->Next;
  }
  return Result;
}

u32 GetAllCollisionVelocityConstraints( contact_manifold* ContactManifolds, contact_data*** ResultCVC)
{
  u32 ContactCount = GetContactCount(ContactManifolds);
  contact_data** DstContacts = PushArray(GlobalGameState->TransientArena, ContactCount, contact_data*);

  contact_manifold* Manifold = ContactManifolds;
  u32 ContactIndex = 0;

  contact_data** DstContact = DstContacts;
  while (Manifold)
  {
    contact_data* SrcContact = Manifold->Contacts.First();
    while (SrcContact)
    {
      *(DstContact++) = SrcContact;
      SrcContact = Manifold->Contacts.Next(SrcContact);
    }

    Manifold = Manifold->Next;
  }

  *ResultCVC = DstContacts;

  return ContactCount;
}

// Normal points from ContactPoint A to ContactPoint B
// TangentOne Cross TangentTwo = Normal
// Normals and tangents are ofcourse in World Space [WS]

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

void InitiateContactVelocityConstraints(contact_manifold* Manifold)
{
  while(Manifold)
  {
    component_spatial* SpatialA = GetSpatialComponent(Manifold->EntityIDA);
    component_collider* ColliderA = GetColliderComponent(Manifold->EntityIDA);
    component_dynamics* DynamicsA = GetDynamicsComponent(Manifold->EntityIDA);

    component_spatial* SpatialB = GetSpatialComponent(Manifold->EntityIDB);
    component_collider* ColliderB = GetColliderComponent(Manifold->EntityIDB);
    component_dynamics* DynamicsB = GetDynamicsComponent(Manifold->EntityIDB);

    vector_list<contact_data> Contacts = Manifold->Contacts;
    contact_data* Contact = Contacts.First();
    while(Contact)
    {
      Contact->Cache = CreateDataCashe(*Contact, SpatialA, SpatialB, DynamicsA, DynamicsB, ColliderA, ColliderB);
      Contact = Contacts.Next(Contact);
    }
    Manifold = Manifold->Next;
  }
}


void SpatialSystemUpdate( world* World )
{
  // There are 5 types of constraints that are handles separately
  //   1 Contact-Velocity-Constraints [CVC], Collisions between rigid bodies
  //   2 Contact-Position-Constraints [CPC], Collisions between rigid bodies
  //   2 Joint-Velocity-Constraints [JVC],   Hinges, springs, prismatics etc
  //   3 Joint-Position-Constraints [JPC],   ------||------
  //   4 Time-of-Impact-Position-Constraints [TOI], Another type of collision where
  //     time of impact is important, not sure yet what this is.

  //------- Solver - Algo --------

  //  ------ OverView --------
  // For each body
  //    Integrate Velocities (Apply Gravity and Damping
  //  Set up CVC
  //  Set up JVC
  //  Do Warm Starting of CVC
  //  For # iterations:
  //     Solve JVC and CVC
  //  Store impulses of CVC
  //  Integrate Positions
  //

  //  ------ Detailed --------

  // Initiate Contact Solver
  //  Set up:
  //    - A huge Position vector Containng all Positions
  //    - A huge Velocity vector containig all Velocities
  //    - All the Contacts, Position and velocity constraints
  //           (A contact handles contacts between two bodies.
  //            Contacts are also connected with edges joining
  //            multiple contacting bodies) (Persistant,

  //    - Set up position independent Contact Variables
  //         Allocate space Apply all position-independent variables of the Contact Solver
  //         Friction, Restitution, Entity Indeces, Masses, Inertial Tensors, etc etc
  //         Initiate normal and tangent impulse if warmstarting

  //    - Set up position dependent Contact Variables
  //         rA, rB, normalMass,

  //    - If he has two contact points for a manifold (2-4 for me I would guess) he
  //      sets up a block-solver, not quite sure what that is yet.
  //      Seems like he calculates some sort of determinant from the normal, rA and rB,
  //      and checks if the condition number is less thant 1000, (that is he checks if the
  //      matrix is invertible) If not he has redundant points... Ahaa, if two points are too
  //      close, he reduces the number of points back to one. Could I do something similar when
  //      choosing contact points? Right now I just arbitrarily state that if two points are closer
  //      than some value they are the same point.

  //  - Do warm starting of CVC
  //      Integrate velocities using the previously stored normal impulse

  // CVC Warm-Starting
  // Initiate the Joint-Velocity-Constraints [JVC]
  // Solve CVC

  r32 h = World->dtForFrame;
  IntegrateAllVelocities(h);

  world_contact_chunk* ContactManifolds = World->ContactManifolds;

  InitiateContactVelocityConstraints(ContactManifolds->FirstManifold);

  DoWarmStarting(ContactManifolds->FirstManifold);


  u32 IterationCount = 24;
  for (u32 i = 0; i < IterationCount; i += 1)
  {
    SolveFrictionalConstraints(ContactManifolds->FirstManifold);
    SolveNonPenetrationConstraints(h, ContactManifolds->FirstManifold);
  }



  IntegratePositions(h);

#if 0
  u32* Entities = 0;
  dynamics_positions* Positions  = 0;
  dynamics_velocities* Velocities = 0;
  u32 EntityCount = GetAllPositionsAndVelocities(&Entities, &Positions, &Velocities);

  // NOTE(Jakob): An island is a graph of bodies that affect each other via some potential
  //              contact, joint or other constraint. It has a contact-manifold graph that
  //              can be traversed to figure out which bodies are interacting, their const-
  //              aints etc.

  // NOTE(Jakob): Given an island: Traverses the chart and selects out collisions and joints
  contact_data** CVCs = 0;
  //contact_data* JVCs = 0;
  u32 CVCCount = GetAllCollisionVelocityConstraints(World->ContactManifolds->FirstManifold, &CVCs);

  //u32 JVCCount = GetAllJointVelocityConstraints(World->ContactManifolds, EntityCount, Entities, &JVCs);


  u32 IterationCount = 24;
  for (u32 i = 0; i < IterationCount; i += 1)
  {
    //Solve(CVCs);
    //Solve(JVCs);
  }


#if 0


  // TODO(Jakob): CollisionMatrix is a very sparse symmetric matrix, should be improved once
  //              we implement island.
  //
  //              Islands should be set up maybe by the Collision Handler and the matrices
  //              with them.

  // NOTE(Jakob): A collision matrix is a EntityCount X EntityCount matrix where an interaction
  //              occures between body i and j if Element(i,j) is set
  //              The Collision Matrix is supposed to be a accelerated data structure
  //              already have that in the world_contact_chunk. So this is not really
  //              efficient at the moment.
  //              The goal is to replace world_contact_chunk with islands, Collision-graphs
  //              and CollisionMatrices

  u32* CollisionMatrix = PushArray(GlobalGameState->TransientArena, EntityCount*EntityCount,u32);

  world_contact_chunk* WorldContacts = World->ContactManifolds;
  contact_manifold* Manifold = WorldContacts->FirstManifold;
  while(Manifold)
  {
    // NOTE(Jakob): A populates Rows, B populats columns
    u32 EntityIndexA = GetIndexOfEntity(EntityCount, Entities, Manifold->EntityIDA);
    u32 EntityIndexB = GetIndexOfEntity(EntityCount, Entities, Manifold->EntityIDB);
    u32 CollisionMatrixInxdex = ToArrayIndex(EntityIndexA, EntityIndexB, EntityCount);
    CollisionMatrix[CollisionMatrixIndex] = 1;

    //GetCollisionVelocityConstraint();

    Manifold = Manifold->Next;
  }
#endif
  StoreAllPositionsAndVelocities(EntityCount, Entities, Positions, Velocities);
  IntegratePositions(h);
#endif



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

  BeginScopedEntityManagerMemory();
  component_result* Components = GetComponentsOfType(GlobalGameState->EntityManager, COMPONENT_FLAG_DYNAMICS);

  component_spatial* SpatialA = 0;
  component_dynamics* DynamicsA = 0;

  while(Next(GlobalGameState->EntityManager, Components))
  {
    SpatialA = (component_spatial*) GetComponent(GlobalGameState->EntityManager, Components, COMPONENT_FLAG_SPATIAL);
    DynamicsA = (component_dynamics*) GetComponent(GlobalGameState->EntityManager, Components, COMPONENT_FLAG_DYNAMICS);
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

void InitiateJointVelocityConstraints_old(joint_constraint* Joint)
{
  Joint->Impulse = 0;
  Joint->AxialMass = 0;

  Joint->LowerImpulse = 0;
  Joint->UpperImpulse = 0;

  Joint->RotationAngle = 0;

  // Distance Constraint
  Joint->EnableLimit = false;
  Joint->LowerAngle = 0;
  Joint->UpperAngle = 0;

  // Motor Constraint
  Joint->MotorImpulse = 0;
  Joint->EnableMotor = false;
  Joint->MaxMotorTorque = 0;
  Joint->MotorSpeed = 0;


  component_spatial  * SA = GetSpatialComponent(Joint->EntityA);
  component_spatial  * SB = GetSpatialComponent(Joint->EntityB);
  component_dynamics * DA = GetDynamicsComponent(Joint->EntityA);
  component_dynamics * DB = GetDynamicsComponent(Joint->EntityB);

  Joint->mA = R32Max;
  Joint->IA_inv = {};
  v3 VA = {};
  v3 WA = {};
  if(DA)
  {
    Joint->mA = DA->Mass;
    Joint->IA_inv = DA->I_inv;
    VA = DA->LinearVelocity;
    WA = DA->AngularVelocity;
  }

  Joint->mB = R32Max;
  Joint->IB_inv = {};
  v3 VB = {};
  v3 WB = {};
  if (DB)
  {
    Joint->mB = DB->Mass;
    Joint->IB_inv = DB->I_inv;
    VB = DB->LinearVelocity;
    WB = DB->AngularVelocity;
  }

  v4 RA = SA->Rotation;
  v4 RB = SB->Rotation;

  Joint->rA = RotateQuaternion(RA, Joint->LocalAnchorA - Joint->LocalCenterA);
  Joint->rB = RotateQuaternion(RB, Joint->LocalAnchorB - Joint->LocalCenterB);

  r32 mA_inv = 1.f / Joint->mA;
  r32 mB_inv = 1.f / Joint->mB;


}
