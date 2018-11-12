

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
		CameraMovementCallback(){};
		virtual ~CameraMovementCallback(){};

		void execute();
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
 *	Class: 		RenderVisitor
 *	Purpose: 	Traverses the scenegraph and renders it.
 */

class RenderVisitor : public BaseVisitor
{
	public:
		RenderVisitor(  game_offscreen_buffer* aBuffer )
		{
			mBuffer = aBuffer;
			mRasterProj=M4( 	mBuffer->Width/2.f,  0, 0, mBuffer->Width/2.f, 
							    0, mBuffer->Height/2.f, 0, mBuffer->Height/2.f, 
							    0, 0, 0, 0,
							    0, 0, 0, 1);

			T = M4Identity();
			mCameraMat    = M4Identity();
			mCameraProjMat= M4Identity();

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
			mCameraMat    = M4Identity();
			mCameraProjMat= M4Identity();
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

		v2 RenderVisitor::Rasterize( v4 pointInCamCoord )
		{
			v4 pointInCamProjection = mCameraProjMat * pointInCamCoord;
			v4 pointInScreenCoord   = mRasterProj * pointInCamProjection;

			return V2(pointInScreenCoord.X, pointInScreenCoord.Y);
		}

		m4 mRasterProj;
		m4 mCameraMat;
		m4 mCameraProjMat;
		m4 T;
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

		CameraNode( ){};
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

			mProj = M4( 2/rlDiff,         0,        0, -rlSum/rlDiff, 
						       0,   2/tbDiff,       0, -tbSum/tbDiff, 
						       0,         0, 2/fnDiff, -fnSum/fnDiff,
						       0,         0,        0,             1);
		}

		void lookAt( v3 aFrom,  v3 aTo,  v3 aTmp = V3(0,1,0) )
		{
			v3 Forward = normalize(aFrom - aTo);
			v3 Right = cross(normalize(aTmp), Forward);
			v3 Up = cross(Forward, Right);
			
			mCamToWorld =  M4(
							 Right.X,   Right.Y,   Right.Z,   aFrom.X,
							 Up.X,      Up.Y,      Up.Z,      aFrom.Y,
							 Forward.X, Forward.Y, Forward.Z, aFrom.Z,
							 0,         0,         0,         1);
		}

		m4 mCamToWorld;
		m4 mProj;
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

		m4 T;
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

		GeometryNode(geometry Geometry)
		{
			#if 0
			v3 cm = V3(0,0,0);
			for(int32 i = 0; i < Geometry.nrVertex; ++i)
			{
				cm += Geometry.vertex[i];
			}

			cm = cm/Geometry.nrVertex;

			for(int32 i = 0; i < Geometry.nrVertex; ++i)
			{
				mp0 = V4(Geometry.vertex[i] - cm, 1);
			}
			#endif

			Object = Geometry;

		}
		virtual ~GeometryNode(){};

		void acceptVisitor( BaseVisitor* v ) override
		{
			v->apply(this);
		};

		geometry Object;
};

void RenderVisitor::apply( BitmapNode* n)
{
//	v4 obj = mCumulativeMat * mObject;
//	obj = mRasterProj*obj;
//	BlitBMP( mBuffer, (real32) obj.X, (real32) obj.Y, n->BMP);
}

void RenderVisitor::apply( GeometryNode* n)
{

	m4 worldToCam = AffineInverse( mCameraMat );

	for(int32 i = 0; i < n->Object.nrTriangles; ++i)
	{

		real32* TriangleNormal = &n->Object.triangleNormal[ 3*i ];
		v4 FN = worldToCam * T * V4( TriangleNormal[0],TriangleNormal[1],TriangleNormal[2], 0 );
		if( FN *V4(0,0,1,0) <= 0 )
		{
			continue;
		}
		
		int32* Triangle = &n->Object.triangleIdx[3*i];

		real32* Vertex1 = &n->Object.vertex[ 3*Triangle[0] ];
		real32* Vertex2 = &n->Object.vertex[ 3*Triangle[1] ];
		real32* Vertex3 = &n->Object.vertex[ 3*Triangle[2] ];

		v4 P0 = worldToCam * T * V4( Vertex1[0],Vertex1[1],Vertex1[2], 1 );
		v4 P1 = worldToCam * T * V4( Vertex2[0],Vertex2[1],Vertex2[2], 1 );
		v4 P2 = worldToCam * T * V4( Vertex3[0],Vertex3[1],Vertex3[2], 1 );

		v2 p0 = Rasterize( P0 );
		v2 p1 = Rasterize( P1 );
		v2 p2 = Rasterize( P2 );
		
		FillTriangle(mBuffer, p0, p1, p2 );
	}
	
	v2 Origo = Rasterize( V4(0,0,0,1) );
	DrawCircle( mBuffer, Origo.X, Origo.Y , 10);
	popTrace();
}

void RenderVisitor::apply( CameraNode* n)
{
	mCameraMat = n->mCamToWorld;
	mCameraProjMat = n->mProj;
}


void RenderVisitor::apply( TransformNode* n )
{
	pushTrace( n->T, n->nrChildren );
}