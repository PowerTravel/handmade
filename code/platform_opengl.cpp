
#include "render_push_buffer.h"
#include "affine_transformations.h"
#include "string.h"

#define GL_FRAMEBUFFER_SRGB               0x8DB9
#define GL_SRGB8_ALPHA8                   0x8C43
#define GL_SHADING_LANGUAGE_VERSION 	  0x8B8C

// Windows Specific defines. WGL = WindowsOpenGL
#define WGL_CONTEXT_MAJOR_VERSION_ARB             0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB             0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB               0x2093
#define WGL_CONTEXT_FLAGS_ARB                     0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB              0x9126
#define WGL_CONTEXT_DEBUG_BIT_ARB                 0x0001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB    0x0002
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB          0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002

#define ERROR_INVALID_VERSION_ARB               0x2095
#define ERROR_INVALID_PROFILE_ARB               0x2096

struct opengl_info
{
	b32 ModernContext;
	char* Vendor;
	char* Renderer;
	char* Version;
	char* ShadingLanguageVersion;
	char* Extensions;
	b32 EXT_texture_sRGB_decode;
	b32 GL_ARB_framebuffer_sRGB;
	b32 GL_ARB_vertex_shader;
};

global_variable u32 TextureBindCount = 0;

internal GLuint LoadShader( u32 CodeLength, char* SourceCode, GLenum ShaderType )
{
	//GLuint ShaderHandle = glCreateShader(ShaderType);
	// GLuint FragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	char* LineStart = SourceCode;
	char* LineEnd = str::FindFirstOf("\r\n", LineStart);

	while(LineStart)
	{
 		umm LineLength	= LineEnd ? ( LineEnd - LineStart ) : (CodeLength - (LineStart - SourceCode));

		Assert(LineLength < STR_MAX_LINE_LENGTH );

		char LineBuffer[STR_MAX_LINE_LENGTH] = {};
		char* ScanPtrSrc = LineStart;
		char* ScanPtrDst = LineBuffer;
		str::CopyStrings( LineLength, LineStart, STR_MAX_LINE_LENGTH, LineBuffer );



		LineStart = str::FindFirstNotOf("\r\n", LineEnd);
		LineEnd   = str::FindFirstOf("\r\n", LineStart);

	}
	return 0;
}

internal opengl_info
OpenGLGetExtensions(b32 ModernContext)
{
	opengl_info Result = {};

	Result.ModernContext = ModernContext;
	Result.Vendor = (char*) glGetString(GL_VENDOR);
	Result.Renderer = (char*) glGetString(GL_RENDERER);
	Result.Version = (char*) glGetString(GL_VERSION);

	if(Result.ModernContext)
	{
		Result.ShadingLanguageVersion = (char*) glGetString(GL_SHADING_LANGUAGE_VERSION);
	}else{
		Result.ShadingLanguageVersion = "(None)";
 	}
	Result.Extensions = (char*) glGetString(GL_EXTENSIONS);

	u32 ExtensionsStringLength = str::StringLength( Result.Extensions );
	char* ExtensionStart = Result.Extensions;
	char* Tokens = " \t\n";
	char* ExtensionEnd   = str::FindFirstOf( Tokens, ExtensionStart );
 	while( ExtensionStart )
	{
		u64 ExtensionStringLength =  ExtensionEnd ? (u64) (ExtensionEnd - ExtensionStart) : (u64) (ExtensionStart - ExtensionsStringLength);

		if( str::Contains( str::StringLength( "EXT_texture_sRGB_decode" ), "EXT_texture_sRGB_decode", 
				   ExtensionStringLength, ExtensionStart ) )
		{
			Result.EXT_texture_sRGB_decode = true;
		}
		if( str::Contains( str::StringLength( "GL_ARB_framebuffer_sRGB" ), "GL_ARB_framebuffer_sRGB", 
				   ExtensionStringLength, ExtensionStart ) )
		{
			Result.GL_ARB_framebuffer_sRGB = true;
		}

		if( str::Contains( str::StringLength( "GL_ARB_vertex_shader" ), "GL_ARB_vertex_shader", 
				   ExtensionStringLength, ExtensionStart ) )
		{
			Result.GL_ARB_vertex_shader = true;
		}

		ExtensionStart = str::FindFirstNotOf(Tokens, ExtensionEnd);
		ExtensionEnd   = str::FindFirstOf( Tokens, ExtensionStart );
	}

	return Result;
}

