#pragma once

#include "math/affine_transformations.h"

void LookAt( component_camera* Camera, v3 aFrom,  v3 aTo,  v3 aTmp = V3(0,1,0) )
{
  v3 Forward = Normalize(aFrom - aTo);
  v3 Right   = Normalize( CrossProduct(aTmp, Forward) );
  v3 Up      = Normalize( CrossProduct(Forward, Right) );

  m4 CamToWorld;
  CamToWorld.r0 = V4(Right, 0);
  CamToWorld.r1 = V4(Up,    0);
  CamToWorld.r2 = V4(Forward, 0);
  CamToWorld.r3 = V4(aFrom, 1);
  CamToWorld = Transpose(CamToWorld);

  Camera->V = RigidInverse(CamToWorld);
  AssertIdentity(Camera->V * CamToWorld, 0.001 );
}

struct ray
{
  v3 Origin;
  v3 Direction;
};

ray GetRayFromCamera(component_camera* Camera, v2 MousePos)
{
  ray Result{};
  v2 ScreenSpace = CanonicalToScreenSpace(MousePos);
  r32 HalfAngleOfView = 0.5f * (Pi32/180.f)*Camera->AngleOfView;
  r32 TanHalfAngle = Tan(HalfAngleOfView);
  v2 DirectionOfPixel = V2((ScreenSpace.X * Camera->AspectRatio * TanHalfAngle),
                           (ScreenSpace.Y * TanHalfAngle));
  m4 InvView = RigidInverse(Camera->V);
  Result.Origin = V3(InvView * V4(0,0,0,1));
  Result.Direction = Normalize(V3(InvView * V4(DirectionOfPixel,-1,0)));
  return Result;
}

void GetCameraDirections(component_camera* Camera, v3* Up, v3* Right, v3* Forward)
{
  m4 V_Inv = Transpose(RigidInverse(Camera->V));
  if (Right)
    *Right = V3(V_Inv.r0);
  if(Up)
    *Up = V3(V_Inv.r1);
  if(Forward)
    *Forward = V3(V_Inv.r2);
}

void TranslateCamera( component_camera* Camera, const v3& DeltaP  )
{
  Camera->DeltaPos += DeltaP;
}

void RotateCamera( component_camera* Camera, const r32 DeltaAngle, const v3& Axis )
{
  Camera->DeltaRot = GetRotationMatrix( DeltaAngle, V4(Axis, 0) ) * Camera->DeltaRot;
}

void UpdateViewMatrix(  component_camera* Camera )
{
  m4 CamToWorld = RigidInverse( Camera->V );
  AssertIdentity( CamToWorld*Camera->V,0.001 );

  v4 NewUpInCamCoord    = Column( Camera->DeltaRot, 1 );
  v4 NewUpInWorldCoord  = CamToWorld * NewUpInCamCoord;

  v4 NewAtDirInCamCoord  = Column( Camera->DeltaRot, 2 );
  v4 NewAtDirInWorldCord = CamToWorld * NewAtDirInCamCoord;
  v4 NewPosInWorldCoord  = Column( CamToWorld, 3 ) + CamToWorld*V4( Camera->DeltaPos, 0 );

  v4 NewAtInWorldCoord   = NewPosInWorldCoord-NewAtDirInWorldCord;

  LookAt(Camera, V3(NewPosInWorldCoord), V3(NewAtInWorldCoord), V3(NewUpInWorldCoord) );

  Camera->DeltaRot = M4Identity();
  Camera->DeltaPos = V3( 0, 0, 0 );
}


void SetOrthoProj( component_camera* Camera, r32 aNear, r32 aFar, r32 aRight, r32 aLeft, r32 aTop, r32 aBottom )
{
  Assert(aNear < 0);
  Assert(aFar  > 0);
  aFar     = -aFar;
  aNear      = -aNear;
  r32 rlSum  = aRight+aLeft;
  r32 rlDiff = aRight-aLeft;

  r32 tbSum  = aTop+aBottom;
  r32 tbDiff = aTop-aBottom;

  r32 fnSum  = aFar+aNear;
  r32 fnDiff = aFar-aNear;

  Camera->P =  M4( 2/rlDiff,          0,        0, -rlSum/rlDiff,
                          0,   2/tbDiff,        0, -tbSum/tbDiff,
                          0,          0, 2/fnDiff, -fnSum/fnDiff,
                          0,          0,        0,             1);
}

