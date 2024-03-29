
#include "render_push_buffer.h"
#include "math/affine_transformations.h"
#include "bitmap.h"
#include "assets.cpp"

inline internal void OpenGLPrintProgramLog(GLuint Program)
{
  GLint ProgramMessageSize;
  glGetProgramiv( Program, GL_INFO_LOG_LENGTH, &ProgramMessageSize );
  char* ProgramMessage = (char*) VirtualAlloc(0, ProgramMessageSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  glGetProgramInfoLog(Program, ProgramMessageSize, NULL, (GLchar*) ProgramMessage);
  OutputDebugStringA(ProgramMessage);
  VirtualFree(ProgramMessage, 0, MEM_RELEASE);
}

inline internal void OpenGLPrintShaderLog(GLuint Shader)
{
  GLint CompileMessageSize;
  glGetShaderiv( Shader, GL_INFO_LOG_LENGTH, &CompileMessageSize );
  char* CompileMessage = (char*) VirtualAlloc(0, CompileMessageSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  glGetShaderInfoLog(Shader, CompileMessageSize, NULL, (GLchar*) CompileMessage);
  OutputDebugStringA(CompileMessage);
  VirtualFree(CompileMessage, 0, MEM_RELEASE);
}


static GLuint OpenGLLoadShader( char* SourceCode, GLenum ShaderType )
{
  GLuint Shader = glCreateShader(ShaderType);
  glShaderSource( Shader, 1, (const GLchar**) &SourceCode, NULL );
  glCompileShader( Shader );
  GLint Compiled;
  glGetShaderiv(Shader, GL_COMPILE_STATUS, &Compiled);
  if(!Compiled)
  {
    OpenGLPrintShaderLog(Shader);
    exit(1);
  }

  return Shader;
};

static GLuint OpenGLCreateProgram( char* VertexShaderCode, char* FragmentShaderCode )
{
  GLuint VertexShader   = OpenGLLoadShader( VertexShaderCode,   GL_VERTEX_SHADER   );
  GLuint FragmentShader = OpenGLLoadShader( FragmentShaderCode, GL_FRAGMENT_SHADER );
  
  GLuint Program = glCreateProgram();

  // Attatch the shader to the program
  glAttachShader(Program, VertexShader);
  glAttachShader(Program, FragmentShader);
  glLinkProgram(Program);

  GLint Linked;
  glGetProgramiv(Program, GL_LINK_STATUS, &Linked);
  if(!Linked)
  {
    OpenGLPrintProgramLog(Program);
    exit(1);
  }

  glValidateProgram(Program);
  GLint Valid;
  glGetProgramiv(Program, GL_VALIDATE_STATUS, &Valid);
  if(!Valid)
  {
    OpenGLPrintProgramLog(Program);
    exit(1);
  }

  glDetachShader(Program, VertexShader);
  glDetachShader(Program, FragmentShader);
  
  glDeleteShader(VertexShader);
  glDeleteShader(FragmentShader);
  
  
  return Program;
}

// glDrawElementsInstanced is used when we have two buffers (1 Buffer with a geometry and another buffer with Instance Data )
// https://stackoverflow.com/questions/24516993/is-it-possible-to-use-index-buffer-objects-ibo-with-the-function-glmultidrawe

// glDrawElements uses 1 VertexBuffer and 1 IndexBuffer and draws the VertexBuffer using an offset into IndexBuffer

// glMultiDrawElements uses 1 VertexBuffer and 1 IndexBuffer and draws n number of whats in the VertexBuffer using an offset into IndexBuffer specific for each n

opengl_program OpenGLCreateTextProgram()
{
  char VertexShaderCode[] = R"FOO(
#version  330 core
uniform mat4 P;  // Projection Matrix - Transforms points from ScreenSpace to UnitQube.
layout (location = 0) in vec3 vertice;
layout (location = 2) in vec2 uv;
layout (location = 3) in int TexSlot;
layout (location = 4) in vec4 QuadRect;
layout (location = 5) in vec4 UVRect;
layout (location = 6) in vec4 Color;
out vec4 VertexColor;
out vec3 ArrayUV;
void main()
{
  mat3 projection;
  projection[0] = vec3(P[0].x,0,0);
  projection[1] = vec3(0,P[1].y,0);
  projection[2] = vec3(P[3].x,P[3].y,1);

  mat3 QuadTransform;
  QuadTransform[0] = vec3(QuadRect.z, 0, 0); // z=Width
  QuadTransform[1] = vec3(0, QuadRect.w, 0); // w=Height
  QuadTransform[2] = vec3(QuadRect.xy, 1);   // x,y = x,y

  mat3 TextureTransform;
  TextureTransform[0] = vec3(UVRect.z, 0, 0);
  TextureTransform[1] = vec3(0, UVRect.w, 0);
  TextureTransform[2] = vec3(UVRect.xy, 1);

  gl_Position = vec4((projection*QuadTransform*vec3(vertice.xy,1)).xy,0,1);
  vec2 TextureCoordinate = (TextureTransform*vec3(uv.xy,1)).xy;
  VertexColor = Color;
  ArrayUV = vec3(TextureCoordinate.x, TextureCoordinate.y, TexSlot);
}
)FOO";
  
  char* FragmentShaderCode = R"FOO(
#version 330 core
out vec4 fragColor;
in vec4 VertexColor;
in vec3 ArrayUV;
uniform sampler2D ourTexture;
uniform sampler2DArray TextureSampler;
void main() 
{
  
  vec4 Sample = texture(TextureSampler, ArrayUV);
  fragColor = Sample * VertexColor;
}
)FOO";
  
  opengl_program Result = {};
  Result.Program = OpenGLCreateProgram( VertexShaderCode, FragmentShaderCode );
  glUseProgram(Result.Program);
  Result.ProjectionMat    = glGetUniformLocation(Result.Program, "ProjectionMat");
  glUseProgram(0);
  
  return Result;
}


opengl_program OpenGLCreateTexturedQuadProgram()
{
  char VertexShaderCode[] = R"FOO(
#version  330 core
uniform mat4 P;  // Projection Matrix - Transforms points from ScreenSpace to UnitQube.
layout (location = 0) in vec3 vertice;
layout (location = 2) in vec2 uv;
layout (location = 3) in int TexSlot;
layout (location = 4) in vec4 QuadRect;
layout (location = 5) in vec4 UVRect;
out vec3 ArrayUV;
void main()
{
  mat3 projection;
  projection[0] = vec3(P[0].x,0,0);
  projection[1] = vec3(0,P[1].y,0);
  projection[2] = vec3(P[3].x,P[3].y,1);

  mat3 QuadTransform;
  QuadTransform[0] = vec3(QuadRect.z, 0, 0); // z=Width
  QuadTransform[1] = vec3(0, QuadRect.w, 0); // w=Height
  QuadTransform[2] = vec3(QuadRect.xy, 1);   // x,y = x,y

  mat3 TextureTransform;
  TextureTransform[0] = vec3(UVRect.z, 0, 0);
  TextureTransform[1] = vec3(0, UVRect.w, 0);
  TextureTransform[2] = vec3(UVRect.xy, 1);

  gl_Position = vec4((projection*QuadTransform*vec3(vertice.xy,1)).xy,0,1);
  vec2 TextureCoordinate = (TextureTransform*vec3(uv.xy,1)).xy;
  ArrayUV = vec3(TextureCoordinate.x, TextureCoordinate.y, TexSlot);
}
)FOO";
  
  char* FragmentShaderCode = R"FOO(
