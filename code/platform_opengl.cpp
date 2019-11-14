
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
		char* CompileMessage;
		GLint CompileMessageSize;
		glGetShaderiv( ShaderHandle, GL_INFO_LOG_LENGTH, &CompileMessageSize );
		
		CompileMessage = (char*) VirtualAlloc(0, CompileMessageSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
		char* Tmp 	   = (char*) VirtualAlloc(0, CompileMessageSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
		glGetShaderInfoLog(ShaderHandle, CompileMessageSize, NULL, (GLchar*) CompileMessage);
		snprintf((char*)Tmp, CompileMessageSize, "%s", CompileMessage);
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
layout (location = 0) in vec3 vertice;\n\
layout (location = 1) in vec3 verticeNormal;\n\
layout (location = 2) in vec2 textureCoordinate;\n\
\n\
uniform mat4 M;  // Model Matrix - Transforms points from ModelSpace to WorldSpace.\n\
                 // Includes Translation, Rotation and Scaling\n\
uniform mat4 NM; // Normal Model Matrix = Transpose( RigidInverse(M) );\n\
                 // Keeps normals correct under shearing scaling\n\
uniform mat4 V;  // View Matrix - Camera Position and Orientation in WorldSpace\n\
uniform mat4 P;  // Projection Matrix - Scales the visible world into the unit cube.\n\
\n\
// Premultiplied color values\n\
uniform vec4 ambientProduct, diffuseProduct, specularProduct; \n\
\n\
uniform vec4 lightPosition;  // World space\n\
uniform vec4 cameraPosition; // World space\n\
\n\
uniform float shininess; // Shininess of material\n\
\n\
out float Kd; // Diffuse angle cos-value\n\
out float Ks; // Specular angle cos-value\n\
\n\
//out float r;  // Length of L;\n\
\n\
out vec4 vertexColor;\n\
out vec2  texCoord;\n\
\n\
void main()\n\
{\n\
\n\
// Variable Descriptions:\n\
// N [Normal Vector], Object Space\n\
// L [Light Vector] Unit vector of vertex to light in world space;\n\
// E [Eye Vector]   Unit vector of vertex to camera in world space;\n\
// H [Half Vector]  Unit vector pointing between between L and E;\n\
\n\
	vec4 Vertice = M * vec4(vertice,1);\n\
	vec4 N = normalize( NM * vec4(verticeNormal,0) );\n\
	vec4 L = normalize( lightPosition - Vertice );\n\
	Kd = max(dot(L,N), 0.0);\n\
\n\
	vec4 E = normalize( cameraPosition - Vertice );\n\
	vec4 H = normalize(L+E);\n\
	Ks = pow(max(dot(H,N), 0.0 ), shininess);\n\
\n\
\n\
	vertexColor = ambientProduct + Kd*(diffuseProduct + Ks*specularProduct);\n\
\n\
	texCoord = textureCoordinate;\n\
	gl_Position = P*V*Vertice;\n\
}\n\
"};

	global_variable	char FragmentShaderCode[] = 
	{
"#version 330 core\n\
in vec4  vertexColor;\n\
in vec2  texCoord;\n\
out vec4 fragColor;\n\
\n\
uniform sampler2D ourTexture;\n\
\n\
void main() \n\
{\n\
	fragColor = texture(ourTexture, texCoord) * vertexColor;\n\
}\n\
"};

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
	Result.Program = Program;
	glUseProgram(Program);

	Result.M =  glGetUniformLocation(Program, "M");
	Result.NM = glGetUniformLocation(Program, "NM");
	Result.P =  glGetUniformLocation(Program, "P");
	Result.V =  glGetUniformLocation(Program, "V");

    Result.lightPosition   = glGetUniformLocation(Program, "lightPosition");
    Result.cameraPosition  = glGetUniformLocation(Program, "cameraPosition");
    Result.ambientProduct  = glGetUniformLocation(Program, "ambientProduct");
    Result.diffuseProduct  = glGetUniformLocation(Program, "diffuseProduct");
    Result.specularProduct = glGetUniformLocation(Program, "specularProduct");
    Result.attenuation     = glGetUniformLocation(Program, "attenuation");
    Result.shininess       = glGetUniformLocation(Program, "shininess");
	glUseProgram(0);
	
	return Result;
} 

void OpenGLSendMeshToGPU( u32* VAO, 
	 const u32 NrIndeces, const u32* IndexData, const opengl_vertex* VertexData)
{
	Assert(VertexData);
	Assert(IndexData);
	
	// Generate vao
	glGenVertexArrays(1, VAO);

	// set it as current one	
	glBindVertexArray(*VAO);

	// Generate a generic buffer
	GLuint DataBuffer;
	glGenBuffers(1, &DataBuffer);
	// Bind the buffer ot the GL_ARRAY_BUFFER slot
	glBindBuffer( GL_ARRAY_BUFFER, DataBuffer);
	
	// Send data to it
	glBufferData( GL_ARRAY_BUFFER, NrIndeces * sizeof(opengl_vertex), (GLvoid*) VertexData, GL_STATIC_DRAW);

	// Say how to interpret the buffer data
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(opengl_vertex), (GLvoid*) NULL);           // Vertecis
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(opengl_vertex), (GLvoid*) sizeof(v3) );    // Normals
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(opengl_vertex), (GLvoid*) (2*sizeof(v3))); // Textures


	GLuint FaceBuffer;
	glGenBuffers(1, &FaceBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, FaceBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, NrIndeces * sizeof(u32), IndexData, GL_STATIC_DRAW);
	
	glBindVertexArray(0);
}	

