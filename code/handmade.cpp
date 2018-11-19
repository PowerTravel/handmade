
#include <stdio.h>
#include "handmade.h"
#include "handmade_render.cpp"
#include "scene_graph.cpp"

internal void
GameOutputSound(game_sound_output_buffer* SoundBuffer, int ToneHz)
{

	local_persist real32 tSine = 0.f;

	int16 ToneVolume = 2000;
	int SamplesPerPeriod = SoundBuffer->SamplesPerSecond/ToneHz;

	real32 twopi = (real32) (2.0*Pi32);

	int16 *SampleOut = SoundBuffer->Samples;
	for(int SampleIndex = 0; 
		SampleIndex < SoundBuffer->SampleCount; 
		++SampleIndex)
	{

#if 0
		real32 SineValue = Sin(tSine);
		int16 SampleValue = (int16)(SineValue * ToneVolume);

		tSine += twopi/(real32)SamplesPerPeriod;	
		if(tSine > twopi)
		{
			tSine -= twopi;
		}
#else	
		int16 SampleValue = 0;
#endif	
		*SampleOut++ = SampleValue; // Left Speker
		*SampleOut++ = SampleValue; // Rright speaker
	}
}

#if 0
bool32 closeEnough(real32 a, real32 b) 
{

	real32 diff = a - b;

	if(diff < 0)
	{
		diff = -diff;
	}

	return (diff < .001);

}

real32 betterGuess(real32 x, real32 g) 
{
   return ( (g + x/g) / 2.f );
}

real32 test(real32 x, real32 g) 
{
	if( closeEnough(x/g, g) )
	{
		return g;
	}else{
		return test(x, betterGuess(x, g) );
	}
}
#endif

#if 0
int main( void )
{
	//MainWindow mainWindow = MainWindow();
	MainWindow::getInstance().init(1024,768);
	//mainWindow.init(1024,768);

	group_ptr grp = build_graph();
	RenderVisitor r = RenderVisitor();	
	UpdateVisitor u = UpdateVisitor();	

	//while(mainWindow.isRunning()){
	while(MainWindow::getInstance().isRunning()){
		MainWindow::getInstance().clear();
		MainWindow::getInstance().getInput();
		MainWindow::getInstance().update();	

	//	std::cout << "NEW FRAME" << std::endl;
		u.traverse(grp.get());
		r.traverse(grp.get());

		// Swap buffers
		MainWindow::getInstance().swap();
	} 

//	p.printToFile("bajsloek.m");

	MainWindow::getInstance().destroy();

	return 0;
}

group_ptr build_graph()
{
	// Create Nodes
	// Root
	group_ptr grp = group_ptr(new Group());

	// Camera
	camera_ptr cam = camera_ptr(new Camera());
	cam->connectCallback(callback_ptr(new CameraMovementCallback(cam)));

	// Transform 1 and 2
	transform_ptr trns1 = transform_ptr(new Transform());
	trns1->translate(vec3(0.5,0,0));
	trns1->scale(vec3(0.5,0.5,0.5));
	trns1->rotate(90,vec3(0,0,1));

	transform_ptr trns2 = transform_ptr(new Transform());
	trns2->translate(vec3(-0.5,0,0));
	trns2->scale(vec3(0.5,0.5,0.5));

	// Geometry
	geometry_vec gvec = Geometry::loadFile("../models/sphere.obj");

	// Link the tree

	grp->addChild(cam);
	cam->addChild(trns1);
	cam->addChild(trns2);

	for(int i = 0; i<gvec.size(); i++){
		trns1->addChild(gvec[i]);
		trns2->addChild(gvec[i]);
	}

	return grp;
}

#endif


v4 ParseNumbers(char* String)
{
	char* Start = str::FindFirstNotOf( " \t", String );
	char WordBuffer[OBJ_MAX_WORD_LENGTH];
	int32 CoordinateIdx = 0;
	v4 Result = V4(0,0,0,1);
	while( Start )
	{
		Assert(CoordinateIdx < 4);

		char* End = str::FindFirstOf( " \t", Start);

		size_t WordLength = ( End ) ? (End - Start) : str::StringLength(Start);

		Assert(WordLength < OBJ_MAX_WORD_LENGTH);

		Copy( WordLength, Start, WordBuffer );
		WordBuffer[WordLength] = '\0';

		Result.E[CoordinateIdx++] =(real32) str::StringToReal64(WordBuffer);

		Start = (End) ? str::FindFirstNotOf(" \t", End) : End;
	}

	return Result;
}