#version 330 core
out vec4 fragColor;
in vec3 ArrayUV;
uniform sampler2D ourTexture;
uniform sampler2DArray TextureSampler;
void main() 
{
  
  vec4 Sample = texture(TextureSampler, ArrayUV);
  fragColor = Sample;
}
)FOO";
  
  opengl_program Result = {};
  Result.Program = OpenGLCreateProgram( VertexShaderCode, FragmentShaderCode );
  glUseProgram(Result.Program);
  Result.ProjectionMat = glGetUniformLocation(Result.Program, "ProjectionMat");
  glUseProgram(0);
  
  return Result;
}


opengl_program OpenGLCreateUntexturedQuadOverlayQuadProgram()
{
  char VertexShaderCode[] = R"FOO(
#version  330 core
uniform mat4 ProjectionMat;
layout (location = 0) in vec3 v;
layout (location = 3) in vec4 rect;
layout (location = 4) in vec4 color;
out vec4 vertexColor;
void main()
{
  mat3 projection;
  projection[0] = vec3(ProjectionMat[0].x,0,0);
  projection[1] = vec3(0,ProjectionMat[1].y,0);
  projection[2] = vec3(ProjectionMat[3].x,ProjectionMat[3].y,1);

  mat3 quadTransform;
  quadTransform[0] = vec3(rect.z, 0, 0); // z=Width
  quadTransform[1] = vec3(0, rect.w, 0); // w=Height
  quadTransform[2] = vec3(rect.x + 0.5*rect.z,
                          rect.y + 0.5*rect.w, 1);   // x,y = x,y

  gl_Position = vec4((projection*quadTransform*vec3(v.xy,1)).xy,0,1);
  vertexColor = color;
}
)FOO";
  
  char FragmentShaderCode[] = R"FOO(
#version 330 core
out vec4 fragColor;
in vec4 vertexColor;
void main() 
{
  fragColor = vertexColor;
}
)FOO";
  
  opengl_program Result = {};
  Result.Program = OpenGLCreateProgram( VertexShaderCode, FragmentShaderCode );
  glUseProgram(Result.Program);
  Result.ProjectionMat    = glGetUniformLocation(Result.Program, "ProjectionMat");
  glUseProgram(0);
  
  return Result;
}



opengl_program OpenGLCreateProgram3D()
{
  char* VertexShaderCode =
  R"FOO(
  #version  330 core
  layout (location = 0) in vec3 v;
  layout (location = 1) in vec3 vn;
  layout (location = 2) in vec2 uv;
  
  uniform mat4 ModelMat;        // Model Matrix - Transforms points from ModelSpace to WorldSpace.
  uniform mat4 NormalMat;       // Normal Model Matrix = Transpose( RigidInverse(M) );
  uniform mat4 TextureMat;      // View Matrix - Camera Position and Orientation in WorldSpace
  uniform mat4 ViewMat;         // View Matrix - Camera Position and Orientation in WorldSpace
  uniform mat4 ProjectionMat;   // Projection Matrix - Scales the visible world into the unit cube.
  uniform float Shininess;      // Shininess of material
  
  uniform vec4 LightPosition;  // World space
  uniform vec4 CameraPosition; // World space
  
  out vec2 TextureCoordinate;
  out float Ks;
  out float Kd;
  void main()
  {
  
  // Variable Descriptions:
  // V [Vertex Point], World Space
  // N [Normal Vector], Object Space
  // L [Light Vector] Unit vector of vertex to light in world space;
  // E [Eye Vector]   Unit vector of vertex to camera in world space;
  // H [Half Vector]  Unit vector pointing between between L and E;
  
    vec4 V = ModelMat * vec4(v,1);
    vec4 N = normalize( NormalMat * vec4(vn,0) );
    vec4 L = normalize( LightPosition - V );
    Kd = max(dot(L,N), 0.0);
  
    vec4 E = normalize( CameraPosition - V );
    vec4 H = normalize(L+E);
    Ks = max(pow(max(dot(H,N), 0.0 ), Shininess),0.0);
  
    TextureCoordinate = vec2(TextureMat*vec4(uv,0,1));
    gl_Position = ProjectionMat*ViewMat*V;
  }
  )FOO";
  
  char* FragmentShaderCode = R"FOO(
#version 330 core
in vec4  vertexColor;
in vec2  TextureCoordinate;
in float Ks;
in float Kd;
// Premultiplied color values
uniform vec4 AmbientProduct, DiffuseProduct, SpecularProduct; 

out vec4 fragColor;

uniform sampler2D ourTexture;
uniform sampler2DArray TextureSampler;

