

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
class BaseNode
{
	public:

		BaseNode(){};
		virtual ~BaseNode(){};

		virtual void update(){};
		virtual void acceptVisitor( BaseVisitor* ) = 0;
		virtual void connectCallback( BaseCallback* ){};

		BaseNode* Children = 0;
};

#define NodeApply( name ) void apply( name* aNode )

//class CameraNode;
//class GeometryNode;
//class TransformNode;
class RootNode;
class BitmapNode;

class BaseVisitor
{

	public:
		BaseVisitor(){};
		virtual ~BaseVisitor(){};

		// A method for traversing the scene graph
		virtual void traverse( BaseNode* aNode) final
		{
			if(aNode == 0){return;}

			// Inject itself into the node
			aNode->acceptVisitor( this );
			
			if( aNode->Children != 0 )
			{
				traverse( aNode->Children );
			}
		}

		// Each node has their own apply function
		virtual void apply( RootNode* aNode ){};
		virtual void apply( BitmapNode* aNode){};
};

/*	
 *	Class: 		RenderVisitor
 *	Purpose: 	Traverses the scenegraph and renders it.
 */

class RenderVisitor : public BaseVisitor
{
	public:
		RenderVisitor(  game_offscreen_buffer* aBuffer, v4 aObj )
		{
			mBuffer = aBuffer;
			mRasterProj=M4( 	mBuffer->Width/2.f,  0, 0, mBuffer->Width/2.f, 
							    0, mBuffer->Height/2.f, 0, mBuffer->Height/2.f, 
							    0, 0, 0, 0,
							    0, 0, 0, 1);
			mObject = aObj;


		};
		virtual ~RenderVisitor(){};

		void apply( RootNode* n) override
		{
			// Clear screen to black
			DrawRectangle(mBuffer, 0,0, (real32) mBuffer->Width,   (real32) mBuffer->Height, 0,0,0);
		};

		void apply( BitmapNode* n) override;

	private:
		v4 mObject;
		m4 mRasterProj;
		game_offscreen_buffer* mBuffer;
};

/*
 * Class:	RootNode
 * Purpose:	Root of renderTree and contains the draw method
 * Misc: 	Is always Root
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
 * Misc: 	Is always Root
 */
class CameraNode : public BaseNode
{
	public:

		CameraNode( real32, aXPos, real32 aYPos, real32 aZPos, real32 aNear, real32 aFar, real32 aLeft, real32 aRight, real32 aTop, real32 aBottom )
		{

			mCamera = M4( 1, 0, 0, aXPos, 
						  0, 1, 0, aYPos, 
						  0, 0, 1, aZPos,
						  0, 0, 0, 1);

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

		Matrix44f lookAt(const Vec3f& from, const Vec3f& to, const Vec3f& tmp = Vec3f(0, 1, 0))
		{
			Vec3f forward = normalize(from - to);
			Vec3f right = crossProduct(normalize(tmp), forward);
			Vec3f up = crossProduct(forward, right);
			
			Matrix44f camToWorld;
			
			camToWorld[0][0] = right.x;
			camToWorld[0][1] = right.y;
			camToWorld[0][2] = right.z;
			camToWorld[1][0] = up.x;
			camToWorld[1][1] = up.y;
			camToWorld[1][2] = up.z;
			camToWorld[2][0] = forward.x;
			camToWorld[2][1] = forward.y;
			camToWorld[2][2] = forward.z;
			
			camToWorld[3][0] = from.x;
			camToWorld[3][1] = from.y;
			camToWorld[3][2] = from.z;
		}

		void setCamPos( v3 aCameraPosition, v3 aLookDirection )
		{
			mCamera = M4( 1, 0, 0, aXPos, 
						  0, 1, 0, aYPos, 
						  0, 0, 1, aZPos,
						  0, 0, 0, 1);
		}

	private:

		v4 mPos;
		m4 mCamProj;
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


void RenderVisitor::apply( BitmapNode* n)
{
	m4 cam = AffineInverse( mCamera );
	v4 obj = cam * mObject;
	obj = mOrtoProj*obj;
	obj = mRasterProj*obj;
	BlitBMP( mBuffer, (real32) obj.X, (real32) obj.Y, n->BMP);
}