void OpenGLDraw( u32 VertexArrayObject, u32 nrVertecies )
{
	glBindVertexArray( VertexArrayObject );
	glDrawElements( GL_TRIANGLES, nrVertecies, GL_UNSIGNED_INT, (void*)0);
	glBindVertexArray(0);
}

internal opengl_info
OpenGLGetExtensions()
{
	opengl_info Result = {};

	Result.Vendor   = (char*) glGetString(GL_VENDOR);
	Result.Renderer = (char*) glGetString(GL_RENDERER);
	Result.Version  = (char*) glGetString(GL_VERSION);

	Result.ShadingLanguageVersion = (char*) glGetString(GL_SHADING_LANGUAGE_VERSION);
	Result.Extensions = (char*) glGetString(GL_EXTENSIONS);

	u32 ExtensionsStringLength = str::StringLength( Result.Extensions );
	char* ExtensionStart = Result.Extensions;
	char* Tokens = " \t\n";
	char* ExtensionEnd   = str::FindFirstOf( Tokens, ExtensionStart );
 	while( ExtensionStart )
	{
		u64 ExtensionStringLength =  ExtensionEnd ? (u64) (ExtensionEnd - ExtensionStart) : (u64) (ExtensionStart - ExtensionsStringLength);

        // NOTE: EXT_texture_sRGB_decode has been core since 2.1
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

void OpenGLInitExtensions()
{
	opengl_info Info = OpenGLGetExtensions();
	OpenGLDefaultInternalTextureFormat = GL_RGBA8;
	if(Info.EXT_texture_sRGB_decode)
	{
		OpenGLDefaultInternalTextureFormat = GL_SRGB8_ALPHA8;
	}

	if(Info.GL_ARB_framebuffer_sRGB)
	{
		// Will take as input LinearRGB and convert to sRGB Space.
		// sRGB = Pow(LinearRGB, 1/2.2)
		// glEnable(GL_FRAMEBUFFER_SRGB);
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
	if(RenderTarget->Handle)
	{
		glBindTexture( GL_TEXTURE_2D, RenderTarget->Handle );	
		return;
	}

	glBindTexture( GL_TEXTURE_2D, RenderTarget->Handle );
	// Generate a texture slot
	glGenTextures(1, &RenderTarget->Handle);

	// Enable texture slot
	glBindTexture( GL_TEXTURE_2D, RenderTarget->Handle );

	// Send a Texture to GPU referenced to the enabled texture slot
	glTexImage2D( GL_TEXTURE_2D,  0, GL_RGBA8,
  				  RenderTarget->Width,  RenderTarget->Height,
  				  0, GL_BGRA_EXT, GL_UNSIGNED_BYTE,
  				  RenderTarget->Pixels);

	// Set texture environment state:
	// See documantation here: https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTexParameter.xhtml

	// How to resize textures
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST ); // Just take nearest
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST ); // Just take nearest

	// Wrapping textures, (Mirror. Repeat border color, clamp, repeat etc... )
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
	glBindTexture( GL_TEXTURE_2D, 0 );

}

v4 Blend(v4* A, v4* B)
{
	v4 Result =  V4( A->X * B->X,
				     A->Y * B->Y,
				     A->Z * B->Z,
				     A->W * B->W );
	return Result;
}

void PushGeometryToOpenGL( game_render_commands* Commands, component_mesh* Mesh)
{
	mesh_data* MeshData  = Mesh->Data;
	mesh_indeces Indeces = Mesh->Indeces;

	u32 NrIndeces  = Indeces.Count;

	Assert(Commands->TemporaryMemory.IsEmpty());

	u32 MemoryNeeded = NrIndeces * (sizeof(opengl_vertex) + sizeof(u32));

	Assert( MemoryNeeded <= Commands->TemporaryMemory.Remaining() );

	u8* VertexMemory =  Commands->TemporaryMemory.GetTail();
	for( u32 Index = 0; Index < NrIndeces; ++Index )
	{
		opengl_vertex* VertexData = (opengl_vertex*) Commands->TemporaryMemory.GetMemory(sizeof(opengl_vertex));
		if(Indeces.vi)
		{
			u32 VecIndex  = Indeces.vi[Index];
			VertexData->v  = MeshData->v[VecIndex];
		}
		if(Indeces.ni)
		{
			u32 NormIndex  = Indeces.ni[Index];
			VertexData->vn = MeshData->vn[NormIndex];	
		}
		if(Indeces.ti)
		{
			u32 TexIndex = Indeces.ti[Index];
			VertexData->vt = MeshData->vt[TexIndex];
		}
	}
	
	u8* IndexMemory =  Commands->TemporaryMemory.GetTail();
	for( u32 Index = 0; Index < NrIndeces; ++Index )
	{
		u32* IndexData = (u32*) Commands->TemporaryMemory.GetMemory(sizeof(u32));
		*IndexData = Index;
	}

	OpenGLSendMeshToGPU(  &Mesh->VAO, NrIndeces, (u32*) IndexMemory, (opengl_vertex*) VertexMemory );

	Commands->TemporaryMemory.Clear();
}

internal void
OpenGLRenderGroupToOutput( game_render_commands* Commands, s32 WindowWidth, s32 WindowHeight )
{
	render_push_buffer* RenderPushBuffer = (render_push_buffer*) Commands->RenderMemory.GetBase();

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Enable Textures
	glEnable(GL_TEXTURE_2D);
	// Cull triangles which normal is not towards the camera
	glEnable(GL_CULL_FACE);	

	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS); 

	glClearColor(0,0,0.4,1);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	const r32 DesiredAspectRatio = 1.77968526f;
	OpenGLSetViewport( DesiredAspectRatio, WindowWidth, WindowHeight );

	// OpenGL uses Column major convention.
	// Our math library uses Row major convention which means we need to transpose the 
	// matrices AND reverse the order of multiplication.
	// Transpose(A*B) = Transpose(B) * Transpose(A)
	m4 V = RenderPushBuffer->ViewMatrix;
	m4 P = RenderPushBuffer->ProjectionMatrix;

	opengl_program* Prog = &Commands->RenderProgram;
	glUniformMatrix4fv(Prog->P,  1, GL_TRUE, P.E);
	glUniformMatrix4fv(Prog->V,  1, GL_TRUE, V.E);

    r32 Attenuation = 1;
	
	glUniform4fv( Prog->cameraPosition, 1, GetCameraPosition(&V).E);
	glUniform1f(  Prog->attenuation, Attenuation);

	v4 LightColor    = V4(0,0,0,1);

	// For each render group
	for( push_buffer_header* Entry = RenderPushBuffer->First; Entry != 0; Entry = Entry->Next )
	{
		u8* Head = (u8*) Entry;
		u8* Body = Head + sizeof(push_buffer_header);
		switch(Entry->Type)
		{
			case render_type::LIGHT:
			{
				entry_type_light* Light = (entry_type_light*) Body;
				glUniform4fv( Prog->lightPosition,  1, (Light->M*V4(0,0,0,1)).E);
				LightColor    = Light->Color;
			}break;

			case render_type::MESH:
			{
				entry_type_mesh* MeshEntry = (entry_type_mesh*) Body;

				glUniformMatrix4fv(Prog->M,  1, GL_TRUE, MeshEntry->M.E);
				glUniformMatrix4fv(Prog->NM, 1, GL_TRUE, MeshEntry->NM.E);;

				glDisable( GL_TEXTURE_2D );

				if(!MeshEntry->Mesh->VAO)
				{
					PushGeometryToOpenGL(Commands, MeshEntry->Mesh);
				}

				if( MeshEntry->Surface )
				{
					component_surface* Surface = MeshEntry->Surface;
                    material* Material =  Surface->Material;
  					
					u32 SurfaceSmoothnes = 3;
                    v4 AmbientColor   = Blend(&LightColor, &Material->AmbientColor);
                    v4 DiffuseColor   = Blend(&LightColor, &Material->DiffuseColor) * (1.f / 3.1415f);
                    v4 SpecularColor  = Blend(&LightColor, &Material->SpecularColor) * ( SurfaceSmoothnes + 8.f ) / (8.f*3.1415f);

					if(Material->DiffuseMap)
					{
						LoadTexture(Material->DiffuseMap);
						AmbientColor + V4(0,0,0,0);
					}else{
						local_persist bitmap EmptyBitmap = {};
						u8 WhitePixel[4] = {255,255,255,255};
						EmptyBitmap.Width  = 1;
						EmptyBitmap.Height = 1;
						EmptyBitmap.Pixels = (void*) WhitePixel;
						LoadTexture(&EmptyBitmap);
					}
                    
                    glUniform4fv( Prog->ambientProduct,  1, AmbientColor.E);
                    glUniform4fv( Prog->diffuseProduct,  1, DiffuseColor.E);
                    glUniform4fv( Prog->specularProduct, 1, SpecularColor.E);
                    glUniform1f( Prog->shininess, Material->Shininess);

				}
				
				OpenGLDraw( MeshEntry->Mesh->VAO, MeshEntry->Mesh->Indeces.Count );
			
			}break;
		}
	}

#if 0
	for( push_buffer_header* Entry = RenderPushBuffer->First; Entry != 0; Entry = Entry->Next )
	{
		switch(Entry->Type)
		{
			case render_type::SPRITE:
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
//				OpenGlPushRect( v, SpriteEntry->Coordinates, Brightness  );

			}break;
		}
	}
#endif
}



#if 1

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

#endif