void main() 
{
  
  vec4 SpecialSample = texture(ourTexture, TextureCoordinate);
  if(SpecialSample.a < 0.1)
    discard;

  //fragColor = Kd*(DiffuseProduct + Ks*SpecularProduct);
  fragColor = SpecialSample * Kd*(DiffuseProduct + Ks*SpecularProduct);
  fragColor.w = SpecialSample.w;

  //fragColor = SpecialSample;
}
)FOO";
  
  opengl_program Result = {};
  Result.Program = OpenGLCreateProgram( VertexShaderCode, FragmentShaderCode );
  glUseProgram(Result.Program);
  
  Result.ModelMat         = glGetUniformLocation(Result.Program, "ModelMat");
  Result.NormalMat        = glGetUniformLocation(Result.Program, "NormalMat");
  Result.TextureMat       = glGetUniformLocation(Result.Program, "TextureMat");
  Result.ProjectionMat    = glGetUniformLocation(Result.Program, "ProjectionMat");
  Result.ViewMat          = glGetUniformLocation(Result.Program, "ViewMat");
  Result.AmbientProduct   = glGetUniformLocation(Result.Program, "AmbientProduct");
  Result.DiffuseProduct   = glGetUniformLocation(Result.Program, "DiffuseProduct");
  Result.SpecularProduct  = glGetUniformLocation(Result.Program, "SpecularProduct");
  Result.LightPosition    = glGetUniformLocation(Result.Program, "LightPosition");
  Result.CameraPosition   = glGetUniformLocation(Result.Program, "CameraPosition");
  Result.Shininess        = glGetUniformLocation(Result.Program, "Shininess");
  
  glUseProgram(0);
  
  return  Result;
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
//  Platform.DEBUGPrint("%s\n", Result.Extensions);
  while( ExtensionStart )
  {
    u64 ExtensionStringLength =  ExtensionEnd ? (u64) (ExtensionEnd - ExtensionStart) : (u64) (str::StringLength(ExtensionStart));
    
    if( str::Contains( str::StringLength( "GL_ARB_debug_output" ), "GL_ARB_debug_output",
                      ExtensionStringLength, ExtensionStart ) )
    {
      int a = 10;
    }

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

opengl_info OpenGLInitExtensions()
{
  opengl_info Info = OpenGLGetExtensions();
  if(Info.EXT_texture_sRGB_decode)
  {
    //OpenGLDefaultInternalTextureFormat = GL_SRGB8_ALPHA8;
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
  
  if(Info.GL_blend_func_separate)
  {
    // Not sure I want this
    //glEnable(GL_BLEND);
  }
  return Info;
}

void ErrorCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
{
  Platform.DEBUGPrint("Source: %d, Type: %d, ID: %d, Severity %d, %s\n",source, type, id, severity, message);
}

void InitOpenGL(open_gl* OpenGL)
{
  OpenGL->Info = OpenGLInitExtensions();
  OpenGL->DefaultInternalTextureFormat = GL_RGBA8;
  OpenGL->DefaultTextureFormat = GL_RGBA;
  OpenGL->DefaultTextureFormat = GL_BGRA_EXT;
  OpenGL->MaxTextureCount = NORMAL_TEXTURE_COUNT;
  OpenGL->MaxSpecialTextureCount = SPECIAL_TEXTURE_COUNT;
  OpenGL->PhongShadingProgram = OpenGLCreateProgram3D();
  OpenGL->QuadOverlayProgram = OpenGLCreateUntexturedQuadOverlayQuadProgram();
  OpenGL->TextOverlayProgram = OpenGLCreateTextProgram();
  OpenGL->TexturedQuadOverlayProgram = OpenGLCreateTexturedQuadProgram();
  
  OpenGL->BufferSize = Megabytes(1);
  
  // Enable 2D Textures
  glEnable(GL_TEXTURE_2D);
  // Enable 3D Textures
  glEnable(GL_TEXTURE_3D);
  
  ///
  
  OpenGL->SignleWhitePixelTexture = 0;
  glGenTextures(1, &OpenGL->SignleWhitePixelTexture);
  glBindTexture(GL_TEXTURE_2D, OpenGL->SignleWhitePixelTexture);
  u8 WhitePixel[4] = {0xFF,0xFF,0xFF,0xFF};
  glTexImage2D(GL_TEXTURE_2D, 0, OpenGL->DefaultInternalTextureFormat, 1, 1, 0, OpenGL->DefaultTextureFormat, GL_UNSIGNED_BYTE, WhitePixel);
  
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT );
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT );
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
  
  ////
  
  glGenTextures(OpenGL->MaxSpecialTextureCount, OpenGL->SpecialTextures);
  for (int i = 0; i < SPECIAL_TEXTURE_COUNT; ++i)
  {
    glBindTexture(GL_TEXTURE_2D, OpenGL->SpecialTextures[i]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
  }
  
  ////
  
  glGenTextures(1, &OpenGL->TextureArray);
  glBindTexture(GL_TEXTURE_2D_ARRAY, OpenGL->TextureArray);
  
  glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
  
  b32 highesMipLevelReached = false;
  
  u32 ImageWidth = TEXTURE_ARRAY_DIM;
  u32 ImageHeight = TEXTURE_ARRAY_DIM;
  
#if 0
  // Using MipMaps
  u32 mipLevel = 0;
  while(!highesMipLevelReached)
  {
    glTexImage3D(GL_TEXTURE_2D_ARRAY, mipLevel, OpenGL->DefaultInternalTextureFormat,
                 ImageWidth, ImageHeight, OpenGL->MaxTextureCount, 0,
                 OpenGL->DefaultTextureFormat, GL_UNSIGNED_BYTE, 0);
    
    if((ImageWidth == 1) && (ImageHeight == 1))
    {
      highesMipLevelReached = true;
      ImageWidth  = 0;
      ImageHeight = 0;
    }
    mipLevel++;
    ImageWidth =  (ImageWidth>1)  ?  (ImageWidth+1)/2  : ImageWidth;
    ImageHeight = (ImageHeight>1) ?  (ImageHeight+1)/2 : ImageHeight;
  }
#else
  // Not using mipmaps until we need it
  glTexImage3D(GL_TEXTURE_2D_ARRAY,
               0,
               OpenGL->DefaultInternalTextureFormat,
               ImageWidth, ImageHeight, OpenGL->MaxTextureCount, 0,
               OpenGL->DefaultTextureFormat, GL_UNSIGNED_BYTE, 0);
#endif
  
  glGenBuffers(1, &OpenGL->ElementVBO);
  glBindBuffer(GL_ARRAY_BUFFER, OpenGL->ElementVBO);
  glBufferData(GL_ARRAY_BUFFER, OpenGL->BufferSize, 0, GL_DYNAMIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  
  glGenBuffers(1, &OpenGL->ElementEBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, OpenGL->ElementEBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, OpenGL->BufferSize, 0, GL_DYNAMIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  
  glGenBuffers(1, &OpenGL->InstanceVBO);
  glBindBuffer(GL_ARRAY_BUFFER, OpenGL->InstanceVBO);
  glBufferData(GL_ARRAY_BUFFER, OpenGL->BufferSize, 0, GL_STREAM_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  
  // Gen and Bind VAO Containing opengl_vertex
  // The generated and bound VBOs will be implicitly attached to the VAO at the glVertexAttribPointer call
  glGenVertexArrays(1, &OpenGL->ElementVAO);
  glBindVertexArray(OpenGL->ElementVAO);
  
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, OpenGL->ElementEBO); // Does affect the VAO
  
  glBindBuffer( GL_ARRAY_BUFFER, OpenGL->ElementVBO);
  // These affect the VAO
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(opengl_vertex), (GLvoid*) OffsetOf(opengl_vertex, v));
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(opengl_vertex), (GLvoid*) OffsetOf(opengl_vertex, vn) );
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(opengl_vertex), (GLvoid*) OffsetOf(opengl_vertex, vt));
  glBindBuffer( GL_ARRAY_BUFFER, 0); // Does NOT affect the VAO
  
  glBindVertexArray(0);

  OpenGL->QuadBaseOffset = 0;
  OpenGL->TextBaseOffset = OpenGL->BufferSize/3;
  OpenGL->OverlayTexQuadBaseOffset = (2*OpenGL->BufferSize)/3;

  glGenVertexArrays(1, &OpenGL->QuadVAO);
  glBindVertexArray(OpenGL->QuadVAO);
  
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, OpenGL->ElementEBO);
  
  // The quadVAO now implicitly binds these parameters to whatever VBO is currently bound (ObjectKeeper->VBO)
  glBindBuffer(GL_ARRAY_BUFFER, OpenGL->ElementVBO);
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(opengl_vertex), (GLvoid*) OffsetOf(opengl_vertex, v));
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(opengl_vertex), (GLvoid*) OffsetOf(opengl_vertex, vn));
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(opengl_vertex), (GLvoid*) OffsetOf(opengl_vertex, vt));
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  
  glBindBuffer(GL_ARRAY_BUFFER, OpenGL->InstanceVBO);
  // The quadVAO now implicitly binds these parameters to whatever VBO is currently bound (quadInstanceVBO)
  glEnableVertexAttribArray(3);
  glEnableVertexAttribArray(4);
  glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(overlay_quad_data), (void *)(OpenGL->QuadBaseOffset + OffsetOf(overlay_quad_data,QuadRect)));
  glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(overlay_quad_data), (void *)(OpenGL->QuadBaseOffset + OffsetOf(overlay_quad_data,Color)));
  glVertexAttribDivisor(3, 1);
  glVertexAttribDivisor(4, 1);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  ////

  glGenVertexArrays(1, &OpenGL->TextVAO);
  glBindVertexArray(OpenGL->TextVAO);
  
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, OpenGL->ElementEBO);
  
  glBindBuffer(GL_ARRAY_BUFFER, OpenGL->ElementVBO);
  // The textVAO now implicitly binds these parameters to whatever VBO is currently bound (ObjectKeeper->VBO)
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(opengl_vertex), (GLvoid*) OffsetOf(opengl_vertex, v));
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(opengl_vertex), (GLvoid*) OffsetOf(opengl_vertex, vn));
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(opengl_vertex), (GLvoid*) OffsetOf(opengl_vertex, vt));
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  
  glBindBuffer(GL_ARRAY_BUFFER, OpenGL->InstanceVBO);
  // The textVAO now implicitly binds these parameters to whatever VBO is currently bound (quadInstanceVBO)
  glEnableVertexAttribArray(3);
  glEnableVertexAttribArray(4);
  glEnableVertexAttribArray(5);
  glEnableVertexAttribArray(6);
  glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(text_data), (void *)(OpenGL->TextBaseOffset + OffsetOf(text_data,TextureSlot)));
  glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(text_data), (void *)(OpenGL->TextBaseOffset + OffsetOf(text_data,QuadRect)));
  glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(text_data), (void *)(OpenGL->TextBaseOffset + OffsetOf(text_data,UVRect)));
  glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(text_data), (void *)(OpenGL->TextBaseOffset + OffsetOf(text_data,Color)));
  glVertexAttribDivisor(3, 1);
  glVertexAttribDivisor(4, 1);
  glVertexAttribDivisor(5, 1);
  glVertexAttribDivisor(6, 1);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  
  glBindVertexArray(0);

