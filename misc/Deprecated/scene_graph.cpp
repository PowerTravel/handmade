
#if 0

struct CameraNode_;
struct GeometryNode_;

//struct scene_graph_visitor_interface
//{
//	void (*Apply)( CameraNode_*);
//	void (*Apply)( GeometryNode_*);
//};
//
//struct scene_graph_visitor
//{
//	void* instance;
//	const scene_graph_visitor_interface* interface;
//};
//
struct RenderVisitor_
{
	void Apply(GeometryNode_*)
	{
		int A = 10;
	};
	void Apply(CameraNode_*)
	{
		int B = 20;
	};
};
//void visitor_Apply(scene_graph_visitor* v, CameraNode_* n)
//{
//	(v->interface->Apply)(n);
//}
//
//void
//RenderVisitor_Apply(RenderVisitor_* v, GeometryNode_* n)
//{
//    v->Apply(n);
//}
//
//void
//RenderVisitor_Apply(RenderVisitor_* v, CameraNode_* n)
//{
//    v->Apply(n);
//}


struct scene_graph_node_interface 
{
    void (*AcceptVisitor)( RenderVisitor_* v);
};

struct scene_graph_node
{
    void *instance;
    const scene_graph_node_interface* interface;
};

void AcceptVisitor(scene_graph_node* n, RenderVisitor_* v)
{
	(n->interface->AcceptVisitor)( v);
}


struct CameraNode_
{
    m4 V;
    m4 T;
};

struct GeometryNode_
{
    obj_geometry* O;
};


void
GeometryNode_AcceptVisitor(GeometryNode_* n, RenderVisitor_* v)
{
    v->Apply(n);
}

void
CameraNode_AcceptVisitor(CameraNode_* n, RenderVisitor_* v)
{
    v->Apply(n);
}

enum class InterfaceEnum
{
	SCENEGRAPH_VISITOR_RENDER,

	SCENEGRAPH_NODE_CAMERA,
	SCENEGRAPH_NODE_GEOMETRY
};

struct interface_table
{
//	scene_graph_visitor_interface VisitorBase;
//	scene_graph_visitor_interface VisitorRender;

	scene_graph_node_interface NodeGeometry;
	scene_graph_node_interface NodeCamera;
};

scene_graph_node*
Create_(memory_arena* Arena, InterfaceEnum i)
{
	local_persist interface_table iTable
	{
//		.VisitorRender 	= (void (*)( scene_graph_node* )) Apply_Render

		(void (*)(RenderVisitor_* v)) GeometryNode_AcceptVisitor,
		(void (*)(RenderVisitor_* v)) CameraNode_AcceptVisitor
	};

	scene_graph_node* Result = PushStruct(Arena, scene_graph_node);

	switch(i)
	{
//		case SCENEGRAPH_VISITOR_RENDER:
//		{
//			Result = PushStruct(Arena, scene_graph_visitor);
//			Result->instance = PushStruct(Arena, RenderVisitor_);
//			Result->interface = &iTable.VisitorRender;
//		}break;
		case InterfaceEnum::SCENEGRAPH_NODE_CAMERA:	
		{	
			Result->instance = PushStruct(Arena, CameraNode_);
			Result->interface = &iTable.NodeGeometry;
		}break;
		case InterfaceEnum::SCENEGRAPH_NODE_GEOMETRY:	
		{
			Result->instance = PushStruct(Arena, GeometryNode_);
			Result->interface = &iTable.NodeCamera;
		}break;
	}

    return Result;
};


void setup(memory_arena* Arena )
{
	scene_graph_node* Camera = Create_(Arena, InterfaceEnum::SCENEGRAPH_NODE_CAMERA);
	scene_graph_node* Geometry = Create_(Arena, InterfaceEnum::SCENEGRAPH_NODE_GEOMETRY);
	RenderVisitor_ R = {};

	AcceptVisitor(Camera, &R);
	AcceptVisitor(Geometry, &R);

}



typedef struct shape_interface {
    double (*Area)(void *instance);
    double (*Volume)(void *instance);
} ShapeInterface;

typedef struct {
    void *instance;
    const ShapeInterface *interface;
} Shape;

Shape *
shape_Create(void *instance, ShapeInterface *interface)
{
    Shape *shape = (Shape *) malloc(sizeof(Shape));
    shape->instance = instance;
    shape->interface = interface;
    return shape;
}

double
shape_Area(Shape *shape)
{
    return (shape->interface->Area)(shape->instance);
}

double
shape_Volume(Shape *shape)
{
    return (shape->interface->Volume)(shape->instance);
}


typedef struct {
    double x;
} Square;