void OpenGLInit(b32 ModernContext)
{
	opengl_info Info = OpenGLGetExtensions(ModernContext);
	OpenGLDefaultInternalTextureFormat = GL_RGBA8;
	if(Info.EXT_texture_sRGB_decode)
	{
		OpenGLDefaultInternalTextureFormat = GL_SRGB8_ALPHA8;
	}

	if(Info.GL_ARB_framebuffer_sRGB)
	{
		// Will take as input LinearRGB and convert to sRGB Space.
		// sRGB = Pow(LinearRGB, 1/2.2)
		glEnable(GL_FRAMEBUFFER_SRGB);
	}
}

void OpenGLSetViewport( r32 ViewPortAspectRatio, s32 WindowWidth, s32 WindowHeight )
{
	r32 AspectRatio   = WindowWidth / (r32) WindowHeight;

	s32 OffsetX = 0;
	s32 OffsetY = 0;
	s32 ViewPortWidth  = 0;
	s32 ViewPortHeight = 0;
	
	// Bars on top and bottom
	if( ViewPortAspectRatio > AspectRatio )
	{
		ViewPortWidth  = WindowWidth;
		ViewPortHeight = (s32) (ViewPortWidth / ViewPortAspectRatio);

		Assert(ViewPortHeight < WindowHeight);

		OffsetX = 0;
		OffsetY = (WindowHeight - ViewPortHeight) / 2;

	}else{
		ViewPortHeight = WindowHeight;
		ViewPortWidth  = (s32) (ViewPortAspectRatio * ViewPortHeight);

		Assert(ViewPortWidth <= WindowWidth);
		
		OffsetX = (WindowWidth - ViewPortWidth) / 2;
		OffsetY = 0;
		
	}

	glViewport( OffsetX, OffsetY, ViewPortWidth, ViewPortHeight);

}

void LoadTexture( bitmap* RenderTarget )
{
	// Enable texture slot 0
	glBindTexture( GL_TEXTURE_2D, 0 );

	// Send a Texture to GPU referenced to the texture handle
	glTexImage2D( GL_TEXTURE_2D,  0, GL_RGBA8,
  				  RenderTarget->Width,  RenderTarget->Height, 
  				  0, GL_BGRA_EXT, GL_UNSIGNED_BYTE,
  				  RenderTarget->Pixels);

	// Set texture environment state:
	// See documantation here: https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTexParameter.xhtml

	// How to resize textures
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ); // Just take nearest
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR ); // Just take nearest

	// Wrapping textures, (Mirror. Repeat border color, clamp, repeat etc... )
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );

}

internal void DisplayBitmapViaOpenGL( u32 Width, u32 Height, void* Memory )
{
	const r32 DesiredAspectRatio = 1.77968526f;
	OpenGLSetViewport( DesiredAspectRatio, Width, Height );

	// Make our texture slot current
	glBindTexture( GL_TEXTURE_2D, 0 );

//  GL_BGRA or GL_RGBA
	// Send a Texture to GPU referenced to the texture handle
	glTexImage2D( GL_TEXTURE_2D,  0, GL_RGBA8,
  				  Width,  Height, 
  				  0, GL_BGRA_EXT, GL_UNSIGNED_BYTE,
  				  Memory);

	// Set texture environment state:
	// See documantation here: https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTexParameter.xhtml

	// How to resize textures
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST ); // Just take nearest
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST ); // Just take nearest

	// Wrapping textures, (Mirror. Repeat border color, clamp, repeat etc... )
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );

	// Enable Texturing
	glEnable( GL_TEXTURE_2D );

	glClearColor(0.7f, 0.7f, 0.f, 0.f);
	glClear(GL_COLOR_BUFFER_BIT);

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glBegin(GL_TRIANGLES);
	
	r32 P = 1.f;

	glTexCoord2f( 0.f, 0.f );
	glVertex2f( -P, -P );
	glTexCoord2f( 1.f, 0.f );
	glVertex2f(  P, -P );
	glTexCoord2f( 1.f, 1.f );
	glVertex2f(  P,  P );

	glTexCoord2f( 0.f, 0.f );
	glVertex2f( -P, -P );
	glTexCoord2f( 1.f, 1.f );
	glVertex2f(  P,  P );
	glTexCoord2f( 0.f, 1.f );
	glVertex2f( -P,  P );

	glEnd();

}

void BindTexture(bitmap* Bitmap)
{
	glEnable( GL_TEXTURE_2D );
	if(Bitmap->Handle)
	{
		glBindTexture(GL_TEXTURE_2D, Bitmap->Handle);
	}else{

		Bitmap->Handle = ++TextureBindCount;
		
		// Enable texture slot 
		glBindTexture(GL_TEXTURE_2D, Bitmap->Handle);

		// Send a Texture to GPU referenced to the texture handle
		glTexImage2D( GL_TEXTURE_2D,  0, OpenGLDefaultInternalTextureFormat,
			  Bitmap->Width,  Bitmap->Height, 
		  	  0, GL_BGRA_EXT, GL_UNSIGNED_BYTE,  Bitmap->Pixels);

		// Set texture environment state:
		// See documantation here: https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTexParameter.xhtml

		// How to treat texture
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	}
}

