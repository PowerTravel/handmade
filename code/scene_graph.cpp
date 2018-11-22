

/*	
 *	Class: 		BaseCallback
 *	Purpose: 	BaseClass for all callback classes
 *	Misc:		All other callbacks inherit from this one
 */
class BaseCallback{

	public:
		virtual void execute() = 0;	

};

class CameraMovementCallback : public BaseCallback
{
	public:

		CameraMovementCallback(game_controller_input* aController)
		{
			Controller = aController;
		};
		virtual ~CameraMovementCallback(){};

		void execute()
		{
			if( Controller->IsAnalog )
			{
				if(Controller->Start.EndedDown)
				{

				}else{

				}

				if(Controller->RightStickLeft.EndedDown)
				{

				}
				if(Controller->RightStickRight.EndedDown)
				{
				}
				if(Controller->RightStickUp.EndedDown)
				{
				}
				if(Controller->RightStickDown.EndedDown)
				{
				}
				if(Controller->RightShoulder.EndedDown)
				{

				}
			}	
		};

		game_controller_input* Controller=0;
};


/*	
 *	Class: 		BaseNode
 *	Purpose: 	BaseClass for the Nodes classes. 
 *	Misc:		All other Nodes inherit from this one.
 */

class BaseVisitor;
class BaseNode;

class BaseNode
{

	public:

		BaseNode(){};
		virtual ~BaseNode(){};

		virtual void update()
		{
			if(Callback)
			{
				Callback->execute();
			}
		};

		virtual void acceptVisitor( BaseVisitor* ) = 0;
		virtual void connectCallback( BaseCallback* bc )
		{
			Callback = bc;
		};

		BaseNode* FirstChild = 0;
		BaseNode* NextSibling = 0;
		int32 nrChildren = 0;
		BaseCallback* Callback = 0;

		void pushChild( BaseNode* aNode )
		{
			if( FirstChild == 0)
			{
				FirstChild = aNode;
				++nrChildren;
				return;
			}

			BaseNode* Child = FirstChild;
			while( Child->NextSibling )
			{
				Child = Child->NextSibling;
			}

			Child->NextSibling = aNode;
			++nrChildren;
		}
};

//class TransformNode;
class RootNode;
class CameraNode;
class TransformNode;
class BitmapNode; 	// leaf
class GeometryNode; // leaf


class BaseVisitor
{
	public:
		BaseVisitor(){};
		virtual ~BaseVisitor(){};

		// A method for traversing the scene graph depth first
		virtual void traverse( BaseNode* aNode ) final
		{
			if(aNode == 0){return;}

			aNode->acceptVisitor( this );

			traverse( aNode->FirstChild );

			traverse( aNode->NextSibling );

		}

		// Each node has their own apply function
		virtual void apply( RootNode* aNode ){};
		virtual void apply( CameraNode* aNode){};
		virtual void apply( BitmapNode* aNode){};
		virtual void apply( GeometryNode* aNode){};
		virtual void apply( TransformNode* aNode){};
};


/*	
 *	Class: 		UpdateVisitor
 *	Purpose: 	Traverses the scenegraph and updates it.
 */

class UpdateVisitor : public BaseVisitor
{
	public:
		UpdateVisitor( )
		{

		};
		virtual ~UpdateVisitor()
		{
		};

		void apply( CameraNode* n) override;
};


/*	
 *	Class: 		RenderVisitor
 *	Purpose: 	Traverses the scenegraph and renders it.
 */

class RenderVisitor : public BaseVisitor
{
	public:
		RenderVisitor(  render_push_buffer* aBuffer )
		{
			Assert(aBuffer->OffscreenBuffer);
			Assert(aBuffer->Arena);

			RenderPushBuffer = aBuffer;;


			game_offscreen_buffer* Buffer = aBuffer->OffscreenBuffer;
			RenderPushBuffer->R = M4( Buffer->Width/2.f,  0, 0, Buffer->Width/2.f, 
				    			     0, Buffer->Height/2.f, 0, Buffer->Height/2.f, 
				    			     0, 0, 1, 0,
				    			     0, 0, 0, 1);

			T = M4Identity();
			RenderPushBuffer->V = M4Identity();
			RenderPushBuffer->P = M4Identity();
		};
		virtual ~RenderVisitor()
		{
		};