#if 1
  glGenVertexArrays(1, &OpenGL->TexQuadVAO);
  glBindVertexArray(OpenGL->TexQuadVAO);
  
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, OpenGL->ElementEBO);
  
  // The quadVAO now implicitly binds these parameters to whatever VBO is currently bound (ObjectKeeper->VBO)
  glBindBuffer(GL_ARRAY_BUFFER, OpenGL->ElementVBO);
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(opengl_vertex), (GLvoid*) OffsetOf(opengl_vertex, v));
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(opengl_vertex), (GLvoid*) OffsetOf(opengl_vertex, vn));
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(opengl_vertex), (GLvoid*) OffsetOf(opengl_vertex, vt));
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  
  glBindBuffer(GL_ARRAY_BUFFER, OpenGL->InstanceVBO);
  // The quadVAO now implicitly binds these parameters to whatever VBO is currently bound (quadInstanceVBO)
  glEnableVertexAttribArray(3);
  glEnableVertexAttribArray(4);
  glEnableVertexAttribArray(5);
  glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(textured_overlay_quad_data), (void *)(OpenGL->OverlayTexQuadBaseOffset + OffsetOf(textured_overlay_quad_data, TextureSlot)));
  glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(textured_overlay_quad_data), (void *)(OpenGL->OverlayTexQuadBaseOffset + OffsetOf(textured_overlay_quad_data, QuadRect)));
  glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(textured_overlay_quad_data), (void *)(OpenGL->OverlayTexQuadBaseOffset + OffsetOf(textured_overlay_quad_data, UVRect)));
  glVertexAttribDivisor(3, 1);
  glVertexAttribDivisor(4, 1);
  glVertexAttribDivisor(5, 1);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
#endif

#if HANDMADE_INTERNAL
  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallbackARB(ErrorCallback, 0);
#endif
  
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

internal inline v4
Blend(v4* A, v4* B)
{
  v4 Result =  V4( A->X * B->X,
                  A->Y * B->Y,
                  A->Z * B->Z,
                  A->W * B->W );
  return Result;
}

b32 CompareU32Triplet( const u8* DataA, const  u8* DataB)
{
  u32* U32A = (u32*) DataA;
  const u32 A1 = *(U32A+0);
  const u32 A2 = *(U32A+1);
  const u32 A3 = *(U32A+2);
  
  u32* U32B = (u32*) DataB;
  const u32 B1 = *(U32B+0);
  const u32 B2 = *(U32B+1);
  const u32 B3 = *(U32B+2);
  
  return (A1 == B1) && (A2 == B2) && (A3 == B3);
}
b32 CompareU32(u8* DataA, u8* DataB)
{
  const u32 U32A = *( (u32*) DataA);
  const u32 U32B = *( (u32*) DataB);
  return (U32A == U32B);
}

u32 PushUnique( u8* Array, const u32 ElementCount, const u32 ElementByteSize,
               u8* NewElement, b32 (*CompareFunction)(const u8* DataA, const u8* DataB))
{
  for( u32 i = 0; i < ElementCount; ++i )
  {
    if( CompareFunction(NewElement, Array) )
    {
      return i;
    }
    Array += ElementByteSize;
  }
  
  // If we didn't find the element we push it to the end
  utils::Copy(ElementByteSize, NewElement, Array);
  
  return ElementCount;
}