obj_geometry DEBUGReadOBJ(thread_context* Thread, game_state* aGameState, 
			   debug_platform_read_entire_file* ReadEntireFile,
			   debug_platfrom_free_file_memory* FreeEntireFile,
			   char* FileName)
{
	obj_geometry Result = {};

	debug_read_file_result ReadResult = ReadEntireFile(Thread, FileName);

	if( ReadResult.ContentSize )
	{
		int32 nrVertices = 0;
		int32 nrVertexNormals = 0;
		int32 nrTexturePoints = 0;
		int32 nrFaces = 0;

		char* ScanPtr = ( char* ) ReadResult.Contents;
		char* FileEnd =  ( char* ) ReadResult.Contents + ReadResult.ContentSize;

		while( ScanPtr < FileEnd )
		{
			char* ThisLine = ScanPtr;
			ScanPtr = str::FindFirstOf('\n', ThisLine);
			ScanPtr = (ScanPtr) ? (ScanPtr + 1) : FileEnd;

			size_t Length = ScanPtr - ThisLine;

			Assert(Length < OBJ_MAX_LINE_LENGTH);

			if(Length < 2)
			{
				continue;
			}

			char LineCopy[OBJ_MAX_LINE_LENGTH];
			Copy( Length, ThisLine, LineCopy );
			LineCopy[Length] = '\0';


			// Jump over Comment lines
			if( str::BeginsWith( 1,"#",Length, LineCopy) || 
			 	str::BeginsWith( 2,"\r\n",Length, LineCopy) ||
			 	str::BeginsWith( 1,"\n",Length, LineCopy)  )
			{
				continue;
			}

			// Vertices: v x y z [w=1]
			else if( str::BeginsWith(2,"v ", Length, LineCopy) )
			{
				++nrVertices;
			}

			// Vertex Normals: vn i j k
			else if( str::BeginsWith(3,"vn ", Length, LineCopy) )
			{
				++nrVertexNormals;
			}

			// Vertex Textures: vt u v [w=0]
			else if( str::BeginsWith(3,"vt ", Length, LineCopy) )
			{
				++nrTexturePoints;
			}

			// Faces: f v1[/vt1][/vn1] v2[/vt2][/vn2] v3[/vt3][/vn3] ...
			else if( str::BeginsWith(2,"f ", Length, LineCopy) )
			{
				++nrFaces;
			}else if(str::BeginsWith(7,"mtllib ", Length, LineCopy)  ){
				// Unimplemented
			}else if(str::BeginsWith(2,"g ", Length, LineCopy)  ){
				// Unimplemented
			}else if(str::BeginsWith(7,"usemtl ", Length, LineCopy)  ){
				// Unimplemented
			}else if(str::BeginsWith(2,"s ", Length, LineCopy)  ){
				// Unimplemented
			}
			else if (str::BeginsWith(2, "o ", Length, LineCopy)) {
				// Unimplemented
			}
			else if (str::BeginsWith(2, "l ", Length, LineCopy)) {
				// Unimplemented
			}
			else{
				Assert(0);
			
			}
		}

		memory_arena* Arena = &aGameState->AssetArena;
		
		Result.nv = nrVertices;
		Result.v = (v4*) PushArray(Arena, nrVertices ,v4);
		v4* vertices  = Result.v;

		Result.nvn = nrVertexNormals;
		Result.vn = (v4*) PushArray(Arena, nrVertexNormals ,v4);
		v4* vertexNormals = Result.vn;

		Result.nvt = nrTexturePoints;
		Result.vt = (v3*) PushArray(Arena, nrTexturePoints ,v3);
		v3* texturePoints = Result.vt;

		Result.nf = nrFaces;
		Result.f= (face*) PushArray(Arena, nrFaces, face);
		face* faces = Result.f;

		int32 verticeIdx 	  = 0;
		int32 vertexNormalIdx = 0;
		int32 texturePointIdx = 0;
		int32 faceIdx = 0;

		ScanPtr = ( char* ) ReadResult.Contents;
		while( ScanPtr < FileEnd )
		{
			char* ThisLine = ScanPtr;
			ScanPtr = str::FindFirstOf('\n', ThisLine);
			ScanPtr = (ScanPtr) ? (ScanPtr + 1) : FileEnd;


			char* TrimEnd = ScanPtr-1;
			while( (TrimEnd > ThisLine) && ( (*TrimEnd == '\n') || (*TrimEnd == '\r') ) )
			{
				--TrimEnd;
			}
			++TrimEnd;

			char* TrimStart = ThisLine;
			while( (TrimStart < TrimEnd) && ((*TrimStart == ' ') || (*TrimStart == '\t')) )
			{
				++TrimStart;
			}

			ThisLine = TrimStart;
			size_t Length = TrimEnd - ThisLine;

			Assert(Length < OBJ_MAX_LINE_LENGTH);

			if(Length < 2)
			{
				continue;
			}
			
			char LineBuffer[OBJ_MAX_LINE_LENGTH];
			
			Copy( Length, ThisLine, LineBuffer );
			LineBuffer[Length] = '\0';

			char WordBuffer[OBJ_MAX_WORD_LENGTH];

			// Jump over Comment lines
			if( str::BeginsWith( 1,"#",Length, LineBuffer) )
			{
				continue;
			}

			// Vertices: v x y z [w=1]
			else if( str::BeginsWith(2,"v ", Length, LineBuffer) )
			{
				Assert(verticeIdx < nrVertices);
				vertices[verticeIdx++] = ParseNumbers(LineBuffer+2);
			}
			
			// Vertex Normals: vn i j k
			else if( str::BeginsWith(3,"vn ", Length, LineBuffer) )
			{
				Assert(vertexNormalIdx<=nrVertexNormals);

				vertexNormals[ vertexNormalIdx++ ] = ParseNumbers(LineBuffer+3);
				
			}

			// Vertex Textures: vt u v [w=0]
			else if( str::BeginsWith(3,"vt ", Length, LineBuffer) )
			{
				Assert( texturePointIdx <=nrTexturePoints );
				texturePoints[ texturePointIdx++ ] = V3( ParseNumbers(LineBuffer+3) );
			}

			// Faces: f v1[/vt1][/vn1] v2[/vt2][/vn2] v3[/vt3][/vn3] ...
			else if( str::BeginsWith(2,"f ", Length, LineBuffer) )
			{
				char* Start = str::FindFirstNotOf( " \t", LineBuffer+2 );

				
				face* f = &faces[faceIdx++];
				f->nv = str::GetWordCount(Start);
				Assert( f->nv >= 3);
				Result.nt += f->nv - 2;

				int32 VertIdx = 0;

				while( Start )
				{
					char* End = str::FindFirstOf( " \t", Start);

					size_t WordLength = ( End ) ? (End - Start) : str::StringLength(Start);

					Assert(WordLength < OBJ_MAX_WORD_LENGTH);

					Copy( WordLength, Start, WordBuffer );
					WordBuffer[WordLength] = '\0';


					char* StartNr = WordBuffer;
					char* EndNr = 0;
					int32 i =0;
					while( StartNr )
					{
						EndNr 	= str::FindFirstOf("/", StartNr);
						if( EndNr )
					 	{
							*EndNr++ = '\0';
						}

						int32 nr = (int32) str::StringToReal64(StartNr)-1;
						Assert(nr>=0);
						Assert(VertIdx < f->nv);
						switch(i)
						{
							case 0:
							{	
								if( !f->vi )
								{
									f->vi = (int32*) PushArray(Arena, f->nv, int); 
								}

								f->vi[VertIdx] = nr;
							}break;

							case 1:
							{
								if( !f->ni )
								{
									f->ni  = (int32*) PushArray(Arena, f->nv, int); 
								}
								f->ni[VertIdx] = nr;
							}break;

							case 2:
							{
								if( !f->ti )
								{
									f->ti = (int32*) PushArray(Arena, f->nv, int); 
								}

								f->ti[VertIdx] = nr;
							}break;
						}

						++i;

						StartNr = str::FindFirstNotOf("/", EndNr);
						
					}

					++VertIdx;

					Start = (End) ? str::FindFirstNotOf(" \t", End) : End;
				}

			}else if(str::BeginsWith(7,"mtllib ", Length, LineBuffer)  ){
				// Unimplemented
			}else if(str::BeginsWith(2,"g ", Length, LineBuffer)  ){
				// Unimplemented
			}else if(str::BeginsWith(7,"usemtl ", Length, LineBuffer)  ){
				// Unimplemented
			}else if(str::BeginsWith(2,"s ", Length, LineBuffer)  ){
				// Unimplemented
			}
			
	
		}

		FreeEntireFile(Thread, ReadResult.Contents);

		
		v4 cm = V4(0,0,0,0);
		for(int32 i = 0; i<Result.nv; ++i)
		{
			cm += Result.v[i];
		}
		cm = cm/(real32)Result.nv;
		cm.W = 0;

		// Center the object around origin
		v4 MaxAxis = V4(0,0,0,0);
		real32 MaxDistance = 0;
		for(int32 i = 0; i<Result.nv; ++i)
		{
			Result.v[i] = Result.v[i]-cm;
			real32 distance = norm( V3(Result.v[i]) ); 
			if( distance > MaxDistance )
			{
				MaxAxis = Result.v[i];
				MaxDistance = distance;
			}
		}
		if(MaxDistance > 1)
		{
			MaxDistance = 1/MaxDistance;
		}
		// Scale object to the unit cube
		for(int32 i = 0; i<Result.nv; ++i)
		{
			Result.v[i] = Result.v[i]*MaxDistance; 
			Result.v[i].W = 1;
		}


		Result.t = PushArray(Arena, Result.nt, triangle);
		int32 TriangleIdx = 0;

		for(int32 i = 0; i<Result.nf; ++i)
		{
			face* f = &Result.f[i];
			for(int32 j = 0; j < f->nv-2; ++j)
			{

				triangle* t = &Result.t[TriangleIdx++];

				t->vi[0] = f->vi[0];
				t->vi[1] = f->vi[1+j];
				t->vi[2] = f->vi[2+j];

				v4 v0 = Result.v[f->vi[0]];
				v4 v1 = Result.v[f->vi[1]];
				v4 v2 = Result.v[f->vi[2]];


				v3 r1 = V3(v1-v0);
				v3 r2 = V3(v2-v0);
				v4 triangleNormal = V4( cross( r1 , r2 ),0);
				t->n = normalize( triangleNormal );
			}
		}


	}

	return Result;
}