		void apply( RootNode* n) override
		{
			// Clear screen to black
			game_offscreen_buffer* Buffer = RenderPushBuffer->OffscreenBuffer;
			DrawRectangle(Buffer, 0,0, (real32) Buffer->Width,   (real32) Buffer->Height, 1,1,1);
			DrawRectangle(Buffer, 1,1, (real32) Buffer->Width-2,   (real32) Buffer->Height-2, 0.3,0.3,0.3);

			T = M4Identity();
		};

		void apply( CameraNode* n) override;
		void apply( BitmapNode* n) override;
		void apply( GeometryNode* n) override;
		void apply( TransformNode* n ) override;

	private:

		// Todo: Make proper Stack
		struct trace
		{
			m4 CumulativeMatrix;
			uint32 Uses;
		};

		void pushTrace( m4 Mat, int32 Uses )
		{
			Assert(TraceDepth < MaxTraces);
			T = T*Mat;
			trace& tr = Trace[TraceDepth];
			tr.CumulativeMatrix = T;
			tr.Uses = Uses;
			TraceDepth++;
		}

		void popTrace( )
		{
			while( TraceDepth > 0 )
			{
				if( Trace[TraceDepth-1].Uses > 1 )
				{
					Trace[TraceDepth-1].Uses--;
					break;
				}
				--TraceDepth;
			}

			if(TraceDepth > 0)
			{
				T = Trace[TraceDepth-1].CumulativeMatrix;
			}else{
				T = M4Identity();
			}
		}

		m4 T; // ModelMatrix
		render_push_buffer* RenderPushBuffer;

		trace Trace[10];
		int32 TraceDepth = 0;
		int32 MaxTraces = 10;
};

/*
 * Class:	RootNode
 * Purpose:	Root of renderTree
 */
class RootNode : public BaseNode
{
	public:

		RootNode(){};
		virtual ~RootNode(){};

		void acceptVisitor( BaseVisitor* v ) override
		{
			v->apply(this);
		};

};

/*
 * Class:	CameraNode
 * Purpose:	Holds camera and projection matrices
 */
class CameraNode : public BaseNode
{
	public:

		CameraNode( v3 aFrom, v3 aTo, real32 aNear, real32 aFar, real32 aLeft, real32 aRight, real32 aTop, real32 aBottom )
		{
			DeltaRot = M4Identity();
			DeltaPos = V3(0,0,0);
			lookAt( aFrom,  aTo );
			local_persist real32 t = 0;
			//setOrthoProj( aNear, aFar, aLeft, aRight, aTop, aBottom );
			setPerspectiveProj( aNear, aFar, aLeft, aRight, aTop, aBottom);
			t+=0.01;
			if(t >= 20 * Pi32)
			{
				t -= 20*Pi32;
			}
		};

		virtual ~CameraNode(){};

		void acceptVisitor( BaseVisitor* v ) override
		{
			v->apply(this);
		};

		void setOrthoProj( real32 aNear, real32 aFar, real32 aLeft, real32 aRight, real32 aTop, real32 aBottom )
		{
			aFar = -aFar;
			aNear = -aNear;
			real32 rlSum  = aRight+aLeft;
			real32 rlDiff = aRight-aLeft;

			real32 tbSum  = aTop+aBottom;
			real32 tbDiff = aTop-aBottom;

			real32 fnSum  = aFar+aNear;
			real32 fnDiff = aFar-aNear;

			P =  M4( 2/rlDiff,         0,        0, -rlSum/rlDiff, 
						    0,   2/tbDiff,       0, -tbSum/tbDiff, 
						    0,         0, 2/fnDiff, -fnSum/fnDiff,
						    0,         0,        0,             1);
		}