struct gl_vertex_buffer
{
  u32 IndexCount;
  u32* Indeces;
  u32 VertexCount;
  opengl_vertex* VertexData;
};

internal gl_vertex_buffer
CreateGLVertexBuffer( memory_arena* TemporaryMemory,
                     const u32 IndexCount,
                     const u32* VerticeIndeces, const u32* TextureIndeces, const u32* NormalIndeces,
                     const v3* VerticeData,     const v2* TextureData,     const v3* NormalData)
{
  Assert(VerticeIndeces && VerticeData);
  u32* GLVerticeIndexArray  = PushArray(TemporaryMemory, 3*IndexCount, u32);
  u32* GLIndexArray         = PushArray(TemporaryMemory, IndexCount, u32);
  
  u32 VerticeArrayCount = 0;
  for( u32 i = 0; i < IndexCount; ++i )
  {
    const u32 vidx = VerticeIndeces[i];
    const u32 tidx = TextureIndeces ? TextureIndeces[i] : 0;
    const u32 nidx = NormalIndeces  ? NormalIndeces[i]  : 0;
    u32 NewElement[3] = {vidx, tidx, nidx};
    u32 Index = PushUnique((u8*)GLVerticeIndexArray, VerticeArrayCount, sizeof(NewElement), (u8*) NewElement,
                           [](const u8* DataA, const u8* DataB) {
                             u32* U32A = (u32*) DataA;
                             const u32 A1 = *(U32A+0);
                             const u32 A2 = *(U32A+1);
                             const u32 A3 = *(U32A+2);
                             u32* U32B = (u32*) DataB;
                             const u32 B1 = *(U32B+0);
                             const u32 B2 = *(U32B+1);
                             const u32 B3 = *(U32B+2);
                             b32 result = (A1 == B1) && (A2 == B2) && (A3 == B3);
                             return result;
                           });
    if(Index == VerticeArrayCount)
    {
      VerticeArrayCount++;
    }
    
  	GLIndexArray[i] = Index;
  }
  
  opengl_vertex* VertexData = PushArray(TemporaryMemory, VerticeArrayCount, opengl_vertex);
  opengl_vertex* Vertice = VertexData;
  for( u32 i = 0; i < VerticeArrayCount; ++i )
  {
    const u32 vidx = *(GLVerticeIndexArray + 3 * i + 0);
    const u32 tidx = *(GLVerticeIndexArray + 3 * i + 1);
    const u32 nidx = *(GLVerticeIndexArray + 3 * i + 2);
    Vertice->v  = VerticeData[vidx];
    Vertice->vt = TextureData ? TextureData[tidx] : V2(0,0);
    Vertice->vn = NormalData  ? NormalData[nidx]  : V3(0,0,0);
    ++Vertice;
  }
  
  gl_vertex_buffer Result = {};
  Result.IndexCount = IndexCount;
  Result.Indeces = GLIndexArray;
  Result.VertexCount = VerticeArrayCount;
  Result.VertexData = VertexData;
  return Result;
}

internal void setOpenGLState(u32 State)
{
  if(State & RENDER_STATE_CULL_BACK)
  {
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
  }else{
    glDisable(GL_CULL_FACE);
  }
  
  // TODO: We would like to be able to draw a filled mesh with wireframe ontop
  //       At the moment its not supported
  if(State & RENDER_STATE_POINTS)
  {
    glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
  }else if(State & RENDER_STATE_WIREFRAME)
  {
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  }else if(State & RENDER_STATE_FILL)
  {
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  }
}

v3 GetPositionFromMatrix( const m4* M )
{
  m4 inv = RigidInverse(*M);
  return V3(Column(inv,3));
}

void PushObjectToGPU(open_gl* OpenGL, game_asset_manager* AssetManager, object_handle ObjectHandle)
{ 
  buffer_keeper* ObjectKeeper = 0;
  mesh_indeces* Object = GetAsset(AssetManager, ObjectHandle, &ObjectKeeper);
  
  mesh_data* MeshData = GetAsset(AssetManager, Object->MeshHandle);
  
  temporary_memory TempMem = BeginTemporaryMemory(&AssetManager->AssetArena);
  
  Assert(Object->Count && Object->vi && Object->ti && Object->ni && MeshData->v && MeshData->vt && MeshData->vn);
  
  // Each object within a mesh gets a copy of the mesh, not good.
  gl_vertex_buffer GLBuffer =  CreateGLVertexBuffer(&AssetManager->AssetArena, 
                                                    Object->Count,
                                                    Object->vi,
                                                    Object->ti,
                                                    Object->ni,
                                                    MeshData->v,
                                                    MeshData->vt,
                                                    MeshData->vn);
  
  
  
  u32 VBOOffset = OpenGL->ElementVBOOffset;
  u32 VBOSize = GLBuffer.VertexCount * sizeof(opengl_vertex);
  OpenGL->ElementVBOOffset += VBOSize;
  Assert(OpenGL->ElementVBOOffset < OpenGL->BufferSize );
  
  glBindBuffer(GL_ARRAY_BUFFER, OpenGL->ElementVBO);
  glBufferSubData( GL_ARRAY_BUFFER,                 // Target
                  VBOOffset,                       // Offset
                  VBOSize,                         // Size
                  (GLvoid*) GLBuffer.VertexData);  // Data
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  
  u32 EBOOffset = OpenGL->ElementEBOOffset;
  u32 EBOSize   = GLBuffer.IndexCount * sizeof(u32);
  OpenGL->ElementEBOOffset += EBOSize;
  Assert(OpenGL->ElementEBOOffset < OpenGL->BufferSize );
  
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, OpenGL->ElementEBO);
  glBufferSubData( GL_ELEMENT_ARRAY_BUFFER,      // Target
                  EBOOffset,                    // Offset
                  EBOSize,                      // Size
                  (GLvoid*) GLBuffer.Indeces);  // Data
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  
  Assert((OpenGL->ElementEBOOffset + EBOSize) < OpenGL->BufferSize);
  
  Assert(ObjectKeeper->Referenced);
  ObjectKeeper->Count = GLBuffer.IndexCount;
  ObjectKeeper->Index = ((u8*) 0 + EBOOffset);
  ObjectKeeper->VertexOffset = VBOOffset/sizeof(opengl_vertex);
  
  EndTemporaryMemory(TempMem);
}

void CopyBitmapSubregion( u32 X, u32 Y, u32 Width, u32 Height, u32 Stride, u32* SrcPixels, u32* DstPixels )
{
  u32 PixelCount = Width*Height;
  u32 Xp = X;
  u32 Xf = X+Width;
  u32 Yp = Y;

  u32* DstEndPixel = DstPixels + PixelCount;
  u32* DstPixel = DstPixels;
  while(DstPixel < DstEndPixel)
  {
    u32 SrcPixelIndex = Yp * Stride + Xp++;
    u32* SrcPixel = SrcPixels + SrcPixelIndex;
    if(Xp >= Xf)
    {
      Xp = X;
      Yp++;
    }
    u32 SrcPixelValue = *SrcPixel;
    *DstPixel++ = SrcPixelValue;
  }
}

