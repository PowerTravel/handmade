
struct geometry
{
	int32 nrVertex;
	int32 nrTriangles;
	real32* vertex;
	int32*  triangleIdx;
	real32*  triangleNormal;
};

geometry getBox()
{
	real32 side = 1;
	local_persist real32 vertex[24] =
	{
		-side/2,-side/2, -side/2,
		 side/2,-side/2, -side/2,
		 side/2,-side/2,  side/2,
		-side/2,-side/2,  side/2,
		-side/2, side/2,  side/2,
		 side/2, side/2,  side/2,
		 side/2, side/2, -side/2,
		-side/2, side/2, -side/2	
	};

	local_persist int32 triangleIdx[36] =
	{
		1, 6, 5,
		1, 5, 2,
		0, 3, 4,
		0, 4, 7,
		5, 6, 7,
		5, 7, 4,
		0, 1, 2,
		0, 2, 3,
		2, 5, 4,
		2, 4, 3,
		0, 7, 6,
		0, 6, 1
	};


	local_persist real32 triangleNormal[36] = 
	{
		 1, 0, 0,
		 1, 0, 0,
		-1, 0, 0,
		-1, 0, 0,
		 0, 1, 0,
		 0, 1, 0,
		 0,-1, 0,
		 0,-1, 0,
		 0, 0, 1,
		 0, 0, 1,
		 0, 0,-1,
		 0, 0,-1
	};

 	geometry Result = {};
	Result.nrVertex = 8;
	Result.nrTriangles = 12;
	Result.vertex = vertex;
	Result.triangleIdx = triangleIdx;
	Result.triangleNormal = triangleNormal;

	return Result;
};