		void setPerspectiveProj( real32 aNear, real32 aFar, real32 aLeft, real32 aRight, real32 aTop, real32 aBottom )
		{
			real32 rlSum  = aRight+aLeft;
			real32 rlDiff = aRight-aLeft;

			real32 tbSum  = aTop+aBottom;
			real32 tbDiff = aTop-aBottom;

			real32 fnSum  = aFar+aNear;
			real32 fnDiff = aFar-aNear;

			real32 n2 = aNear*2;

			real32 fn2Prod = 2*aFar*aNear;

			P =  M4( n2/rlDiff,         0,  rlSum/rlDiff,               0, 
			                 0, n2/tbDiff,  tbSum/tbDiff,               0, 
			                 0,         0, -fnSum/fnDiff, -fn2Prod/fnDiff,
			                 0,         0,            -1,              -0);
		}

		void setPerspectiveProj( const real32 angleOfView, const real32 imageAspectRatio,
							 	const real32 n, const real32 f)
		{
			real32 scale = Tan(angleOfView * 0.5f * Pi32 / 180.f) * n;
			real32 r = imageAspectRatio * scale;
			real32 l = -r;
			real32 t = scale;
			real32 b = -t;

			setPerspectiveProj( n, f, r, l, t, b );
		} 

//		void setPinholeCamera( real32 filmApertureHeight, real32 filmApertureWidth, 
//			 					 real32 focalLength, 		real32 nearClippingPlane, 
//			 					 real32 inchToMM = 25.4f )
//		{	
//			real32 top = ( nearClippingPlane* filmApertureHeight * inchToMM * 0.5 ) /  (real32) focalLength;
//			real32 bottom = -top;
//			real32 filmAspectRatio = filmApertureWidth / (real32) filmApertureHeight;
//			real32 left = bottom * filmAspectRatio;
//			real32 left = -right;
//
//			setPerspectiveProj( real32 aNear, real32 aFar, real32 aLeft, real32 aRight, real32 aTop, real32 aBottom );
//
//		}
		void lookAt( v3 aFrom,  v3 aTo,  v3 aTmp = V3(0,1,0) )
		{
			v3 Forward = normalize(aFrom - aTo);
			v3 Right   = normalize( cross(aTmp, Forward) );
			v3 Up      = normalize( cross(Forward, Right) );

			m4 CamToWorld; 
			CamToWorld.r0 = V4(Right,	0);
			CamToWorld.r1 = V4(Up,		0);
			CamToWorld.r2 = V4(Forward,	0);
			CamToWorld.r3 = V4(aFrom,	1);
			CamToWorld = Transpose(CamToWorld);
	
			V = RigidInverse(CamToWorld);
			AssertIdentity(V * CamToWorld, 0.001 );

		}

		void Translate( v3 DeltaP  )
		{
			DeltaPos = DeltaP;
		}

		void Rotate( real32 DeltaAngle, v3 Axis )
		{
			DeltaRot = GetRotationMatrix( DeltaAngle, Axis );
		}

		void Update()
		{
			m4 CamToWorld = RigidInverse(V);
			AssertIdentity(CamToWorld*V,0.001);

			v4 NewUpInCamCoord    = Column(DeltaRot,1);
			v4 NewUpInWorldCoord  = CamToWorld * NewUpInCamCoord;

			v4 NewAtDirInCamCoord  = Column(DeltaRot,2);
			v4 NewAtDirInWorldCord = CamToWorld * NewAtDirInCamCoord;
			v4 NewPosInWorldCoord  = Column(CamToWorld,3) + CamToWorld*V4( DeltaPos, 0 );

			v4 NewAtInWorldCoord   = NewPosInWorldCoord-NewAtDirInWorldCord;

			lookAt( V3(NewPosInWorldCoord), V3(NewAtInWorldCoord), V3(NewUpInWorldCoord) );
	
			DeltaRot = M4Identity();
			DeltaPos = V3( 0, 0, 0 );
		}

		m4 DeltaRot;
		v3 DeltaPos;
		m4 V;   // View Matrix
		m4 P;
};


/*
 * Class:	TransformNode
 * Purpose:	Holds all Transformations
 */
class TransformNode : public BaseNode
{
	public:

		TransformNode()
		{
			T =  M4Identity();
		};

		virtual ~TransformNode(){};

		void Translate( v3 dR )
		{
			T.E[3]  += dR.X;
			T.E[7]  += dR.Y;
			T.E[11] += dR.Z;
		}

