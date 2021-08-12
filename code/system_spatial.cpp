#include "entity_components.h"
#include "handmade_tile.h"
#include "utility_macros.h"
#include "math/aabb.h"
#include "dynamic_aabb_tree.h"
#include "gjk_narrow_phase.h"
#include "epa_collision_data.h"

#define WARM_STARTING_FRACTION 1.0f

#define FRICTIONAL_COEFFICIENT 0.2f
#define BAUMGARTE_COEFFICIENT  0.4f
#define RESTITUTION_COEFFICIENT 0.0f
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

internal r32 GetLambda(  v3 V[], v3 J[], v3 InvMJ[], r32 Bias)
{
  r32 Numerator   = -(DotProductV12xV12( J, V) + Bias);
  r32 Denominator =   DotProductV12xV12( J, InvMJ);

  r32 Result = Numerator / Denominator;
  return Result;
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


void GetVelocityVector(component_dynamics * DA, component_dynamics * DB, v3* Velocity)
{
  if (DA)
  {
    Velocity[0] = DA->LinearVelocity;
    Velocity[1] = DA->AngularVelocity;
  }

  if (DB)
  {
    Velocity[2] = DB->LinearVelocity;
    Velocity[3] = DB->AngularVelocity;
  }
}

void AddVelocityVector(component_dynamics * DA, component_dynamics * DB, v3* DeltaVelocity)
{
  if (DA)
  {
    DA->LinearVelocity += DeltaVelocity[0];
    DA->AngularVelocity += DeltaVelocity[1];
  }

  if (DB)
  {
    DB->LinearVelocity += DeltaVelocity[2];
    DB->AngularVelocity += DeltaVelocity[3];
  }
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

internal void DoWarmStarting( contact_manifold* FirstManifold  )
{
  TIMED_FUNCTION();
  contact_manifold* Manifold = FirstManifold;
  while (Manifold)
  {
    vector_list<contact_data>* Contacts = &Manifold->Contacts;
    contact_data* Contact = Contacts->First( );

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

        component_dynamics* DynamicsA = GetDynamicsComponent(Manifold->EntityIDA);
        component_dynamics* DynamicsB = GetDynamicsComponent(Manifold->EntityIDB);
        v3 DeltaVTot[4] = {
          DeltaV[0] + DeltaV1[0] + DeltaV2[0],
          DeltaV[1] + DeltaV1[1] + DeltaV2[1],
          DeltaV[2] + DeltaV1[2] + DeltaV2[2],
          DeltaV[3] + DeltaV1[3] + DeltaV2[3]
        };
        AddVelocityVector(DynamicsA, DynamicsB, DeltaVTot);
      }
      Contact = Contacts->Next(Contact);
    }
    Manifold = Manifold->Next;
  }
}

// NOTE(Jakob):