void PushBitmapToGPU(open_gl* OpenGL, game_asset_manager* AssetManager, bitmap_handle BitmapHandle)
{
  bitmap_keeper* BitmapKeeper = 0;
  bitmap* RenderTarget = GetAsset(AssetManager, BitmapHandle, &BitmapKeeper);
  
  Assert(RenderTarget->BPP == 32);
  
  if(!RenderTarget->Special)
  {
    BitmapKeeper->Special = false;
    if(!BitmapKeeper->TextureSlot)
    {
      glBindTexture( GL_TEXTURE_2D_ARRAY, OpenGL->TextureArray);
      u32 MipLevel = 0;
      BitmapKeeper->TextureSlot = OpenGL->TextureCount++;
      Assert(OpenGL->TextureCount < OpenGL->MaxTextureCount);
      
      glTexSubImage3D(GL_TEXTURE_2D_ARRAY,
                      MipLevel,
                      0, 0, BitmapKeeper->TextureSlot, // x0,y0,TextureSlot
                      RenderTarget->Width, RenderTarget->Height, 1,
                      OpenGL->DefaultTextureFormat, GL_UNSIGNED_BYTE, RenderTarget->Pixels);
    }else{
      glBindTexture( GL_TEXTURE_2D_ARRAY, OpenGL->TextureArray);
      u32 MipLevel = 0;

      if(BitmapKeeper->UseSubRegion)
      {
        ScopedMemory M = ScopedMemory(&AssetManager->AssetArena);
        u32 X = (u32) BitmapKeeper->SubRegion.X;
        u32 Y = (u32) BitmapKeeper->SubRegion.Y;
        u32 W = (u32) BitmapKeeper->SubRegion.W;
        u32 H = (u32) BitmapKeeper->SubRegion.H;
        midx PixelCount = W * H;
        u32* Pixels = PushArray(&AssetManager->AssetArena, PixelCount, u32);

        CopyBitmapSubregion(X, Y, W, H, RenderTarget->Width, (u32*) RenderTarget->Pixels, Pixels);

        glTexSubImage3D(GL_TEXTURE_2D_ARRAY,
                        MipLevel,
                        X, Y, BitmapKeeper->TextureSlot, // x0,y0,TextureSlot
                        W, H, 1,
                        OpenGL->DefaultTextureFormat, GL_UNSIGNED_BYTE, Pixels);
      }else{

        glTexSubImage3D(GL_TEXTURE_2D_ARRAY,
                        MipLevel,
                        0, 0, BitmapKeeper->TextureSlot, // x0,y0,TextureSlot
                        RenderTarget->Width, RenderTarget->Height, 1,
                        OpenGL->DefaultTextureFormat, GL_UNSIGNED_BYTE, RenderTarget->Pixels);
      }
    }
  }else{
    BitmapKeeper->Special = true;
    if(!BitmapKeeper->TextureSlot)
    {
      BitmapKeeper->TextureSlot = OpenGL->SpecialTextureCount++;
      Assert(OpenGL->SpecialTextureCount < OpenGL->MaxSpecialTextureCount);
      glBindTexture( GL_TEXTURE_2D, OpenGL->SpecialTextures[BitmapKeeper->TextureSlot] );
      glTexImage2D( GL_TEXTURE_2D,  0, GL_RGBA8,
                   RenderTarget->Width,  RenderTarget->Height,
                   0, GL_BGRA_EXT, GL_UNSIGNED_BYTE,
                   RenderTarget->Pixels);
    }else{
      Assert(OpenGL->SpecialTextureCount < OpenGL->MaxSpecialTextureCount);
      glBindTexture( GL_TEXTURE_2D, OpenGL->SpecialTextures[BitmapKeeper->TextureSlot] );


      if(BitmapKeeper->UseSubRegion)
      {
        ScopedMemory M = ScopedMemory(&AssetManager->AssetArena);
        u32 X = (u32) BitmapKeeper->SubRegion.X;
        u32 Y = (u32) BitmapKeeper->SubRegion.Y;
        u32 W = (u32) BitmapKeeper->SubRegion.W;
        u32 H = (u32) BitmapKeeper->SubRegion.H;
        midx PixelCount = W * H;
        u32* Pixels = PushArray(&AssetManager->AssetArena, PixelCount, u32);

        CopyBitmapSubregion(X, Y, W, H, RenderTarget->Width, (u32*) RenderTarget->Pixels, Pixels);

        glTexSubImage2D ( GL_TEXTURE_2D,  0,
                   X,Y,W,H,
                   GL_RGBA, GL_UNSIGNED_BYTE,
                   Pixels);
      }else{
        glTexSubImage2D ( GL_TEXTURE_2D,  0, 
                   0,
                   0,
                   RenderTarget->Width,
                   RenderTarget->Height,
                   GL_RGBA, GL_UNSIGNED_BYTE,
                   RenderTarget->Pixels);
      }

    }
    
    
  }
  
}

