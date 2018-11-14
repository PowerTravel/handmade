

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

				if(Controller->LeftStickLeft.EndedDown)
				{
				}
				if(Controller->LeftStickRight.EndedDown)
				{
				}
				if(Controller->LeftStickUp.EndedDown)
				{
				}
				if(Controller->LeftStickDown.EndedDown)
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
		RenderVisitor(  game_offscreen_buffer* aBuffer )
		{
			mBuffer = aBuffer;
			R =M4( 	mBuffer->Width/2.f,  0, 0, mBuffer->Width/2.f, 
				    0, mBuffer->Height/2.f, 0, mBuffer->Height/2.f, 
				    0, 0, 0, 0,
				    0, 0, 0, 1);

			T = M4Identity();
			V = M4Identity();
			P = M4Identity();
		};
		virtual ~RenderVisitor()
		{
		};

		void apply( RootNode* n) override
		{
			// Clear screen to black
			DrawRectangle(mBuffer, 0,0, (real32) mBuffer->Width,   (real32) mBuffer->Height, 1,1,1);
			DrawRectangle(mBuffer, 1,1, (real32) mBuffer->Width-2,   (real32) mBuffer->Height-2, 0.3,0.3,0.3);

			T = M4Identity();
			V = M4Identity();
			P = M4Identity();
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

		void popTrace()
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

		m4 R; // Rasterization Matrix
		m4 V; // View Matrix
		m4 P; // Projection Matrix
		m4 T; // ModelMatrix
		game_offscreen_buffer* mBuffer;


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
			lookAt( aFrom,  aTo );

			setOrthoProj( aNear, aFar, aLeft, aRight, aTop, aBottom );
		};

		virtual ~CameraNode(){};

		void acceptVisitor( BaseVisitor* v ) override
		{
			v->apply(this);
		};

		void setOrthoProj( real32 aNear, real32 aFar, real32 aLeft, real32 aRight, real32 aTop, real32 aBottom )
		{
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

		void lookAt( v3 aFrom,  v3 aTo,  v3 aTmp = V3(0,1,0) )
		{
			v3 Forward = normalize(aFrom - aTo);
			v3 Right = cross(normalize(aTmp), Forward);
			v3 Up = cross(Forward, Right);
			
			CamToWorld =  M4(
							 Right.X,   Right.Y,   Right.Z,   aFrom.X,
							 Up.X,      Up.Y,      Up.Z,      aFrom.Y,
							 Forward.X, Forward.Y, Forward.Z, aFrom.Z,
							 0,         0,         0,         1);

			V = AffineInverse( CamToWorld );
		}

		m4 CamToWorld;
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

	m4 ModelView = V * T;
	for(int32 i = 0; i < O.nt; ++i)
	{
		triangle& t = O.t[i];

		v4 fn = ModelView*t.n;
		if( fn*V4(0,0,1,0) < 0 )
		{
			continue;
		}

		v4 AmbientProduct  = V4(0.3,0,0,1);
		v4 DiffuseProduct  = V4(0,0.8,0,1);
		v4 SpecularProduct = V4(0,0,0.8,1);

		//RenderTriangle( mBuffer, V1,V2,V3, fn, C1, C2, C3, T, V, P, R );

		v4 LightPosition = V4(0,0,-2,1);
		RenderTriangle( mBuffer, O.v[ t.vi[0] ],O.v[ t.vi[1] ],O.v[ t.vi[2] ], t.n, LightPosition, AmbientProduct, DiffuseProduct, SpecularProduct, T, V, P, R );
			
	}
	
	v2 Origo = V2( R*P*ModelView*V4(0,0,0,1) );
	DrawCircle( mBuffer, Origo.X, Origo.Y , 10);
	popTrace();
}

void RenderVisitor::apply( CameraNode* n)
{
	V = n->V;
	P = n->P;
}


void UpdateVisitor::apply( CameraNode* n)
{
	n->update();
};


void RenderVisitor::apply( TransformNode* n )
{
	pushTrace( n->T, n->nrChildren );
}