inline void IntegrateVelocities( r32 dt )
{
  TIMED_FUNCTION();

  BeginScopedEntityManagerMemory();

  component_result* ComponentList = GetComponentsOfType(GlobalGameState->EntityManager, COMPONENT_FLAG_DYNAMICS);
  while(Next(GlobalGameState->EntityManager, ComponentList))
  {
    component_spatial*  S = GetSpatialComponent(ComponentList);
    component_collider* C = GetColliderComponent(ComponentList);
    component_dynamics* D = GetDynamicsComponent(ComponentList);

    // Forward euler
    // TODO(Jakob): Investigate other more stable integration methods
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
      GetVelocityVector(DynamicsA, DynamicsB, V);

      contact_data_cache* Cache = &Contact->Cache;

      v3 ContactNormal     = Contact->ContactNormal;
      v3 ContactPointDiff  = V3(SpatialA->ModelMatrix * V4(Contact->A_ContactModelSpace,1)) -
        V3(SpatialB->ModelMatrix * V4(Contact->B_ContactModelSpace,1));
      r32 PenetrationDepth = ContactPointDiff * ContactNormal;

      r32 Restitution       = getRestitutionCoefficient(V, RESTITUTION_COEFFICIENT, ContactNormal, SLOP);
      r32 Baumgarte         = getBaumgarteCoefficient(dtForFrame, BAUMGARTE_COEFFICIENT,  PenetrationDepth, SLOP);
      r32 Bias = Baumgarte + Restitution;
      r32 Lambda            = GetLambda( V, Cache->J, Cache->InvMJ, Bias);

      r32 NewLambda = Maximum(Cache->AccumulatedLambda + Lambda, 0);
      r32 LambdaDiff = NewLambda - Cache->AccumulatedLambda;
      Cache->AccumulatedLambda = NewLambda;


      if(LambdaDiff > 0)
      {
        v3 DeltaV[4] = {};
        ScaleV12(LambdaDiff, Cache->InvMJ, DeltaV);
        AddVelocityVector(DynamicsA, DynamicsB, DeltaV);
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
      GetVelocityVector(DynamicsA, DynamicsB, V);

      contact_data_cache* Cache = &Contact->Cache;

      r32 Kf = FRICTIONAL_COEFFICIENT;
      r32 ClampRange = Kf * Cache->AccumulatedLambda;

      r32 LambdaN1 = GetLambda( V, Cache->Jn1, Cache->InvMJn1, 0);
      r32 LambdaN2 = GetLambda( V, Cache->Jn2, Cache->InvMJn2, 0);
      LambdaN1 = Clamp(Cache->AccumulatedLambdaN1 + LambdaN1, -ClampRange, ClampRange);
      LambdaN2 = Clamp(Cache->AccumulatedLambdaN2 + LambdaN2, -ClampRange, ClampRange);
      r32 LambdaDiffN1 = LambdaN1 - Cache->AccumulatedLambdaN1;
      r32 LambdaDiffN2 = LambdaN2 - Cache->AccumulatedLambdaN2;

      Cache->AccumulatedLambdaN1 = LambdaN1;
      Cache->AccumulatedLambdaN2 = LambdaN2;

      v3 DeltaV1[4] = {};
      v3 DeltaV2[4] = {};

      ScaleV12(LambdaDiffN1, Cache->InvMJn1, DeltaV1);
      ScaleV12(LambdaDiffN2, Cache->InvMJn2, DeltaV2);

      if (!(Equals(Abs(LambdaDiffN1),0) && Equals(Abs(LambdaDiffN2),0)))
      {
        v3 DeltaV[4] = {
          (DeltaV1[0] + DeltaV2[0]),
          (DeltaV1[1] + DeltaV2[1]),
          (DeltaV1[2] + DeltaV2[2]),
          (DeltaV1[3] + DeltaV2[3])
        };
        AddVelocityVector(DynamicsA, DynamicsB, DeltaV);
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

  BeginScopedEntityManagerMemory();
  component_result* Components = GetComponentsOfType(GlobalGameState->EntityManager, COMPONENT_FLAG_DYNAMICS);
  while(Next(GlobalGameState->EntityManager,Components))
  {
    component_spatial* S = GetSpatialComponent(Components);
    component_dynamics* D = GetDynamicsComponent(Components);
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
    BeginScopedEntityManagerMemory();
    component_result* Components = GetComponentsOfType(EM, COMPONENT_FLAG_CAMERA);
    Assert(Next(EM, Components));
    component_camera* Camera = GetCameraComponent(Components);

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

        r32 Restitution = getRestitutionCoefficient(V, RESTITUTION_COEFFICIENT, Normal, SLOP);
        r32 Baumgarte = getBaumgarteCoefficient(World->dtForFrame, BAUMGARTE_COEFFICIENT, Length*ScaleFactor, SLOP);
        r32 Bias = Baumgarte + Restitution;
        r32 Lambda = GetLambda( V, Jacobian, InvMJ, Bias);
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

joint_constraint CreateJointConstraint(u32 EntityA, v3 LocalAnchorA, u32 EntityB, v3 LocalAnchorB, v3 GlobalRotationAxis)
{
  joint_constraint Joint = {};
  Joint.EntityA = EntityA;
  Joint.EntityB = EntityB;
  Joint.LocalAnchorA = LocalAnchorA;
  Joint.LocalAnchorB = LocalAnchorB;
  component_spatial* A = GetSpatialComponent(EntityA);
  component_spatial* B = GetSpatialComponent(EntityB);

  Joint.LocalCenterA = ToLocal(A,A->Position);
  Joint.LocalCenterB = ToLocal(B,B->Position);

  component_dynamics* DA = GetDynamicsComponent(EntityA);
  component_dynamics* DB = GetDynamicsComponent(EntityB);
  m3 Empty = {};
  Joint.InvMass[0] = (DA ? (1.f/DA->Mass) : 0) * M3Identity();
  Joint.InvMass[1] =  DA ? DA->I_inv : Empty;
  Joint.InvMass[2] = (DB ? (1.f/DB->Mass) : 0) * M3Identity();
  Joint.InvMass[3] =  DB ? DB->I_inv : Empty;

  return Joint;
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

// https://github.com/erincatto/box2d/blob/master/src/dynamics/b2_revolute_joint.cpp
void InitiateJointVelocityConstraints(joint_constraint* Joint)
{
  component_spatial  * SA = GetSpatialComponent(Joint->EntityA);
  component_spatial  * SB = GetSpatialComponent(Joint->EntityB);
  component_dynamics * DA = GetDynamicsComponent(Joint->EntityA);
  component_dynamics * DB = GetDynamicsComponent(Joint->EntityB);

  v3 Velocity[4] = {};
  GetVelocityVector(DA, DB, Velocity);

  v4 RA = SA->Rotation;
  v4 RB = SB->Rotation;

  v3 rA = RotateQuaternion(RA, Joint->LocalAnchorA - Joint->LocalCenterA);
  v3 rB = RotateQuaternion(RB, Joint->LocalAnchorB - Joint->LocalCenterB);
  Joint->rA = rA;
  Joint->rB = rB;

  Joint->d = ToGlobal(SB, Joint->LocalAnchorB) - ToGlobal(SA, Joint->LocalAnchorA);

  if(Norm(Joint->d)>SLOP)
  {
    v3 d = Normalize(Joint->d);
    Joint->Jacobian[0] = -d;
    Joint->Jacobian[1] = -CrossProduct(rA,d);
    Joint->Jacobian[2] =  d;
    Joint->Jacobian[3] =  CrossProduct(rB,d);
  }

  MultiplyDiagonalM12V12(Joint->InvMass, Joint->Jacobian, Joint->InvMJ);

  v3 DeltaVelocity[4] = {};
  ScaleV12(Joint->Lambda, Joint->InvMJ, DeltaVelocity);
  AddVelocityVector(DA, DB, DeltaVelocity);
}

void SolveJointVelocityConstraints(joint_constraint* Joint, r32 dt)
{
  if(Norm(Joint->d) > SLOP)
  {
    component_dynamics * DA = GetDynamicsComponent(Joint->EntityA);
    component_dynamics * DB = GetDynamicsComponent(Joint->EntityB);

    v3 Velocity[4] = {};
    GetVelocityVector(DA, DB, Velocity);

    r32 BaumBias = (Norm(Joint->d)-SLOP)/dt;
    r32 Lambda = GetLambda(Velocity, Joint->Jacobian, Joint->InvMJ, BaumBias );
    r32 NewLambda = Joint->Lambda + Lambda;
    r32 LambdaDiff = NewLambda - Joint->Lambda;
    Joint->Lambda = NewLambda;

    v3 DeltaV[4] = {};
    ScaleV12(LambdaDiff, Joint->InvMJ, DeltaV);
    AddVelocityVector(DA,DB,DeltaV);
    }
}

/*c
plot_function_samples(100);
plot_title('My Plot');
plot_xaxis('x', -4,4);
plot_yaxis('y', -4,4);
plot(sin(time()-x));
plot( sin(time()+x), cos(time()-x-pi/2) );

*/

void SpatialSystemUpdate( world* World )
{
  TIMED_FUNCTION();

  world_contact_chunk* WorldContacts =  World->ContactManifolds;
  r32 dt = World->dtForFrame;
  IntegrateVelocities(dt);

  InitiateJointVelocityConstraints(&World->Joint);

  InitiateContactVelocityConstraints(WorldContacts->FirstManifold);

  CastRay(GlobalGameState->Input, World);

  DoWarmStarting(WorldContacts->FirstManifold);

  BEGIN_BLOCK(SolveConstraints);
  for (u32 i = 0; i < SLOVER_ITERATIONS; ++i)
  {
    // NOTE(Jakob): Solve Frictional constraints first because non-penetration is more important
    SolveJointVelocityConstraints(&World->Joint, dt);
    SolveFrictionalConstraints(WorldContacts->FirstManifold);
    SolveNonPenetrationConstraints(dt, WorldContacts->FirstManifold);
  }
  END_BLOCK(SolveConstraints);

  Platform.DEBUGPrint("%f \n", World->Joint.Lambda);

  IntegratePositions(dt);

}