internal loaded_bitmap
DEBUGReadBMP( thread_context* Thread, game_state* aGameState, 
			   debug_platform_read_entire_file* ReadEntireFile,
			   debug_platfrom_free_file_memory* FreeEntireFile,
			   char* FileName)
{
	// Note(Jakob). BMP is stored in little Endian, 
	// Little endian means that the least significant digit is stored first
	// So if we read a 32 bit int in 8 bit cunks it would come out as this:
	// 32bit: 0xAABBCCDD ->  
	// 8bit chunks:
	// 0xDD, 0xCC, 0xBB, 0xAA

	// Todo(Jakob): Add hotspots to bitmaps. A hotspot is the alignpoint in pixels. 
	//				For a mouse pointer sprite this would for example be on the tip of
	//				the pointer. Its used to poroperly display the bitmap.

	loaded_bitmap Result = {};
	debug_read_file_result ReadResult = ReadEntireFile(Thread, FileName);

	if (ReadResult.ContentSize != 0)
	{
		void* imageContentsTmp = ReadResult.Contents;
		memory_arena* Arena = &aGameState->AssetArena;
		//ReadResult.Contents = PushCopy(Arena, ReadResult.ContentSize, imageContentsTmp);


		bmp_header* Header = (bmp_header*)ReadResult.Contents;//ReadResult.Contents;

		Assert(Header->BitsPerPixel == 32);
		Assert(Header->Compression == 3);

		Result.Pixels = PushArray(Arena, Header->Width*Header->Height, uint32);
		Result.Width = Header->Width;
		Result.Height = Header->Height;

		uint32* Target = (uint32*)Result.Pixels;
		uint32* Source = (uint32*)(((uint8*)ReadResult.Contents) + Header->FileSize - 4);

		bit_scan_result RedBitShift =   FindLeastSignificantSetBit(Header->RedMask);
		bit_scan_result GreenBitShift = FindLeastSignificantSetBit(Header->GreenMask);
		bit_scan_result BlueBitShift =  FindLeastSignificantSetBit(Header->BlueMask);
		bit_scan_result AlphaBitShift = FindLeastSignificantSetBit(Header->AlphaMask);

		Assert(RedBitShift.Found);
		Assert(GreenBitShift.Found);
		Assert(BlueBitShift.Found);

		for (int Y = 0; Y < Result.Height; ++Y)
		{
			for (int X = 0; X < Result.Width; ++X)
			{
				uint32 Pixel = *Source--;
	
				bit_scan_result BitShift = {};


				uint32 Red   = Pixel & Header->RedMask;     
				uint8 R = (uint8) (Red >> RedBitShift.Index);

				uint32 Green = Pixel & Header->GreenMask;
				uint8 G = (uint8) (Green >> GreenBitShift.Index);
			
				uint32 Blue  = Pixel & Header->BlueMask;
				uint8 B = (uint8) (Blue >> BlueBitShift.Index);
				
				uint8 A = 0xff;
				if (AlphaBitShift.Found)
				{
					uint32 Alpha = Pixel & Header->AlphaMask;
					A = (uint8)(Alpha >> AlphaBitShift.Index);
				}
				*Target++ = (A << 24) | (R << 16) | (G << 8) | (B << 0);

			}
		}		

		FreeEntireFile(Thread, imageContentsTmp);
	}

	return Result;
}