double
square_Area(Square *square)
{
    return square->x * square->x;
}
double
square_Volume(Square *square)
{
    return square->x * square->x * square->x;
}

static ShapeInterface SquareAsShape = 
{
	(double (*)(void *)) square_Area,
	(double (*)(void *)) square_Volume,
};


Square *
square_Create(double sideLength)
{
    Square *square = (Square *) malloc(sizeof(Square));
    square->x = sideLength;
    return square;
}

typedef struct {
    double radius;
} Circle;

double
circle_Area(Circle *circle)
{
    return Pi32 * (circle->radius * circle->radius);
}

double
circle_Volume(Circle *circle)
{
    return (2/3.f)*Pi32 * (circle->radius * circle->radius* circle->radius);
}

static ShapeInterface CircleAsShape = 
{ 
	(double (*)(void *)) circle_Area,
	(double (*)(void *)) circle_Volume,
};

Circle *
circle_Create(double radius)
{
	Circle *circle = (Circle *) malloc(sizeof(Circle));
	circle->radius = radius;
    return circle;
}

void setup( memory_arena* Arena )
{
    // Create concrete types.
    Circle *circle = circle_Create(5.0);
    Square *square = square_Create(10.0);

    // Wire up the tables.
    Shape *circleShape = shape_Create(circle, &CircleAsShape);
    Shape *squareShape = shape_Create(square, &SquareAsShape);

    // Sanity check.
    if( circle_Area(circle) == shape_Area(circleShape))
    {
    	int yeeey = 1;
    }else{
    	int boo = 0;
    }

    if(square_Area(square) == shape_Area(squareShape))
    {
    	int yeeey = 1;
    }else{
    	int boo = 0;
    }

    // Sanity check.
    if( circle_Volume(circle) == shape_Volume(circleShape))
    {
    	int yeeey = 1;
    }else{
    	int boo = 0;
    }

    if(square_Volume(square) == shape_Volume(squareShape))
    {
    	int yeeey = 1;
    }else{
    	int boo = 0;
    }


    free(circle);
    free(circleShape);
    free(square);
    free(squareShape);
}

#endif


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

		CameraMovementCallback(game_controller_input* aController, CameraNode* aCamera)
		{
			Controller = aController;
			Camera = aCamera;
		};
		virtual ~CameraMovementCallback(){};

		void execute() override;

		game_controller_input* Controller=0;
		CameraNode* Camera;
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
		UpdateVisitor(){};
		virtual ~UpdateVisitor(){};

		void apply( CameraNode* n) override;
};


/*	
 *	Class: 		RenderVisitor
 *	Purpose: 	Traverses the scenegraph and renders it.
 */

class RenderVisitor : public BaseVisitor
{
	public:
		RenderVisitor( render_push_buffer* aBuffer )
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

		CameraNode(	real32 aAngleOfView, real32 aAspectRatio )
		{
			AngleOfView = aAngleOfView;
			AspectRatio = aAspectRatio;
			V = M4Identity();
			DeltaRot = M4Identity();
			DeltaPos = V3(0, 0, 0);
			SetPerspectiveProj( -0.1, -100 );
		}
		virtual ~CameraNode(){};

		void acceptVisitor( BaseVisitor* v ) override
		{
			v->apply(this);
		};

		void SetOrthoProj( real32 n, real32 f, real32 r, real32 l,  real32 t, real32 b )
		{
			real32 rlSum  = r+l;
			real32 rlDiff = r-l;

			real32 tbSum  = t+b;
			real32 tbDiff = t-b;

			real32 fnSum  = f+n;
			real32 fnDiff = f-n;

			P =  M4( 2/rlDiff,         0,        0, -rlSum/rlDiff, 
						    0,   2/tbDiff,       0, -tbSum/tbDiff, 
						    0,         0, 2/fnDiff, -fnSum/fnDiff,
						    0,         0,        0,             1);
		}

		void SetOrthoProj( real32 n, real32 f )
		{
			real32 scale = - Tan(AngleOfView * 0.5f * Pi32 / 180.f) * n;
			real32 r = AspectRatio * scale;
			real32 l = -r;
			real32 t = scale;
			real32 b = -t;
			SetOrthoProj( n, f, r, l,  t, b );
		}


		void SetPerspectiveProj( real32 n, real32 f, real32 r, real32 l, real32 t, real32 b )
		{
			real32 rlSum  = r+l;
			real32 rlDiff = r-l;

			real32 tbSum  = t+b;
			real32 tbDiff = t-b;

			real32 fnSum  = f+n;
			real32 fnDiff = f-n;

			real32 n2 = n*2;

			real32 fn2Prod = 2*f*n;

			P =  M4( n2/rlDiff,         0,  rlSum/rlDiff,               0, 
			                 0, n2/tbDiff,  tbSum/tbDiff,               0, 
			                 0,         0, -fnSum/fnDiff, -fn2Prod/fnDiff,
			                 0,         0,            -1,              -0);
		}