		void Rotate( real32 Angle, v3 Axis )
		{
			m4 dR = GetRotationMatrix( Angle, Axis );

			m4 tto = M4(1,0,0,-T.E[3],
						0,1,0,-T.E[7],
						0,0,1,-T.E[13],
						0,0,0,1);

			m4 bfo = M4(1,0,0, T.E[3],
						0,1,0, T.E[7],
						0,0,1, T.E[13],
						0,0,0,1);

			T =  bfo * dR * tto * T;
		}

		void Scale( v3 Scale ) 
		{
			T.E[0] *= Scale.X;
			T.E[5] *= Scale.Y;
			T.E[10] *= Scale.Z;
		}

		void acceptVisitor( BaseVisitor* v ) override
		{
			v->apply(this);
		}

		m4 T; // Model Matrix
};



/*
 * Class:	BitmapNode
 * Purpose:	Holds all bitmaps
 * Misc: 	Is always leaf
 */
class BitmapNode : public BaseNode
{
	public:

		BitmapNode( loaded_bitmap aBMP )
		{ 
			BMP = aBMP; 
		};
		virtual ~BitmapNode(){};

		void acceptVisitor( BaseVisitor* v ) override
		{
			v->apply(this);
		};

		loaded_bitmap BMP;
};


/*
 * Class:	GeometryNode
 * Purpose:	Holds all triangles
 * Misc: 	Is always leaf
 */
class GeometryNode : public BaseNode
{
	public:

		GeometryNode(){};

		GeometryNode(obj_geometry Geometry)
		{
			Object = Geometry;

		}
		virtual ~GeometryNode(){};

		void acceptVisitor( BaseVisitor* v ) override
		{
			v->apply(this);
		};


		obj_geometry Object;
};

void RenderVisitor::apply( BitmapNode* n)
{
//	v4 obj = mCumulativeMat * mObject;
//	obj = Rasterization*obj;
//	BlitBMP( mBuffer, (real32) obj.X, (real32) obj.Y, n->BMP);
}

void RenderVisitor::apply( GeometryNode* n)
{
	local_persist real32 dt = 0;
	if( dt > 20*Pi32 )
	{
		dt -= 20*Pi32;
	}else{
		dt = dt +Pi32*0.1f;
	}

	obj_geometry& O = n->Object;	

	m4 ModelView = RenderPushBuffer->V * T;
	for(int32 i = 0; i < O.nt; ++i)
	{
		triangle& t = O.t[i];

#if 0
		v4 fn = ModelView*t.n;
		if( fn*V4(0,0,1,0) < 0 )
		{
			continue;
		}
#endif

		render_entity Triangle = {};

		Triangle.normal = T*t.n;
		if(Triangle.normal * V4(0,1,0,0) > 0 )
		{
			Triangle.AmbientProduct  	= V4(0.0,0.7,0,0);
			Triangle.DiffuseProduct  	= V4(0,0.0,0,1);
			Triangle.SpecularProduct 	= V4(0,0,0.0,1);
		}else{
			Triangle.AmbientProduct  	= V4(0.4,0,0,1);
			Triangle.DiffuseProduct  	= V4(0,0.0,0,1);
			Triangle.SpecularProduct 	= V4(0,0,0.0,1);
		}
		Triangle.LightPosition 		= V4(1,1,1,1);
	
		Triangle.vertices[0] = T*O.v[ t.vi[0] ];
		Triangle.vertices[1] = T*O.v[ t.vi[1] ];
		Triangle.vertices[2] = T*O.v[ t.vi[2] ];


		Triangle.verticeNormals[0] = T*O.vn[ t.vni[0] ];
		Triangle.verticeNormals[1] = T*O.vn[ t.vni[1] ];
		Triangle.verticeNormals[2] = T*O.vn[ t.vni[2] ];


		PushBuffer( RenderPushBuffer, Triangle );
	}

	popTrace();
}

void RenderVisitor::apply( CameraNode* n)
{
	RenderPushBuffer->V = n->V;
	RenderPushBuffer->P = n->P;
}


void UpdateVisitor::apply( CameraNode* n)
{
	n->update();
};


void RenderVisitor::apply( TransformNode* n )
{
	pushTrace( n->T, n->nrChildren );
}