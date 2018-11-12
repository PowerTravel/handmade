

/*	
 *	Class: 		BaseCallback
 *	Purpose: 	BaseClass for all callback classes
 *	Misc:		All other callbacks inherit from this one
 */
class BaseCallback{

	public:
		virtual void execute() = 0;	

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

		virtual void update(){};
		virtual void acceptVisitor( BaseVisitor* ) = 0;
		virtual void connectCallback( BaseCallback* ){};

		BaseNode* FirstChild = 0;
		BaseNode* NextSibling = 0;

		void pushChild( BaseNode* aNode )
		{

			if( FirstChild == 0)
			{
				FirstChild = aNode;
				return;
			}

			BaseNode* Child = FirstChild;
			while( Child->NextSibling )
			{
				Child = Child->NextSibling;
			}

			Child->NextSibling = aNode;
		}
};

//class TransformNode;
class RootNode;
class CameraNode;
class BitmapNode; 	// leaf
class GeometryNode; // leaf
class TransformNode; // leaf

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
			mTransformMat = M4Identity();

		};
		virtual ~RenderVisitor(){};

		void apply( RootNode* n) override
		{
			// Clear screen to black
			DrawRectangle(mBuffer, 0,0, (real32) mBuffer->Width,   (real32) mBuffer->Height, 0,0,0);
		};

		void apply( CameraNode* n) override;

		void apply( BitmapNode* n) override;

		void apply( GeometryNode* n) override;

		void apply( TransformNode* n ) override;

	private:


		v4 RenderVisitor::worldToScreenCoordinates( v4 pointInWorldCoord )
		{
			pointInWorldCoord = mTransformMat * pointInWorldCoord;

			m4 worldToCam = AffineInverse( mCameraMat );

			v4 pointInCamCoord      = worldToCam * pointInWorldCoord;
			v4 pointInCamProjection = mCameraProjMat * pointInCamCoord;
			v4 pointInScreenCoord   = mRasterProj * pointInCamProjection;

			return pointInScreenCoord;
		}

		m4 mRasterProj;
		m4 mTransformMat;
		m4 mCameraMat;
		m4 mCameraProjMat;
		game_offscreen_buffer* mBuffer;
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
		GeometryNode(v3 p0, v3 p1, v3 p2)
		{
			mp0 = V4(p0.X, p0.Y, p0.Z, 1);
			mp1 = V4(p1.X, p1.Y, p1.Z, 1);
			mp2 = V4(p2.X, p2.Y, p2.Z, 1);
		};
		virtual ~GeometryNode(){};

		void acceptVisitor( BaseVisitor* v ) override
		{
			v->apply(this);
		};

		v4 mp0, mp1, mp2;
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
			dR = M4Identity();
			Pos =  M4Identity();
			dP = M4Identity();
		};

		virtual ~TransformNode(){};

		void Translate( v3 dp )
		{
			dP = M4(1,0,0,dp.X,
					0,1,0,dp.Y,
					0,0,1,dp.Z,
					0,0,0,1);
		}

		void Rotate( real32 angle, v3 axis )
		{
			dR = GetRotationMatrix( angle, axis );
		}

		void acceptVisitor( BaseVisitor* v ) override
		{
			v->apply(this);
		}

		m4 GetTransMat()
		{
			Pos = dP*Pos;			
			m4 tto = M4(1,0,0,-Pos.E[3],
						0,1,0,-Pos.E[7],
						0,0,1,-Pos.E[13],
						0,0,0,1);

			m4 bfo = M4(1,0,0, Pos.E[3],
						0,1,0, Pos.E[7],
						0,0,1, Pos.E[13],
						0,0,0,1);

			Pos = bfo * dR * tto;


			dR = M4Identity();
			dP = M4Identity();

			return Pos;
		}

		m4 Pos;
		m4 dP;
		m4 dR;
};

void RenderVisitor::apply( BitmapNode* n)
{
//	v4 obj = mCumulativeMat * mObject;
//	obj = mRasterProj*obj;
//	BlitBMP( mBuffer, (real32) obj.X, (real32) obj.Y, n->BMP);
}

void RenderVisitor::apply( GeometryNode* n)
{
	v4 p0 = worldToScreenCoordinates( n->mp0 );
	v4 p1 = worldToScreenCoordinates( n->mp1 );
	v4 p2 = worldToScreenCoordinates( n->mp2 );

	FillTriangle(mBuffer, V2(p0.X, p0.Y), V2(p1.X, p1.Y), V2(p2.X, p2.Y) );
}

void RenderVisitor::apply( CameraNode* n)
{
	mCameraMat = n->mCamToWorld;
	mCameraProjMat = n->mProj;
}


void RenderVisitor::apply( TransformNode* n )
{
	local_persist real32 t = 0;
	n->Rotate(t, V3(0,0,1) );
	n->Translate(V3(0,0,0));
	t += 0.05;
	mTransformMat = n->GetTransMat();

	if(t == 2*3.14){
		t -=2*3.14;
	}
}