		void SetPerspectiveProj( real32 n, real32 f )
		{
			real32 scale = Tan(AngleOfView * 0.5f * Pi32 / 180.f) * n;
			real32 r = AspectRatio * scale;
			real32 l = -r;
			real32 t = scale;
			real32 b = -t;

			SetPerspectiveProj( n, f, r, l, t, b );
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
		void LookAt( v3 aFrom,  v3 aTo,  v3 aTmp = V3(0,1,0) )
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
			DeltaRot = GetRotationMatrix( DeltaAngle, &Axis );
		}

		void ApplyMovement()
		{
			m4 CamToWorld = RigidInverse(V);
			AssertIdentity(CamToWorld*V,0.001);

			v4 NewUpInCamCoord    = Column(DeltaRot,1);
			v4 NewUpInWorldCoord  = CamToWorld * NewUpInCamCoord;

			v4 NewAtDirInCamCoord  = Column(DeltaRot,2);
			v4 NewAtDirInWorldCord = CamToWorld * NewAtDirInCamCoord;
			v4 NewPosInWorldCoord  = Column(CamToWorld,3) + CamToWorld*V4( DeltaPos, 0 );

			v4 NewAtInWorldCoord   = NewPosInWorldCoord-NewAtDirInWorldCord;

			LookAt( V3(NewPosInWorldCoord), V3(NewAtInWorldCoord), V3(NewUpInWorldCoord) );
	
			DeltaRot = M4Identity();
			DeltaPos = V3( 0, 0, 0 );
		}

		real32 AngleOfView;
		real32 AspectRatio;
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
			m4 dR = GetRotationMatrix( Angle, &Axis );

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


void CameraMovementCallback::execute()
{
	if( Controller->IsAnalog )
	{
		real32 dr = 0.05; 
		real32 da = 0.05;
		if(Controller->Start.EndedDown)
		{
			v3 CamPos = V3(0,0,1);
			v3 CamAt =  V3(0,0,0);
			Camera->LookAt(CamPos,CamAt);
		}
		if(Controller->LeftStickLeft.EndedDown)
		{
			Camera->Translate( V3(-dr,0,0) );
		}
		if(Controller->LeftStickRight.EndedDown)
		{
			Camera->Translate( V3( dr,0,0) );
		}
		if(Controller->LeftStickUp.EndedDown)
		{
			Camera->Translate( V3(0,dr,0) );
		}
		if(Controller->LeftStickDown.EndedDown)
		{
			Camera->Translate( V3(0,-dr, 0) );
		}
		if(Controller->RightStickUp.EndedDown)
		{
			Camera->Rotate( da, V3(1,0,0) );
		}
		if(Controller->RightStickDown.EndedDown)
		{
			Camera->Rotate( da, V3(-1,0,0) );
		}		
		if(Controller->RightStickLeft.EndedDown)
		{
			Camera->Rotate( da, V3(0,1,0) );
		}
		if(Controller->RightStickRight.EndedDown)
		{
			Camera->Rotate( da, V3(0,-1, 0) );
		}
		if(Controller->RightTrigger.EndedDown)
		{
			Camera->Translate( V3(0, 0, -dr) );
		}
		if(Controller->LeftTrigger.EndedDown)
		{
			Camera->Translate( V3(0, 0, dr) );
		}
		if(Controller->A.EndedDown)
		{
			// at Z, top is Y, X is Right
			Camera->LookAt(V3(0,0,1),V3(0,0,0));
		}
		if(Controller->B.EndedDown)
		{
			// at X, top is Y, X is Left
			Camera->LookAt(V3(1,1,1),V3(0,0,0));			
		}
		if(Controller->X.EndedDown)
		{
			// at X, top is Y, X is Left
			Camera->LookAt(V3(1,0,0),V3(0,0,0));
		}
		if(Controller->Y.EndedDown)
		{
			// at Y, top is X is up, X is Left
			Camera->LookAt(V3(0,1,0),V3(0,0,0), V3(1,0,0));
		}
		if(Controller->RightShoulder.EndedDown)
		{
			Camera->SetOrthoProj( -1, 1 );
		}
		if(Controller->LeftShoulder.EndedDown)
		{
			Camera->SetPerspectiveProj( 0.1, 1000 );
		}

		Camera->ApplyMovement();
	}

};