void SetOrthoProj( component_camera* Camera, r32 n, r32 f )
{
  r32 scale = - Tan( Camera->AngleOfView * 0.5f * Pi32 / 180.f ) * n;
  r32 r = Camera->AspectRatio * scale;
  r32 l = -r;
  r32 t = scale;
  r32 b = -t;
  SetOrthoProj( Camera, n, f, r, l, t, b );
}

void SetPerspectiveProj( component_camera* Camera, r32 n, r32 f, r32 r, r32 l, r32 t, r32 b )
{
  Assert(n > 0);
  Assert(f > n);

  r32 rlSum  = r+l;
  r32 rlDiff = r-l;

  r32 tbSum  = t+b;
  r32 tbDiff = t-b;

  r32 fnSum  = f+n;
  r32 fnDiff = f-n;

  r32 n2 = n*2;

  r32 fn2Prod = 2*f*n;

  Camera->P =  M4( n2/rlDiff,     0,      rlSum/rlDiff,         0,
                       0,     n2/tbDiff,  tbSum/tbDiff,         0,
                       0,         0,     -fnSum/fnDiff, -fn2Prod/fnDiff,
                       0,         0,           -1,              0);

}

void SetPerspectiveProj( component_camera* Camera, r32 n, r32 f )
{
  r32 AspectRatio = Camera->AspectRatio;
  r32 scale = Tan( Camera->AngleOfView * 0.5f * Pi32 / 180.f ) * n;
  r32 r     = AspectRatio * scale;
  r32 l     = -r;
  r32 t     = scale;
  r32 b     = -t;
  SetPerspectiveProj( Camera, n, f, r, l, t, b );
}

#if 0
void setPinholeCamera( r32 filmApertureHeight, r32 filmApertureWidth,
                       r32 focalLength, r32 nearClippingPlane,
                       r32 inchToMM = 25.4f )
{
  r32 top = ( nearClippingPlane* filmApertureHeight * inchToMM * 0.5 ) /  (r32) focalLength;
  r32 bottom = -top;
  r32 filmAspectRatio = filmApertureWidth / (r32) filmApertureHeight;
  r32 left = bottom * filmAspectRatio;
  r32 left = -right;

  setPerspectiveProj( r32 aNear, r32 aFar, r32 aLeft, r32 aRight, r32 aTop, r32 aBottom );
}
#endif

void SetCameraComponent(component_camera* Camera, r32 AngleOfView, r32 AspectRatio )
{
  Camera->AngleOfView  = AngleOfView;
  Camera->AspectRatio = AspectRatio;
  Camera->DeltaRot = M4Identity();
  Camera->DeltaPos = V3(0,0,0);
  Camera->V = M4Identity();
  Camera->P = M4Identity();
  SetPerspectiveProj( Camera, 1, 1000 );
}

void gluPerspective(
  const r32& angleOfView, const r32& imageAspectRatio,
  const r32& n, const r32& f, r32& b, r32& t, r32& l, r32& r)
{
  r32 scale = (r32) Tan( angleOfView * 0.5f * Pi32 / 180.f) * n;
  r = imageAspectRatio * scale, l = -r;
  t = scale, b = -t;
}

// set the OpenGL perspective projection matrix
void glFrustum( const r32& b, const r32& t,
                const r32& l, const r32& r,
                const r32& n, const r32& f, m4& M)
{
  // set OpenGL perspective projection matrix
  v4 R1 = V4( 2 * n / (r - l), 0, 0, 0);
  v4 R2 = V4( 0, 2 * n / (t - b), 0, 0);
  v4 R3 = V4( (r + l) / (r - l), (t + b) / (t - b), -(f + n) / (f - n), -1);
  v4 R4 = V4( 0, 0, -2 * f * n / (f - n), 0);

  M.r0 = R1;
  M.r1 = R2;
  M.r2 = R3;
  M.r3 = R4;

  M = Transpose(M);
}