void OpenGLRenderGroupToOutput(game_render_commands* Commands)
{  

  TIMED_FUNCTION();
  render_group* RenderGroup = Commands->RenderGroup;
  render_group* LightsGroup = Commands->LightsGroup;
  game_asset_manager* AssetManager = Commands->AssetManager;
  open_gl* OpenGL = &Commands->OpenGL;
  
  for (u32 i = 0; i < AssetManager->ObjectPendingLoadCount; ++i)
  {
    // PushObject
    PushObjectToGPU(OpenGL, AssetManager, AssetManager->ObjectPendingLoad[i]);
  }
  AssetManager->ObjectPendingLoadCount = 0;
  
  for (u32 i = 0; i < AssetManager->BitmapPendingLoadCount; ++i)
  {
    // PushObject
    PushBitmapToGPU(OpenGL, AssetManager, AssetManager->BitmapPendingLoad[i]);
  }
  AssetManager->BitmapPendingLoadCount = 0;
  
  r32 R = 0x1E / (r32) 0xFF;
  r32 G = 0x46 / (r32) 0xFF;
  r32 B = 0x5A / (r32) 0xFF;
  glClearColor(R,G,B, 1.f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  
  glEnable(GL_BLEND);
  //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
  
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  
  // TODO: Screen Width is now also used for the resolution. Should we decouple ScreenHeightPixels and ResolutionHeightPixels?
  r32 DesiredAspectRatio = 1.77968526f;
  DesiredAspectRatio = (r32)Commands->ScreenWidthPixels / (r32)Commands->ScreenHeightPixels;
  OpenGLSetViewport( DesiredAspectRatio, Commands->ScreenWidthPixels,  Commands->ScreenHeightPixels );
  glActiveTexture(GL_TEXTURE0);
  m4 Identity = M4Identity();
  opengl_program PhongShadingProgram = Commands->OpenGL.PhongShadingProgram;
  glUseProgram( PhongShadingProgram.Program);
  glBindTexture( GL_TEXTURE_2D_ARRAY, OpenGL->TextureArray);
  
  glUniformMatrix4fv(PhongShadingProgram.ProjectionMat, 1, GL_TRUE, RenderGroup->ProjectionMatrix.E);
  glUniformMatrix4fv(PhongShadingProgram.ViewMat, 1, GL_TRUE, RenderGroup->ViewMatrix.E);
  
  v3 CameraPosition = GetPositionFromMatrix(&RenderGroup->ViewMatrix);
  glUniform4fv(PhongShadingProgram.CameraPosition, 1, V4(CameraPosition,1).E);
  v4 LightColor    = V4(0,0,0,1);
  
  // For each render group
  for( push_buffer_header* Entry = LightsGroup->First; Entry != 0; Entry = Entry->Next )
  {
    u8* Head = (u8*) Entry;
    u8* Body = Head + sizeof(push_buffer_header);
    setOpenGLState(Entry->RenderState);
    switch(Entry->Type)
    {
      // Todo: Move to a separate light cueue
      case render_buffer_entry_type::LIGHT:
      {
        entry_type_light* Light = (entry_type_light*) Body;
        glUniform4fv(PhongShadingProgram.LightPosition, 1, (Light->M*V4(0,0,0,1)).E);
        LightColor = Light->Color;
      }break;
    }
  }
  
  for( push_buffer_header* Entry = RenderGroup->First; Entry != 0; Entry = Entry->Next )
  {
    u8* Head = (u8*) Entry;
    u8* Body = Head + sizeof(push_buffer_header);
    setOpenGLState(Entry->RenderState);
    switch(Entry->Type)
    {
      case render_buffer_entry_type::RENDER_ASSET:
      {
        entry_type_render_asset* AssetTest = (entry_type_render_asset*) Body;
        
        glUniformMatrix4fv(PhongShadingProgram.ModelMat, 1, GL_TRUE, AssetTest->M.E);
        glUniformMatrix4fv(PhongShadingProgram.NormalMat, 1, GL_TRUE, AssetTest->NM.E);
        glUniformMatrix4fv(PhongShadingProgram.TextureMat, 1, GL_TRUE, AssetTest->TM.E);
        
        material* Material = GetAsset(AssetManager, AssetTest->Material);
        u32 SurfaceSmoothness = 3;
        v4 AmbientColor = Blend(&LightColor, &Material->AmbientColor);
        AmbientColor.W = 1;
        v4 DiffuseColor = Blend(&LightColor, &Material->DiffuseColor) * (1.f / 3.1415f);
        DiffuseColor.W = 1;
        v4 SpecularColor = Blend(&LightColor, &Material->SpecularColor) * ( SurfaceSmoothness + 8.f ) / (8.f*3.1415f);
        SpecularColor.W = 1;

        glUniform4fv(PhongShadingProgram.AmbientProduct,  1, AmbientColor.E);
        glUniform4fv(PhongShadingProgram.DiffuseProduct,  1, DiffuseColor.E);
        glUniform4fv(PhongShadingProgram.SpecularProduct, 1, SpecularColor.E);
        glUniform1f(PhongShadingProgram.Shininess,  Material->Shininess);
        
        bitmap_keeper* BitmapKeeper = 0;
        GetAsset(AssetManager, AssetTest->Bitmap, &BitmapKeeper);
        
        if(BitmapKeeper->Special)
        {
          u32 TextureSlotIndex = BitmapKeeper->TextureSlot;
          u32 TextureSlot = OpenGL->SpecialTextures[TextureSlotIndex];
          glBindTexture( GL_TEXTURE_2D, OpenGL->SpecialTextures[BitmapKeeper->TextureSlot]);
        }
        
        buffer_keeper* ObjectKeeper = 0;
        mesh_indeces* Object = GetAsset(AssetManager, AssetTest->Object, &ObjectKeeper);
        
        // TODO: Use Unsinged Short
        glBindVertexArray( OpenGL->ElementVAO );
        glDrawElementsBaseVertex( GL_TRIANGLES, ObjectKeeper->Count, GL_UNSIGNED_INT,
                                 (GLvoid*)(ObjectKeeper->Index),
                                 ObjectKeeper->VertexOffset);
        glBindVertexArray(0);
      }break;
    }
  }
  
  RenderGroup = Commands->OverlayGroup;
  
  if( !RenderGroup->First) {return;}
  
  temporary_memory TempMem = BeginTemporaryMemory(&RenderGroup->Arena);
  
  {
    push_buffer_header* StartEntry = RenderGroup->First;
    push_buffer_header* BreakEntry = 0;
    while(StartEntry)
    {
      u32 TextEntryCount = 0;
      u32 OverlayQuadEntryCount = 0;
      u32 OverlayTexturedQuadEntryCount = 0;
      for( push_buffer_header* Entry = StartEntry;
          Entry != 0;
          Entry = Entry->Next )
      {
        TextEntryCount                += ((Entry->Type == render_buffer_entry_type::TEXT)                  ? 1 : 0);
        OverlayQuadEntryCount         += ((Entry->Type == render_buffer_entry_type::OVERLAY_QUAD)          ? 1 : 0);
        OverlayTexturedQuadEntryCount += ((Entry->Type == render_buffer_entry_type::OVERLAY_TEXTURED_QUAD) ? 1 : 0);
        
        if(Entry->Type == render_buffer_entry_type::NEW_LEVEL)
        {
          BreakEntry = Entry;
          break;
        }
      }
      
      { // Send data to VBO
        text_data* TextBuffer = PushArray(&RenderGroup->Arena, TextEntryCount, text_data);
        overlay_quad_data* QuadBuffer = PushArray(&RenderGroup->Arena, OverlayQuadEntryCount, overlay_quad_data);
        textured_overlay_quad_data* TexQuadBuffer = PushArray(&RenderGroup->Arena, OverlayTexturedQuadEntryCount, textured_overlay_quad_data);
        u32 QuadInstnceIndex = 0;
        u32 TextInstnceIndex = 0;
        u32 TexQuadInstnceIndex = 0;
        for( push_buffer_header* Entry = StartEntry; Entry != BreakEntry; Entry = Entry->Next )
        {
          u8* Head = (u8*) Entry;
          u8* Body = Head + sizeof(push_buffer_header);
          switch(Entry->Type)
          {
            case render_buffer_entry_type::OVERLAY_QUAD:
            {
              entry_type_overlay_quad* Quad = (entry_type_overlay_quad*) Body;
              
              overlay_quad_data QuadData = {};
              QuadData.QuadRect = Quad->QuadRect;
              QuadData.Color = Quad->Colour;
              QuadBuffer[QuadInstnceIndex++] = QuadData;
            }break;
            case render_buffer_entry_type::TEXT:
            {
              text_data TexData = {};
              entry_type_text* Text = (entry_type_text*) Body;
              
              bitmap_keeper* FontKeeper;
              GetAsset(AssetManager, Text->BitmapHandle, &FontKeeper);
              
              TexData.TextureSlot = FontKeeper->TextureSlot;
              TexData.QuadRect = Text->QuadRect;
              TexData.UVRect = Text->UVRect;
              TexData.Color = Text->Colour;
              TextBuffer[TextInstnceIndex++] = TexData;
            }break;
            case render_buffer_entry_type::OVERLAY_TEXTURED_QUAD:
            {
              textured_overlay_quad_data TexQuadData = {};
              entry_type_overlay_textured_quad* TexQ = (entry_type_overlay_textured_quad*) Body;

              bitmap_keeper* FontKeeper;
              GetAsset(AssetManager, TexQ->Handle, &FontKeeper);

              TexQuadData.QuadRect = TexQ->QuadRect;
              TexQuadData.UVRect = TexQ->TextureRect;
              TexQuadData.TextureSlot = FontKeeper->TextureSlot;
              TexQuadBuffer[TexQuadInstnceIndex++] = TexQuadData;
            }
          }
        }
        
        u32 QuadBufferSize = sizeof(overlay_quad_data)*OverlayQuadEntryCount;
        u32 TextBufferSize = sizeof(text_data)*TextEntryCount;
        u32 TexQuadBufferSize = sizeof(textured_overlay_quad_data)*OverlayTexturedQuadEntryCount;

        glBindBuffer(GL_ARRAY_BUFFER, OpenGL->InstanceVBO);
        glBufferSubData( GL_ARRAY_BUFFER,        // Target
                        OpenGL->QuadBaseOffset,  // Offset
                        QuadBufferSize,          // Size
                        (GLvoid*) QuadBuffer);   // Data
        glBufferSubData( GL_ARRAY_BUFFER,        // Target
                        OpenGL->TextBaseOffset,  // Offset
                        TextBufferSize,          // Size
                        (GLvoid*) TextBuffer);   // Data
        glBufferSubData( GL_ARRAY_BUFFER,        // Target
                        OpenGL->OverlayTexQuadBaseOffset,  // Offset
                        TexQuadBufferSize,                 // Size
                        (GLvoid*) TexQuadBuffer);          // Data
        glBindBuffer(GL_ARRAY_BUFFER, 0);
      }
      
      opengl_program QuadOverlayProgram = Commands->OpenGL.QuadOverlayProgram;
      glUseProgram(QuadOverlayProgram.Program);
      glBindTexture( GL_TEXTURE_2D_ARRAY, OpenGL->TextureArray);
      glUniformMatrix4fv(QuadOverlayProgram.ProjectionMat, 1, GL_TRUE, RenderGroup->ProjectionMatrix.E);
      
      // No need to clearh the depth buffer if we disable depth test
      glDisable(GL_DEPTH_TEST);
      // Enable Textures
      glEnable(GL_TEXTURE_2D);
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      
      buffer_keeper* ElementObjectKeeper = 0;
      object_handle ObjectHandle = GetEnumeratedObjectHandle(AssetManager, predefined_mesh::QUAD);
      GetAsset(AssetManager, ObjectHandle, &ElementObjectKeeper);
      
      glBindVertexArray( OpenGL->QuadVAO );
      glDrawElementsInstancedBaseVertex(GL_TRIANGLES,                           // Mode,
                                        ElementObjectKeeper->Count,             // Nr of Elements (Triangles*3)
                                        GL_UNSIGNED_INT,                        // Index Data Type  
                                        (GLvoid*)(ElementObjectKeeper->Index),  // Pointer somewhere in the index buffer
                                        OverlayQuadEntryCount,                  // How many Instances to draw
                                        ElementObjectKeeper->VertexOffset);     // Base Offset into the geometry vbo
      glBindVertexArray(0);
      
      opengl_program TextRenderProgram = Commands->OpenGL.TextOverlayProgram;
      glUseProgram(TextRenderProgram.Program);

      //glBindTexture( GL_TEXTURE_2D_ARRAY, OpenGL->TextureArray);
      glUniformMatrix4fv(QuadOverlayProgram.ProjectionMat, 1, GL_TRUE, RenderGroup->ProjectionMatrix.E);
      
      glBindVertexArray(OpenGL->TextVAO);
      glDrawElementsInstancedBaseVertex(GL_TRIANGLES,                           // Mode,
                                        ElementObjectKeeper->Count,             // Nr of Elements (Triangles*3)
                                        GL_UNSIGNED_INT,                        // Index Data Type  
                                        (GLvoid*)(ElementObjectKeeper->Index),  // Pointer somewhere in the index buffer
                                        TextEntryCount,                         // How many Instances to draw
                                        ElementObjectKeeper->VertexOffset);     // Base Offset into the geometry vbo
      glBindVertexArray(0);


      opengl_program TexturedQuadOverlayProgram = Commands->OpenGL.TexturedQuadOverlayProgram;
      glUseProgram(TexturedQuadOverlayProgram.Program);

      //glBindTexture( GL_TEXTURE_2D_ARRAY, OpenGL->TextureArray);
      glUniformMatrix4fv(QuadOverlayProgram.ProjectionMat, 1, GL_TRUE, RenderGroup->ProjectionMatrix.E);
      
      glBindVertexArray(OpenGL->TexQuadVAO);
      glDrawElementsInstancedBaseVertex(GL_TRIANGLES,                           // Mode,
                                        ElementObjectKeeper->Count,             // Nr of Elements (Triangles*3)
                                        GL_UNSIGNED_INT,                        // Index Data Type  
                                        (GLvoid*)(ElementObjectKeeper->Index),  // Pointer somewhere in the index buffer
                                        OverlayTexturedQuadEntryCount,                    // How many Instances to draw
                                        ElementObjectKeeper->VertexOffset);     // Base Offset into the geometry vbo
      glBindVertexArray(0);
      
      
      StartEntry = 0;
      if(BreakEntry)
      {
        StartEntry = BreakEntry->Next;
        BreakEntry = 0;
      }
    }
    
  }
  
  EndTemporaryMemory(TempMem);
}