internal void 
OpenGlPushRect(const v4* Points, const rect* TextureCoordinates, const r32 Brightness  )
{
	r32 TexXMin = TextureCoordinates->X;
	r32 TexYMin = TextureCoordinates->Y;
	r32 TexXMax = TextureCoordinates->X+TextureCoordinates->W;
	r32 TexYMax = TextureCoordinates->Y+TextureCoordinates->H;

	glBegin(GL_TRIANGLES);
	glTexCoord2f( TexXMin, TexYMin );
	glColor3f( Brightness, Brightness, Brightness );
	glVertex3f( Points[0].X,  Points[0].Y, Points[0].Z );
	glTexCoord2f( TexXMax, TexYMin );
	glColor3f( Brightness, Brightness, Brightness );
	glVertex3f( Points[1].X,  Points[1].Y, Points[1].Z );
	glTexCoord2f( TexXMax, TexYMax );
	glColor3f( Brightness, Brightness, Brightness );
	glVertex3f( Points[2].X,  Points[2].Y, Points[2].Z );
	glEnd(); 

	glBegin(GL_TRIANGLES);
	glTexCoord2f( TexXMin, TexYMin );
	glColor3f( Brightness, Brightness, Brightness );
	glVertex3f( Points[0].X, Points[0].Y, Points[0].Z );
	glTexCoord2f( TexXMax, TexYMax );
	glColor3f( Brightness, Brightness, Brightness );
	glVertex3f( Points[2].X, Points[2].Y, Points[2].Z );
	glTexCoord2f( TexXMin, TexYMax );
	glColor3f( Brightness, Brightness, Brightness );
	glVertex3f( Points[3].X, Points[3].Y, Points[3].Z );
	glEnd();
}