void initiateGame(thread_context* Thread, game_memory* Memory, game_input* Input)
{
	if( ! Memory->GameState )
	{
		Memory->GameState = BootstrapPushStruct(game_state, AssetArena);
		//Memory->GameState = BootstrapPushStruct(game_state, TransientArena);

		//geometry* geom = PushStruct( &GameState->AssetArena, geometry );
		//GameState->Geometry = geom;

		Memory->GameState->testBMP = DEBUGReadBMP(Thread, Memory->GameState, 
											Memory->PlatformAPI.DEBUGPlatformReadEntireFile,
											Memory->PlatformAPI.DEBUGPlatformFreeFileMemory,
											"..\\handmade\\data\\test\\test_hero_front_head.bmp");
		Memory->GameState->testOBJ = DEBUGReadOBJ(Thread, Memory->GameState, 
											Memory->PlatformAPI.DEBUGPlatformReadEntireFile,
											Memory->PlatformAPI.DEBUGPlatformFreeFileMemory,
					//						"..\\handmade\\data\\geometry\\Cube_obj.obj");
//											"..\\handmade\\data\\geometry\\cupcake.obj");
//											"..\\handmade\\data\\geometry\\teapot.obj");					
											"..\\handmade\\data\\geometry\\cube.obj");											

		for(int32 ControllerIndex = 0; 
			ControllerIndex < ArrayCount(Input->Controllers); 
			++ControllerIndex)
		{
			game_controller_input* Controller = GetController(Input,ControllerIndex);
			Controller->IsAnalog = true;
		}

	}
}

