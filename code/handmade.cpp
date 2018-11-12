
#include <stdio.h>
#include "handmade.h"
#include "handmade_render.cpp"
#include "render_tree.cpp"

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

void handleInput(game_input* Input)
{
	Assert( (&Input->Controllers[0].Terminator - &Input->Controllers[0].Button[0]) == 
			 (ArrayCount(Input->Controllers[0].Button)) );
		// Note: Controller 0 is keyboard. Controller 1 through 4 is Gamepads
	for(int32 ControllerIndex = 0; 
		ControllerIndex < ArrayCount(Input->Controllers); 
		++ControllerIndex)
	{
		
		game_controller_input* Controller = GetController( Input, ControllerIndex );

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

		}else{
			
			if(Input->MouseButton[0].EndedDown)
			{

			}

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
	}

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
		ReadResult.Contents = PushCopy(Arena, ReadResult.ContentSize, imageContentsTmp);
		FreeEntireFile(Thread, imageContentsTmp);

		bmp_header* Header = (bmp_header*)ReadResult.Contents;

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
	}

	return Result;
}

void initiateGame(thread_context* Thread, game_memory* Memory, game_input* Input)
{
	if( ! Memory->GameState )
	{
		Memory->GameState = BootstrapPushStruct(game_state, AssetArena);

		//geometry* geom = PushStruct( &GameState->AssetArena, geometry );
		//GameState->Geometry = geom;

		Memory->GameState->testBMP = DEBUGReadBMP(Thread, Memory->GameState, 
											Memory->PlatformAPI.DEBUGPlatformReadEntireFile,
											Memory->PlatformAPI.DEBUGPlatformFreeFileMemory,
											"..\\handmade\\data\\test\\test_hero_front_head.bmp");

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
	Platform = Memory->PlatformAPI;

	initiateGame( Thread,  Memory, Input );
 	handleInput( Input );

	//v4 object = V4( (real32)  Input->MouseX * 15/ Buffer->Width, (real32) Input->MouseY * 15 / Buffer->Height, 0,1 );

	RootNode 	 Root = RootNode();
	CameraNode   Camera  = CameraNode( V3(0,0,0), V3(0,0,1), 1, -1, (real32)  Buffer->Width  / (Buffer->Height), 
																   (real32) -Buffer->Width  / (Buffer->Height), 
																   (real32)  Buffer->Height / (Buffer->Height), 
																   (real32) -Buffer->Height / (Buffer->Height));
	
	TransformNode Trans = TransformNode();

	real32 side = 1;
	real32 height = side * sqrtf(3)/2;
	GeometryNode EquilateralTriangle = GeometryNode( V3( -side/2, -height/2.f, 0), V3(0 , height/2.f ,0),    V3(side/2,-height/2,0) );

	BitmapNode   BitMap = BitmapNode(Memory->GameState->testBMP);

	Root.pushChild( &Camera );

	Camera.pushChild( &Trans );
	Trans.pushChild(&EquilateralTriangle);
//	Camera.pushChild( &Geom1 );
//
//	Camera.pushChild( &Geom2 );
//	Camera.pushChild( &Geom3 );

	RenderVisitor rn = RenderVisitor( Buffer );	

	rn.traverse( &Root );
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
	GameOutputSound(SoundBuffer, 400);
}