internal void
OpenGLRenderGroupToOutput( game_render_commands* Commands, s32 WindowWidth, s32 WindowHeight )
{
	render_push_buffer* RenderPushBuffer = (render_push_buffer*) Commands->PushBuffer;

	const r32 DesiredAspectRatio = 1.77968526f;
	OpenGLSetViewport( DesiredAspectRatio, WindowWidth, WindowHeight );

	glEnable(GL_DEPTH_TEST);
	glClearColor(0.f, 0.f, 0.f, 0.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	
	
	// Get camera matrices
	// The 'gl' means to make explicit we use the Column Major convention used by OpenGL.
	// Our math library uses Row major convention which means we need to transpose the 
	// matrices AND reverse the order of multiplication.
	// Transpose(A*B) = Transpose(B) * Transpose(A)
	m4 V = RenderPushBuffer->ViewMatrix;

	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf( Transpose(V).E );

	m4 glP = Transpose( RenderPushBuffer->ProjectionMatrix );

	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(glP.E);	

	r32 Color = 0.5;
	b32 Odd = true;
	// For each render group
	for( push_buffer_header* Entry = RenderPushBuffer->First; Entry != 0; Entry = Entry->Next )
	{
		switch(Entry->Type)
		{
			case RENDER_TYPE_ENTITY:
			{
				u8* Head = (u8*) Entry;
				u8* Body = Head + sizeof(push_buffer_header);

				entity* Entity = ( (entry_type_entity*) Body)->Entity;

				if(Odd)
				{
					Color = 0.3;
				}
				else {
					Color = 0.5;
				}
				Odd = !Odd;
				if( Entity->Types & COMPONENT_TYPE_LIGHT )
				{

				}

				if( Entity->Types & COMPONENT_TYPE_SURFACE)
				{

					component_surface* Surface = Entity->SurfaceComponent;
					bitmap* DiffuseMap = Surface->Material->DiffuseMap;
					if(DiffuseMap)
					{
						LoadTexture(DiffuseMap);
					}
				}

				if( Entity->Types & COMPONENT_TYPE_MESH )
				{
					glDisable( GL_TEXTURE_2D );
					component_mesh* Mesh = Entity->MeshComponent;
					mesh_data* MeshData = Mesh->Data;

					m4 M = M4Identity();
					if(Entity->Types & COMPONENT_TYPE_SPATIAL)
					{
		 				M = GetAsMatrix( Entity->SpatialComponent );
					}

					glMatrixMode(GL_MODELVIEW);
					m4 ModelView = V*M;
					m4 glModelView = Transpose(ModelView);
					glLoadMatrixf( glModelView.E );


					glMatrixMode(GL_PROJECTION);
					glLoadMatrixf( glP.E  );
					m4 P = RenderPushBuffer->ProjectionMatrix;


					m4 ProjectionModelView = P*V;
					for( u32 TriangleIndex = 0; TriangleIndex < Mesh->TriangleCount; ++TriangleIndex)
					{
						face* Triangle = &Mesh->Triangles[TriangleIndex];
						Assert(Triangle->nv == 3);

						v4 v[3]  = {};

						v[0] = MeshData->v[ Triangle->vi[0] ];
						v[1] = MeshData->v[ Triangle->vi[1] ];
						v[2] = MeshData->v[ Triangle->vi[2] ];

						r32 LinearColor = 0.3;
						glBegin(GL_TRIANGLES);
						glColor3f( LinearColor, 0.f, 0.f );
						glVertex3f( v[0].X, v[0].Y, v[0].Z );
						glColor3f( LinearColor, 0.f, 0.f );
						glVertex3f( v[1].X, v[1].Y, v[1].Z );
						glColor3f( LinearColor, 0.f, 0.f );
						glVertex3f( v[2].X, v[2].Y, v[2].Z );
						glEnd();
					}
				}
			}break;
			case RENDER_TYPE_FLOOR_TILE:
			{
				u8* Head = (u8*)Entry;
				u8* Body = Head + sizeof(push_buffer_header);
				
				entry_type_sprite* SpriteEntry = (entry_type_sprite*) Body;
				bitmap* Bitmap = SpriteEntry->Bitmap;

				BindTexture(Bitmap);

				v4 Translation = GetTranslationFromMatrix(SpriteEntry->M);

				r32 Width  = Index( SpriteEntry->M, 0, 0 );
				r32 Height = Index( SpriteEntry->M, 1, 1 );
				r32 Depth  = Index( SpriteEntry->M, 2, 2 );

				v4 v[4] = {};
				// BottomSquare (Depth = 0,)
				// Lower Left
				v[0] = V4(Translation.X, Translation.Y , Translation.Z, 1);
//				v[0].X = ( 2*v[0].Y + v[0].X ) / 2.f;
//				v[0].Y = ( 2*v[0].Y - v[0].X ) / 2.f;
				// Lower Right
				v[1] = V4(Translation.X + Width, Translation.Y , Translation.Z, 1);
//				v[1].X = ( 2*v[1].Y + v[1].X ) / 2.f;
//				v[1].Y = ( 2*v[1].Y - v[1].X ) / 2.f;
				// Upper Right
				v[2] = V4(Translation.X + Width, Translation.Y  + Height, Translation.Z, 1);
//				v[2].X = ( 2*v[2].Y + v[2].X ) / 2.f;
//				v[2].Y = ( 2*v[2].Y - v[2].X ) / 2.f;
				// Upper Left
				v[3] = V4(Translation.X, Translation.Y  + Height, Translation.Z, 1);
//				v[3].X = ( 2*v[3].Y + v[3].X ) / 2.f;
//				v[3].Y = ( 2*v[3].Y - v[3].X ) / 2.f;

			
				r32 Brightness = 0.8;
				OpenGlPushRect( v, SpriteEntry->Coordinates, Brightness  );
			}break;
			case RENDER_TYPE_SPRITE:
			{
				u8* Head = (u8*)Entry;
				u8* Body = Head + sizeof(push_buffer_header);

				entry_type_sprite* SpriteEntry = (entry_type_sprite*) Body;
				bitmap* Bitmap = SpriteEntry->Bitmap;

				BindTexture(Bitmap);

				v4 Translation = GetTranslationFromMatrix(SpriteEntry->M);

				r32 Width  = Index( SpriteEntry->M, 0, 0 );
				r32 Height = Index( SpriteEntry->M, 1, 1 );
				r32 Depth  = Index( SpriteEntry->M, 2, 2 );

				v4 v[4] = {};
				v[0] = V4(Translation.X,         Translation.Y,           Translation.Z, 1);
				v[1] = V4(Translation.X + Width, Translation.Y,           Translation.Z, 1);
				v[2] = V4(Translation.X + Width, Translation.Y  + Height, Translation.Z, 1);
				v[3] = V4(Translation.X,         Translation.Y  + Height, Translation.Z, 1);

				r32 Brightness = 0.8;
				OpenGlPushRect( v, SpriteEntry->Coordinates, Brightness  );

			}break;
			case RENDER_TYPE_WIREBOX:
			{

			}break;
			default: Assert(0); break;
		}
	}

	Commands->PushBufferSize = 0;
	Commands->PushBufferElementCount = 0;

}