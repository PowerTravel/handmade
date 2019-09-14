
#include "render_push_buffer.h"
#include "affine_transformations.h"
#include "string.h"

global_variable u32 TextureBindCount = 0;

internal GLuint OpenGLLoadShader( char* SourceCode, GLenum ShaderType )
{
	GLuint ShaderHandle = glCreateShader(ShaderType);

	// Upload the shader
	glShaderSource( ShaderHandle, 1, (const GLchar**) &SourceCode, NULL );
	glCompileShader( ShaderHandle );
	
	#if 1
	GLint Compiled;
	glGetShaderiv(ShaderHandle , GL_COMPILE_STATUS, &Compiled);
	if( !Compiled )
	{
		void* CompileMessage;
		GLint CompileMessageSize;
		glGetShaderiv( ShaderHandle, GL_INFO_LOG_LENGTH, &CompileMessageSize );
		
		CompileMessage = VirtualAlloc(0, CompileMessageSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
		void* Tmp 	   = VirtualAlloc(0, CompileMessageSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
		glGetShaderInfoLog(ShaderHandle, CompileMessageSize, NULL, (GLchar*) CompileMessage);
		snprintf((char*)Tmp, CompileMessageSize, "%s", (char*) CompileMessage);
		//VirtualFree(CompileMessage, 0, MEM_RELEASE );
		//VirtualFree(Tmp, 0, MEM_RELEASE );
		Assert(0);
		exit(1);
	}
	#endif

	return ShaderHandle;
};

opengl_program OpenGLCreateShaderProgram()
{
	global_variable	char VertexShaderCode[] = 
	{
"#version  330 core\n\
layout (location = 0) in vec4 vPosition;\n\
uniform mat4 M, V, P;\n\
\n\
void main(){\n\
	//gl_Position = P*V*M*vPosition;\n\
	//mat4 X = V * P;\n\
	gl_Position = M*vPosition;\n\
}"
	};

	global_variable	char FragmentShaderCode[] = 
	{
"#version 330 core\n\
//out vec4 fragColor;\n\
layout(location = 0) out vec3 fragColor;\n\
\n\
void main(){\n\
	fragColor = vec3(1, 0, 0);\n\
}"
	};

	char* SourceCode = NULL;

	GLuint VertexShader   = OpenGLLoadShader( VertexShaderCode,   GL_VERTEX_SHADER   );
	GLuint FragmentShader = OpenGLLoadShader( FragmentShaderCode, GL_FRAGMENT_SHADER );

	
	GLuint Program = glCreateProgram();

	// Attatch the shader to the program
	glAttachShader(Program, VertexShader);
	glAttachShader(Program, FragmentShader);
	glLinkProgram( Program );

	glDetachShader(Program, VertexShader);
	glDetachShader(Program, FragmentShader);

	glDeleteShader(VertexShader);
	glDeleteShader(FragmentShader);

	GLint Linked;
	glGetProgramiv(Program, GL_LINK_STATUS, &Linked);
	if( !Linked )
	{
		void* ProgramMessage;
		GLint ProgramMessageSize;
		glGetProgramiv( Program, GL_INFO_LOG_LENGTH, &ProgramMessageSize );
		
		ProgramMessage = VirtualAlloc(0, ProgramMessageSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
		void* Tmp 	   = VirtualAlloc(0, ProgramMessageSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
		glGetProgramInfoLog(Program, ProgramMessageSize, NULL, (GLchar*) ProgramMessage);
		snprintf((char*)Tmp, ProgramMessageSize, "%s",(char*) ProgramMessage);
		VirtualFree(ProgramMessage, 0, MEM_RELEASE );
		VirtualFree(Tmp, 0, MEM_RELEASE );
		exit(1);
	}


	opengl_program Result = {};
	Result.Program 				   = Program;
	glUseProgram(Program);
	Result.UniformModel 		 = glGetUniformLocation(Program, "M");
	u32 error = glGetError();
	Result.UniformProjection = glGetUniformLocation(Program, "P");
	error = glGetError();
	Result.UniformView 			 = glGetUniformLocation(Program, "V");
	error = glGetError();

	glUseProgram(0);
	
	return Result;
} 

void OpenGLPushDataToArrayBuffer(u32 Slot, u32 Dimension, size_t NrEntries, r32* Data )
{
	if(!Data) return;

	// Generate a generic buffer
	GLuint BufferID;
	glGenBuffers(1, &BufferID);
	
	// Bind the buffer ot the GL_ARRAY_BUFFER slot
	glBindBuffer( GL_ARRAY_BUFFER, BufferID);

	// Send data to it
	glBufferData( GL_ARRAY_BUFFER, NrEntries * Dimension * sizeof(GLfloat), Data, GL_STATIC_DRAW);

	// Say how to interpret the buffer data
	glVertexAttribPointer(Slot, Dimension, GL_FLOAT, GL_FALSE, 0, (GLvoid*)NULL);

	// Enable the Slot
	glEnableVertexAttribArray(Slot);
}

void OpenGLSendMeshToGPU( u32* VAO,	
												  u32 NrVertices,  r32* Vertices, 
											    u32 NrTexCoords, r32* TexCoords, 
											    u32 NrNormals,   r32* Normals,
											    u32 NrFaces,     u32* Faces )
{
	Assert(Vertices);
	Assert(Faces);
	
	// Generate vao
	glGenVertexArrays(1, VAO);

	// set it as current one	
	glBindVertexArray(*VAO);

	OpenGLPushDataToArrayBuffer( 0, 4, NrVertices,  Vertices);
	OpenGLPushDataToArrayBuffer( 1, 2, NrTexCoords, TexCoords);
	OpenGLPushDataToArrayBuffer( 2, 3, NrNormals,   Normals);

	GLuint FaceBuffer;
	glGenBuffers(1, &FaceBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, FaceBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, NrFaces * 3 * sizeof(GLuint), Faces, GL_STATIC_DRAW);
	
	glBindVertexArray(0);
}	

void OpenGLDraw( u32 VertexArrayObject, u32 nrFaces )
{
	glBindVertexArray( VertexArrayObject );
	glDrawElements( GL_TRIANGLES, nrFaces, GL_UNSIGNED_INT, (void*)0);
	glBindVertexArray(0);
}

void OpenGLBeginFrame( u32 Program )
{
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	
}

internal opengl_info
OpenGLGetExtensions(b32 ModernContext)
{
	opengl_info Result = {};

	Result.ModernContext = ModernContext;
	Result.Vendor   = (char*) glGetString(GL_VENDOR);
	Result.Renderer = (char*) glGetString(GL_RENDERER);
	Result.Version  = (char*) glGetString(GL_VERSION);

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

		if( str::Contains( str::StringLength( "GL_EXT_blend_func_separate" ), "GL_EXT_blend_func_separate",
					ExtensionStringLength, ExtensionStart ) )
		{
			Result.GL_blend_func_separate = true;
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

	if(Info.GL_blend_func_separate)
	{
		// Not sure I want this
		//glEnable(GL_BLEND);
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

void BindTexture(bitmap* Bitmap, b32 IsBackground )
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

		if(IsBackground){
		// How to treat texture
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		}else{
			/*
			  glActiveTexture(GL_TEXTURE0);
			  glEnable(GL_TEXTURE_2D);
			  glBindTexture(GL_TEXTURE_2D, textureID0);
			  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
			  // --------------------
			  glActiveTexture(GL_TEXTURE1);
			  glEnable(GL_TEXTURE_2D);
			  glBindTexture(GL_TEXTURE_2D, textureID1);
			  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
			  glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_INTERPOLATE);    // Interpolate RGB with RGB
			  glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PREVIOUS);
			  glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_TEXTURE);
			  glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB, GL_PREVIOUS);
			  glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
			  glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
			  glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB, GL_SRC_ALPHA);
			  // --------------------
			  glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_INTERPOLATE);    // Interpolate ALPHA with ALPHA
			  glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PREVIOUS);
			  glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA, GL_TEXTURE);
			  glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_ALPHA, GL_PREVIOUS);
			  glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
			  glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA);
			  glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_ALPHA, GL_SRC_ALPHA);
			*/
		}
	}
}

internal void 
OpenGlPushRect(const v4* Points, const rect2f* TextureCoordinates, const r32 Brightness  )
{
	r32 TexXMin = 0;
	r32 TexYMin = 0;
	r32 TexXMax = 0;
	r32 TexYMax = 0;

	if(TextureCoordinates)
	{
		TexXMin = TextureCoordinates->X;
		TexYMin = TextureCoordinates->Y;
		TexXMax = TextureCoordinates->X+TextureCoordinates->W;
		TexYMax = TextureCoordinates->Y+TextureCoordinates->H;
	}

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

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS); 
	// Cull triangles which normal is not towards the camera
	glEnable(GL_CULL_FACE);	

	glClearColor(0,0,0.4,1);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	const r32 DesiredAspectRatio = 1.77968526f;
	OpenGLSetViewport( DesiredAspectRatio, WindowWidth, WindowHeight );


	// OpenGL uses Column major convention.
	// Our math library uses Row major convention which means we need to transpose the 
	// matrices AND reverse the order of multiplication.
	// Transpose(A*B) = Transpose(B) * Transpose(A)
	
	const m4 V = RenderPushBuffer->ViewMatrix;
	const m4 P = RenderPushBuffer->ProjectionMatrix;

	opengl_program Prog = Commands->RenderProgram;
	glUniformMatrix4fv(Prog.UniformModel, 1, GL_TRUE, V.E);
	glUniformMatrix4fv(Prog.UniformView,  1, GL_TRUE, V.E);
#if 0
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf( Transpose(V).E );

	m4 glP = Transpose( RenderPushBuffer->ProjectionMatrix );

	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(glP.E);	
#endif

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

				if(( Entity->Types & COMPONENT_TYPE_MESH ) && 
					 ( Entity->Types & COMPONENT_TYPE_SPATIAL ))
				{
					Assert(Entity->Types & COMPONENT_TYPE_SPATIAL);

					glDisable( GL_TEXTURE_2D );
					component_mesh* Mesh = Entity->MeshComponent;

					mesh_data* MeshData = Mesh->Data;
					if(!Mesh->VAO)
					{
						OpenGLSendMeshToGPU(  &Mesh->VAO,	
								MeshData->nv, (r32*) MeshData->v, 
								0, NULL, 
								0, NULL,
								Mesh->Indeces.Count, Mesh->Indeces.vi );
					}


					local_persist float t = 0;
					r32 s = (r32) sin(t);
					r32 c = (r32) cos(t);
		 			m4 M = GetAsMatrix( Entity->SpatialComponent );
					M = GetScaleMatrix( V4( 0.5, 0.5, 0.5, 1) );
					m4 PP = GetTranslationMatrix(V4(c,s,0,0));
					t+=0.02;
					glUniformMatrix4fv(Prog.UniformModel, 1, GL_TRUE, (M*PP).E);
					
					OpenGLDraw( Mesh->VAO, Mesh->Indeces.Count );
#if 0
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
					#endif
				}
			}break;
			case RENDER_TYPE_FLOOR_TILE:
			{
				u8* Head = (u8*)Entry;
				u8* Body = Head + sizeof(push_buffer_header);
				
				entry_type_sprite* SpriteEntry = (entry_type_sprite*) Body;
				bitmap* Bitmap = SpriteEntry->Bitmap;

				BindTexture(Bitmap, true);

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
	//		default: Assert(0); break;
		}
	}


	for( push_buffer_header* Entry = RenderPushBuffer->First; Entry != 0; Entry = Entry->Next )
	{
		switch(Entry->Type)
		{
			case RENDER_TYPE_SPRITE:
			{
				glDisable(GL_DEPTH_TEST);
				u8* Head = (u8*)Entry;
				u8* Body = Head + sizeof(push_buffer_header);

				entry_type_sprite* SpriteEntry = (entry_type_sprite*) Body;
				bitmap* Bitmap = SpriteEntry->Bitmap;

				BindTexture(Bitmap, true);

				v4 Translation = GetTranslationFromMatrix(SpriteEntry->M);

				r32 HalfWidth  = Index( SpriteEntry->M, 0, 0 )/2.f;
				r32 HalfHeight = Index( SpriteEntry->M, 1, 1 )/2.f;
				r32 HalfDepth  = Index( SpriteEntry->M, 2, 2 )/2.f;

				v4 v[4] = {};
				v[0] = V4(Translation.X - HalfWidth, Translation.Y - HalfHeight, Translation.Z, 1);
				v[1] = V4(Translation.X + HalfWidth, Translation.Y - HalfHeight, Translation.Z, 1);
				v[2] = V4(Translation.X + HalfWidth, Translation.Y + HalfHeight, Translation.Z, 1);
				v[3] = V4(Translation.X - HalfWidth, Translation.Y + HalfHeight, Translation.Z, 1);

				r32 Brightness = 0.8;
				OpenGlPushRect( v, SpriteEntry->Coordinates, Brightness  );

			}break;
		}
	}
#if 0
	for( push_buffer_header* Entry = RenderPushBuffer->First; Entry != 0; Entry = Entry->Next )
	{
		switch(Entry->Type)
		{
			case RENDER_TYPE_WIREBOX:
			{
				glDisable(GL_DEPTH_TEST);
				u8* Head = (u8*)Entry;
				u8* Body = Head + sizeof(push_buffer_header);

				entry_type_wirebox* SpriteEntry = (entry_type_wirebox*) Body;
				rect2f R = SpriteEntry->Rect;
				v4 v[4] = {};
				v[0] = V4(R.X,         R.Y,           0, 1);
				v[1] = V4(R.X + R.W,   R.Y,           0, 1);
				v[2] = V4(R.X + R.W,   R.Y  + R.H,    0, 1);
				v[3] = V4(R.X,         R.Y  + R.H,    0, 1);
				glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

				glBindTexture(GL_TEXTURE_2D, 0);
				glColor3f(1, 1, 1);
				glBegin(GL_TRIANGLES);
				glVertex3f(v[0].X, v[0].Y, v[0].Z);
				glVertex3f(v[1].X, v[1].Y, v[1].Z);
				glVertex3f(v[2].X, v[2].Y, v[2].Z);
				glEnd();

				glBegin(GL_TRIANGLES);
				glVertex3f(v[0].X, v[0].Y, v[0].Z);
				glVertex3f(v[2].X, v[2].Y, v[2].Z);
				glVertex3f(v[3].X, v[3].Y, v[3].Z);
				glEnd();

				glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

			}break;
		}
	}
#endif
	Commands->PushBufferSize = 0;
	Commands->PushBufferElementCount = 0;

}