/* 
	Note: 
	extern "C" prevents the C++ compiler from renaming the functions which it does for function-overloading reasons (among other things) by forcing it to use C conventions which does not support overloading. Also called 'name mangling' or 'name decoration'. The actual function names are visible in the outputted .map file in the build directory
*/
// Signature is
//void game_update_and_render (thread_context* Thread, 
//							  game_memory* Memory, 
//							 game_offscreen_buffer* Buffer, 
//							  game_input* Input )

platform_api Platform;

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
	local_persist real32 t = 0;

	Platform = Memory->PlatformAPI;

	initiateGame( Thread,  Memory, Input );

	// Note: Controller 0 is keyboard. Controller 1 through 4 is Gamepads
 	Assert( (&Input->Controllers[0].Terminator - &Input->Controllers[0].Button[0]) == 
			 (ArrayCount(Input->Controllers[0].Button)) );
		

	CameraMovementCallback CameraMovement = CameraMovementCallback( GetController( Input, 1 ) );


	RootNode 	 Root = RootNode();
	CameraNode   Camera  = CameraNode( V3(0,0,0), V3(0,0,1), 1, -1, (real32)  Buffer->Width  / (Buffer->Height), 
																    (real32) -Buffer->Width  / (Buffer->Height), 
																    (real32)  Buffer->Height / (Buffer->Height), 
																    (real32) -Buffer->Height / (Buffer->Height));
	Camera.connectCallback( &CameraMovement );

	TransformNode Rotation;
	Rotation.Rotate(t/10, V3(0,1,0) );
//	Rotation.Rotate(t/50, V3(1,0,0) );
//	Rotation.Translate( V3( Sin(t/5), Cos(t/5),0) );

	TransformNode Rotation2;
	Rotation2.Rotate( t/10+Pi32, V3(0, 1,0) );
	Rotation2.Rotate( Pi32, V3(1, 0,0) );


	GeometryNode Cube = GeometryNode( Memory->GameState->testOBJ );

	Root.pushChild( &Camera );
	Camera.pushChild( &Rotation );
	Rotation.pushChild( &Cube );


	UpdateVisitor uv = UpdateVisitor();
	RenderVisitor rn = RenderVisitor( Buffer  );	


	uv.traverse( &Root );
	rn.traverse( &Root );

	t += Pi32/60;
	if(t >= 200*Pi32)
	{
		t -=200*Pi32;
	}

}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
	GameOutputSound(SoundBuffer, 400);
}
