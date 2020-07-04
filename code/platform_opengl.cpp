
#include "render_push_buffer.h"
#include "math/affine_transformations.h"
#include "bitmap.h"
#include "assets.cpp"
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
    char* Tmp      = (char*) VirtualAlloc(0, CompileMessageSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    glGetShaderInfoLog(ShaderHandle, CompileMessageSize, NULL, (GLchar*) CompileMessage);
    snprintf((char*)Tmp, CompileMessageSize, "%s", CompileMessage);
    //VirtualFree(CompileMessage, 0, MEM_RELEASE );
    //VirtualFree(Tmp, 0, MEM_RELEASE );
    OutputDebugStringA(CompileMessage);
    //Assert(0);
    exit(1);
  }
  #endif

  return ShaderHandle;
};

GLuint OpenGLCreateProgram( char* VertexShaderCode, char* FragmentShaderCode )
{
  GLuint VertexShader   = OpenGLLoadShader( VertexShaderCode,   GL_VERTEX_SHADER   );
  GLuint FragmentShader = OpenGLLoadShader( FragmentShaderCode, GL_FRAGMENT_SHADER );

  GLuint Program = glCreateProgram();

  // Attatch the shader to the program
  glAttachShader(Program, VertexShader);
  glAttachShader(Program, FragmentShader);
  glLinkProgram(Program);

  glDetachShader(Program, VertexShader);
  glDetachShader(Program, FragmentShader);

  glDeleteShader(VertexShader);
  glDeleteShader(FragmentShader);

  GLint Linked;
  glGetProgramiv(Program, GL_LINK_STATUS, &Linked);
  if( !Linked )
  {
    char* ProgramMessage;
    GLint ProgramMessageSize;
    glGetProgramiv( Program, GL_INFO_LOG_LENGTH, &ProgramMessageSize );

    ProgramMessage = (char*) VirtualAlloc(0, ProgramMessageSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    char* Tmp      = (char*) VirtualAlloc(0, ProgramMessageSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    glGetProgramInfoLog(Program, ProgramMessageSize, NULL, (GLchar*) ProgramMessage);
    snprintf((char*)Tmp, ProgramMessageSize, "%s",(char*) ProgramMessage);
    OutputDebugStringA(ProgramMessage);
    //VirtualFree(ProgramMessage, 0, MEM_RELEASE );
    //VirtualFree(Tmp, 0, MEM_RELEASE );
    exit(1);
  }

  return Program;
}

const c8* GetUniformName(open_gl_uniform Enum)
{
  local_persist c8* UniformLibrary[OPEN_GL_UNIFORM_COUNT];
  UniformLibrary[(u32)open_gl_uniform::m4_Model] = "M";
  UniformLibrary[(u32)open_gl_uniform::m4_Normal] = "NM";
  UniformLibrary[(u32)open_gl_uniform::m4_Texture] = "TM";
  UniformLibrary[(u32)open_gl_uniform::m4_Projection] = "P";
  UniformLibrary[(u32)open_gl_uniform::m4_View] = "V";
  UniformLibrary[(u32)open_gl_uniform::v4_AmbientProduct] = "ambientProduct";
  UniformLibrary[(u32)open_gl_uniform::v4_DiffuseProduct] = "diffuseProduct";
  UniformLibrary[(u32)open_gl_uniform::v4_SpecularProduct] = "specularProduct";
  UniformLibrary[(u32)open_gl_uniform::v4_LightPosition] = "lightPosition";
  UniformLibrary[(u32)open_gl_uniform::v4_CameraPosition] = "cameraPosition";
  UniformLibrary[(u32)open_gl_uniform::v2_TextureCoordinate] = "texCoord";
  UniformLibrary[(u32)open_gl_uniform::s_Shininess] = "shininess";
  Assert(OPEN_GL_UNIFORM_COUNT==(u8)open_gl_uniform::count);
  Assert(Enum < open_gl_uniform::count);
  return UniformLibrary[(u32)Enum];
}
// OpenGL uses Column major convention.
// Our math library uses Row major convention which means we need to transpose the
// matrices AND reverse the order of multiplication.
// Transpose(A*B) = Transpose(B) * Transpose(A)
inline internal void DeclareUniform(opengl_program* Program, open_gl_uniform Enum)
{
  const c8* Name = GetUniformName(Enum);
  Program->Uniforms[(u32)(Enum)] = glGetUniformLocation(Program->Program, Name);
 // Assert(Program->Uniforms[(u32)(Enum)] >= 0);
}

inline internal void SetUniformM4(opengl_program Program, open_gl_uniform Enum, m4 Matrix)
{
  s32 UniformID = Program.Uniforms[(u32)(Enum)];
  glUniformMatrix4fv(UniformID, 1, GL_TRUE, Matrix.E);
}

inline internal void SetUniformV4(opengl_program Program, open_gl_uniform Enum, v4 Vector)
{
  s32 UniformID = Program.Uniforms[(u32)(Enum)];
  glUniform4fv(UniformID, 1, Vector.E);
}

inline internal void SetUniformS(opengl_program Program, open_gl_uniform Enum, r32 Scalar)
{
  s32 UniformID = Program.Uniforms[(u32)(Enum)];
  glUniform1f(UniformID, Scalar);
}


opengl_program OpenGLCreateTextProgram()
{
  char VertexShaderCode[] = R"FOO(
#version  330 core
uniform mat4 P;  // Projection Matrix - Transforms points from ScreenSpace to UnitQube.
layout (location = 0) in vec3 vertice;
layout (location = 2) in vec2 textureCoordinate;
layout (location = 3) in vec4 quadRect;
layout (location = 4) in vec4 uvRect;
layout (location = 5) in vec4 color;
out vec4 vertexColor;
out vec2 texCoord;
void main()
{
  mat3 projection;
  projection[0] = vec3(P[0].x,0,0);
  projection[1] = vec3(0,P[1].y,0);
  projection[2] = vec3(P[3].x,P[3].y,1);

  mat3 quadTransform;
  quadTransform[0] = vec3(quadRect.z, 0, 0); // z=Width
  quadTransform[1] = vec3(0, quadRect.w, 0); // w=Height
  quadTransform[2] = vec3(quadRect.xy, 1);   // x,y = x,y

  mat3 uvTransform;
  uvTransform[0] = vec3(uvRect.z, 0, 0);
  uvTransform[1] = vec3(0, uvRect.w, 0);
  uvTransform[2] = vec3(uvRect.xy, 1);

  gl_Position = vec4((projection*quadTransform*vec3(vertice.xy,1)).xy,0,1);
  texCoord = (uvTransform*vec3(textureCoordinate.xy,1)).xy;
  vertexColor = color;
}
)FOO";

  char* FragmentShaderCode = R"FOO(
#version 330 core
out vec4 fragColor;
in vec4 vertexColor;
in vec2 texCoord;
uniform sampler2D ourTexture;
uniform sampler2DArray TextureSampler;
void main() 
{
  //fragColor = texture(ourTexture, texCoord) * vertexColor;
  vec3 ArrayUV = vec3(texCoord.x, texCoord.y, 0);
  fragColor = texture(TextureSampler, ArrayUV) * vertexColor;
}
)FOO";

  opengl_program Result = {};
  Result.Program = OpenGLCreateProgram( VertexShaderCode, FragmentShaderCode );
  glUseProgram(Result.Program);
  DeclareUniform(&Result, open_gl_uniform::m4_Projection);
  glUseProgram(0);

  return Result;
}

// TODO, merge this shader and OpenGLCreateTextProgram when we support 3D textures and can index the 
//       white texture and the font map and send the texture idx in the instancing array
opengl_program OpenGLCreateUntexturedQuadOverlayQuadProgram()
{
  char VertexShaderCode[] = R"FOO(
#version  330 core
uniform mat4 P;  // Projection Matrix - Transforms points from ScreenSpace to UnitQube.
layout (location = 0) in vec3 vertice;
layout (location = 3) in vec4 quadRect;
layout (location = 4) in vec4 color;
out vec4 vertexColor;
void main()
{
  mat3 projection;
  projection[0] = vec3(P[0].x,0,0);
  projection[1] = vec3(0,P[1].y,0);
  projection[2] = vec3(P[3].x,P[3].y,1);

  mat3 quadTransform;
  quadTransform[0] = vec3(quadRect.z, 0, 0); // z=Width
  quadTransform[1] = vec3(0, quadRect.w, 0); // w=Height
  quadTransform[2] = vec3(quadRect.xy, 1);   // x,y = x,y

  gl_Position = vec4((projection*quadTransform*vec3(vertice.xy,1)).xy,0,1);
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
  DeclareUniform(&Result, open_gl_uniform::m4_Projection);
  glUseProgram(0);

  return Result;
}



opengl_program OpenGLCreateProgram3D()
{
   char* VertexShaderCode =
R"FOO(
#version  330 core
layout (location = 0) in vec3 vertice;
layout (location = 1) in vec3 verticeNormal;
layout (location = 2) in vec2 textureCoordinate;

uniform mat4 M;  // Model Matrix - Transforms points from ModelSpace to WorldSpace.
                 // Includes Translation, Rotation and Scaling
uniform mat4 NM; // Normal Model Matrix = Transpose( RigidInverse(M) );
                 // Keeps normals correct under shearing scaling
uniform mat4 TM; // Texture Model Matrix. Shifts texture coordinates in a bitmap
uniform mat4 V;  // View Matrix - Camera Position and Orientation in WorldSpace
uniform mat4 P;  // Projection Matrix - Scales the visible world into the unit cube.

// Premultiplied color values
uniform vec4 ambientProduct, diffuseProduct, specularProduct; 

uniform vec4 lightPosition;  // World space
uniform vec4 cameraPosition; // World space

uniform float shininess; // Shininess of material

out vec4 vertexColor;
out vec2 texCoord;

void main()
{

// Variable Descriptions:
// N [Normal Vector], Object Space
// L [Light Vector] Unit vector of vertex to light in world space;
// E [Eye Vector]   Unit vector of vertex to camera in world space;
// H [Half Vector]  Unit vector pointing between between L and E;

  vec4 Vertice = M * vec4(vertice,1);
  vec4 N = normalize( NM * vec4(verticeNormal,0) );
  vec4 L = normalize( lightPosition - Vertice );
  float Kd = max(dot(L,N), 0.0);

  vec4 E = normalize( cameraPosition - Vertice );
  vec4 H = normalize(L+E);
  float Ks = pow(max(dot(H,N), 0.0 ), shininess);

  vertexColor = ambientProduct + Kd*(diffuseProduct + Ks*specularProduct);

  vec4 tmpTex = TM*vec4(textureCoordinate,0,1);
  texCoord = vec2(tmpTex.x,tmpTex.y);
  gl_Position = P*V*Vertice;
}
)FOO";

  char* FragmentShaderCode = R"FOO(
#version 330 core
in vec4  vertexColor;
in vec2  texCoord;
out vec4 fragColor;

uniform sampler2D ourTexture;

void main() 
{
  fragColor = texture(ourTexture, texCoord) * vertexColor;
}
)FOO";

  opengl_program Result = {};
  Result.Program = OpenGLCreateProgram( VertexShaderCode, FragmentShaderCode );
  glUseProgram(Result.Program);


  DeclareUniform(&Result, open_gl_uniform::m4_Projection);
  DeclareUniform(&Result, open_gl_uniform::m4_View);
  DeclareUniform(&Result, open_gl_uniform::m4_Model);
  DeclareUniform(&Result, open_gl_uniform::m4_Normal);
  DeclareUniform(&Result, open_gl_uniform::m4_Texture);
  DeclareUniform(&Result, open_gl_uniform::v4_AmbientProduct);
  DeclareUniform(&Result, open_gl_uniform::v4_DiffuseProduct);
  DeclareUniform(&Result, open_gl_uniform::v4_SpecularProduct);
  DeclareUniform(&Result, open_gl_uniform::v4_LightPosition);
  DeclareUniform(&Result, open_gl_uniform::v4_CameraPosition);
  DeclareUniform(&Result, open_gl_uniform::s_Shininess);

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

#define TEXTURE_ARRAY_DIM 512;
#define NORMAL_TEXTURE_COUNT 256;

void InitOpenGL(open_gl* OpenGL)
{
  OpenGL->Info = OpenGLInitExtensions();
  OpenGL->DefaultInternalTextureFormat = GL_RGBA8;
  OpenGL->DefaultTextureFormat = GL_RGBA;
  OpenGL->DefaultTextureFormat = GL_BGRA_EXT;
  OpenGL->MaxTextureCount = NORMAL_TEXTURE_COUNT;
  OpenGL->PhongShadingProgram = OpenGLCreateProgram3D();
  OpenGL->QuadOverlayProgram = OpenGLCreateUntexturedQuadOverlayQuadProgram();
  OpenGL->TextOverlayProgram = OpenGLCreateTextProgram();

  OpenGL->BufferSize = Megabytes(32);

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


    // Gen and Bind VAO Containing opengl_vertex
    // The generated and bound VBOs will be implicitly attached to the VAO at the glVertexAttribPointer call
    glGenVertexArrays(1, &OpenGL->ElementVAO);
    glBindVertexArray(OpenGL->ElementVAO);
  
    // Gen and bind a VBO to and bind to the VAO at the GL_ARRAY_BUFFER slot
    glGenBuffers(1, &OpenGL->ElementVBO);
    glBindBuffer( GL_ARRAY_BUFFER, OpenGL->ElementVBO);
    glBufferData( GL_ARRAY_BUFFER, OpenGL->BufferSize, 0, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(opengl_vertex), (GLvoid*) OffsetOf(opengl_vertex, v));  // Vertecis
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(opengl_vertex), (GLvoid*) OffsetOf(opengl_vertex, vn) );   // Normals
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(opengl_vertex), (GLvoid*) OffsetOf(opengl_vertex, vt)); // Textures

    // Same but for the EBO (No need to say how to interpret data, it's already known (indeces))
    glGenBuffers(1, &OpenGL->ElementEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, OpenGL->ElementEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, OpenGL->BufferSize, 0, GL_DYNAMIC_DRAW);

    glBindVertexArray(0);
}


void OpenGLSendMeshToGPU( u32* VAO, u32* VBO, u32* EBO, const u32 NrIndeces, const u32* IndexData, const u32 NrVertecies, const opengl_vertex* VertexData)
{
  Assert(VertexData);
  Assert(IndexData);

  u32 EffectiveEBO = 0;
  if(*VAO==0)
  {
    Assert(*VBO == 0);
    // Generate vao
    glGenVertexArrays(1, VAO);

    // Generate a generic buffer
    glGenBuffers(1, VBO);

    // TODO: Have a proper buffer object state containing VAO,VBO and EBO so we don't 'have' to query it
    if(EBO == NULL)
    {
      glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, (GLint*) &EffectiveEBO );
      if(!EffectiveEBO)
      {
        glGenBuffers(1, &EffectiveEBO);
      }
    }else{
      glGenBuffers(1, EBO);
      EffectiveEBO = *EBO;
    }
  }

  Assert(*VAO);
  Assert(*VBO);
  Assert(EffectiveEBO);

  // Bind the VBO to the current glState
  glBindVertexArray(*VAO);

  // Bind the VBO to the GL_ARRAY_BUFFER slot
  glBindBuffer( GL_ARRAY_BUFFER, *VBO);

  // Send data to it, if VBO already existed it gets reallocated
  glBufferData( GL_ARRAY_BUFFER, NrVertecies * sizeof(opengl_vertex), (GLvoid*) VertexData, GL_STATIC_DRAW);

  // Say how to interpret the data in the VBO
  // The currently bound VBO is implicitly attached to the VAO at the glVertexAttribPointer call
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(opengl_vertex), (GLvoid*) OffsetOf(opengl_vertex, v));  // Vertecis
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(opengl_vertex), (GLvoid*) OffsetOf(opengl_vertex, vn) );   // Normals
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(opengl_vertex), (GLvoid*) OffsetOf(opengl_vertex, vt)); // Textures


  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EffectiveEBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, NrIndeces * sizeof(u32), IndexData, GL_STATIC_DRAW);

  glBindVertexArray(0);
}

void OpenGLDraw( u32 VertexArrayObject, u32 ElementType, u32 nrVertecies, u32 Offset)
{
  GLenum Mode = GL_TRIANGLES;
  switch(ElementType)
  {
    case DATA_TYPE_POINT:    Mode = GL_POINTS;    break;
    case DATA_TYPE_LINE:     Mode = GL_LINES;     break;
    case DATA_TYPE_TRIANGLE: Mode = GL_TRIANGLES; break;
  }
  glBindVertexArray( VertexArrayObject );
  glDrawElements( Mode, nrVertecies, GL_UNSIGNED_INT, (GLvoid*)(sizeof(GLuint) * Offset));
  glBindVertexArray(0);
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

void PushMeshToGPU(memory_arena* TempArena,
  mesh_indeces* Object, book_keeper* ObjectKeeper, mesh_data* MeshData)
{
  if(!ObjectKeeper->Loaded)
  {
    temporary_memory TempMem = BeginTemporaryMemory(TempArena);


    u32 nvi = Object->Count;   // Nr Indeces
    u32* vi = Object->vi;      // Vertex Indeces
    u32* ti = Object->ti;      // Texture Indeces
    u32* ni = Object->ni;      // Normal Indeces

    u32 nv = MeshData->nv;    // Nr Verices
    v3* v  = MeshData->v;     // Vertices

    u32 nvn = MeshData->nvn;   // Nr Vertice Normals
    v3* vn = MeshData->vn;    // Vertice Normals

    u32 nvt = MeshData->nvt;   // Nr Trxture Vertices
    v2* vt = MeshData->vt;    // Texture Vertices

    gl_vertex_buffer GLBuffer =  CreateGLVertexBuffer(TempArena, nvi, vi, ti, ni, v, vt, vn);

    OpenGLSendMeshToGPU(&ObjectKeeper->BufferHandle.VAO,
                        &ObjectKeeper->BufferHandle.VBO,
                        &ObjectKeeper->BufferHandle.EBO,
                        GLBuffer.IndexCount, GLBuffer.Indeces,
                        GLBuffer.VertexCount, GLBuffer.VertexData);
    ObjectKeeper->Loaded = true;

    EndTemporaryMemory(TempMem);
  }
}

void BindTextureToGPU(bitmap* RenderTarget, book_keeper* BitmapKeeper)
{
  Assert(RenderTarget);
  Assert(RenderTarget->BPP == 32);
  if(!BitmapKeeper->Loaded)
  {
    Assert(!BitmapKeeper->BufferHandle.TextureHandle);
    BitmapKeeper->Loaded = true;
    // Generate a texture slot
    glGenTextures(1, &BitmapKeeper->BufferHandle.TextureHandle);

    // Enable texture slot
    glBindTexture( GL_TEXTURE_2D, BitmapKeeper->BufferHandle.TextureHandle );

    // Set texture environment state:
    // See documantation here: https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTexParameter.xhtml

    // Parameters set with glTexParameter affects the currently bound texture object,
    // and stays with the texture object until changed.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,  GL_MIRRORED_REPEAT );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,  GL_MIRRORED_REPEAT );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

    glTexImage2D( GL_TEXTURE_2D,  0, GL_RGBA8,
              RenderTarget->Width,  RenderTarget->Height,
              0, GL_BGRA_EXT, GL_UNSIGNED_BYTE,
              RenderTarget->Pixels);
  }

  glBindTexture( GL_TEXTURE_2D, BitmapKeeper->BufferHandle.TextureHandle );

}


void DrawAsset(opengl_program* Program, game_asset_manager* AssetManager, memory_arena* TempArena,
  entry_type_render_asset* RenderableAsset, v4 LightColor)
{
  SetUniformM4(*Program, open_gl_uniform::m4_Model, RenderableAsset->M);
  SetUniformM4(*Program, open_gl_uniform::m4_Normal, RenderableAsset->NM);
  SetUniformM4(*Program, open_gl_uniform::m4_Texture, RenderableAsset->TM);

  u32 AssetHandle = RenderableAsset->AssetHandle;

  book_keeper* ObjectKeeper = 0;
  mesh_indeces* Object = GetObject(AssetManager, AssetHandle, &ObjectKeeper);
  mesh_data* MeshData = GetMesh(AssetManager, AssetHandle);

  PushMeshToGPU(TempArena, Object, ObjectKeeper, MeshData);

  material* Material   = GetMaterial(AssetManager, AssetHandle);

  u32 SurfaceSmoothness = 3;
  v4 AmbientColor   = Blend(&LightColor, &Material->AmbientColor);
  AmbientColor.W = 1;
  v4 DiffuseColor   = Blend(&LightColor, &Material->DiffuseColor) * (1.f / 3.1415f);
  DiffuseColor.W = 1;
  v4 SpecularColor  = Blend(&LightColor, &Material->SpecularColor) * ( SurfaceSmoothness + 8.f ) / (8.f*3.1415f);
  SpecularColor.W = 1;
  SetUniformV4( *Program, open_gl_uniform::v4_AmbientProduct, AmbientColor);
  SetUniformV4( *Program, open_gl_uniform::v4_DiffuseProduct, DiffuseColor);
  SetUniformV4( *Program, open_gl_uniform::v4_SpecularProduct, SpecularColor);
  SetUniformS(  *Program, open_gl_uniform::s_Shininess, Material->Shininess);

  book_keeper* BitmapKeeper = 0;
  bitmap* RenderTarget = GetBitmap(AssetManager, AssetHandle, &BitmapKeeper);
  BindTextureToGPU(RenderTarget, BitmapKeeper);

  Assert(ObjectKeeper->BufferHandle.VAO);
  OpenGLDraw( ObjectKeeper->BufferHandle.VAO,  DATA_TYPE_TRIANGLE, Object->Count, 0 );
}

void OpenGLPushBufferData(memory_arena* TemporaryMemory, render_buffer* Buffer)
{
  temporary_memory TempMem = BeginTemporaryMemory(TemporaryMemory);
  if(!Buffer->Fill)
  {
    return;
  }

  gl_vertex_buffer GLBuffer =  CreateGLVertexBuffer(TemporaryMemory,
    Buffer->nvi, Buffer->vi, Buffer->ti, Buffer->ni,
    Buffer->v, Buffer->vt, Buffer->vn);

  u32* VAO = Buffer->VAO;
  u32* VBO = Buffer->VBO;
  OpenGLSendMeshToGPU(VAO, VBO, NULL, GLBuffer.IndexCount, GLBuffer.Indeces, GLBuffer.VertexCount, GLBuffer.VertexData);

  EndTemporaryMemory(TempMem);
}

internal void setOpenGLState(u32 State)
{
  if(State & RENDER_STATE_CULL_BACK)
  {
    glEnable(GL_CULL_FACE);
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


struct text_data
{
  rect2f QuadRect;
  rect2f UVRect;
  v4 Color;
};

struct overlay_quad_data
{
  rect2f QuadRect;
  v4 Color;
};
#if 0
void RenderBeginFrame(game_render_commands* Commands)
{
  game_asset_manager* AssetManager = Commands->AssetManager;
  open_gl* OpenGL = Commands->MainRenderGroup;

  for(render_bitmap* RenderBitmap = GetPendingBitmap(AssetManager);
                     RenderBitmap = Next(AssetManager, RenderBitmap)
                                    IsValid(RenderBitmap))
  {
    u32 BufferOffset = SendToGpu(OpenGL, RenderBitmap);
    FinalizeTransfer(AssetManager, RenderBitmap, BufferOffset);  
  }

  for(render_mesh* RenderMesh = GetPendingMesh(AssetManager);
                   RenderMesh = Next(AssetManager, RenderMesh)
                                IsValid(RenderMesh))
  {
    u32 BufferOffset = SendToGpu(OpenGL, RenderMesh);
    FinalizeTransfer(AssetManager, RenderMesh, BufferOffset);
  }
}
#endif


struct mesh_vertex
{
  v3 v;
  v3 n;
  v2 uv; 
};

struct textured_vertex
{
  mesh_vertex Vertex;
  r32 Color;
  u32 TextureIndex;
};


void PushObjectToGPU(open_gl* OpenGL, game_asset_manager* AssetManager, u32 Idx)
{
  u32 ObjectHandle = AssetManager->ObjectPendingLoad[Idx];
  // TODO: Now we only suport one set of indeces per mesh. We want to keep separate track of indeces and meshes
  //       Several indeces can point to one mesh
  book_keeper* ObjectKeeper = 0;
  mesh_indeces* Object = GetObjectFromIndex(AssetManager, ObjectHandle, &ObjectKeeper);
  mesh_data* MeshData = GetMesh(AssetManager, Object->MeshHandle);

  temporary_memory TempMem = BeginTemporaryMemory(&AssetManager->AssetArena);

  Assert(Object->Count && Object->vi && Object->ti && Object->ni && MeshData->v && MeshData->vt && MeshData->vn);
  gl_vertex_buffer GLBuffer =  CreateGLVertexBuffer(&AssetManager->AssetArena, 
    Object->Count,
    Object->vi,
    Object->ti,
    Object->ni,
    MeshData->v,
    MeshData->vt,
    MeshData->vn);

  glBindVertexArray(OpenGL->ElementVAO);

  u32 VBOOffset = OpenGL->ElementVBOOffset;
  u32 VBOSize = GLBuffer.VertexCount * sizeof(opengl_vertex);
  OpenGL->ElementVBOOffset += VBOSize;
  Assert(OpenGL->ElementVBOOffset < OpenGL->BufferSize );

  glBufferSubData( GL_ARRAY_BUFFER,                 // Target
                   VBOOffset,                       // Offset
                   VBOSize,                         // Size
                   (GLvoid*) GLBuffer.VertexData);  // Data
  

  u32 EBOOffset = OpenGL->ElementEBOOffset;
  u32 EBOSize   = GLBuffer.IndexCount * sizeof(u32);
  OpenGL->ElementEBOOffset += EBOSize;
  Assert(OpenGL->ElementEBOOffset < OpenGL->BufferSize );

  glBufferSubData( GL_ELEMENT_ARRAY_BUFFER,      // Target
                   EBOOffset,                    // Offset
                   EBOSize,                      // Size
                   (GLvoid*) GLBuffer.Indeces);  // Data
  Assert((OpenGL->ElementEBOOffset + EBOSize) < OpenGL->BufferSize);

  glBindVertexArray(0);

  EndTemporaryMemory(TempMem);

  ObjectKeeper->Location.Count = GLBuffer.IndexCount;
  ObjectKeeper->Location.Index = ((u8*) 0 + EBOOffset);
  ObjectKeeper->Location.VertexOffset = VBOOffset/sizeof(opengl_vertex);
  int a = 10;
}
void PushBitmapToGPU()
{

}
void OpenGLRenderGroupToOutput( game_render_commands* Commands)
{
  render_group* RenderGroup = Commands->MainRenderGroup;
  game_asset_manager* AssetManager = Commands->AssetManager;
  open_gl* OpenGL = &Commands->OpenGL;

  for (u32 i = 0; i < AssetManager->ObjectPendingLoadCount; ++i)
  {
    // PushObject
    PushObjectToGPU(OpenGL, AssetManager, i);

  }
  AssetManager->ObjectPendingLoadCount = 0;

  for (u32 i = 0; i < AssetManager->BitmapPendingLoadCount; ++i)
  {
    // PushObject
    PushBitmapToGPU();
  }
  AssetManager->BitmapPendingLoadCount = 0;
#if 0
  // Send everything to gpu
  u32 ObjectHandles[256] = {};
  u32 ObjectIndex = 0;
  u32 BitMapHandles[256] = {};
  u32 BitMapIndex = 0;
  u32 NrLines = 0;
  for( push_buffer_header* Entry = RenderGroup->First; Entry != 0; Entry = Entry->Next )
  {
    u8* Head = (u8*) Entry;
    u8* Body = Head + sizeof(push_buffer_header);
    switch(Entry->Type)
    {
      case render_buffer_entry_type::RENDER_ASSET:
      {
        // ObjectBuffer
        u32 AssetHandle = RenderableAsset->AssetHandle;
        book_keeper* ObjectKeeper = 0;
        GetObject(AssetManager, AssetHandle, &ObjectKeeper);
        if(!ObjectKeeper->Loaded)
        {
          ObjectHandles[ObjectIndex++] = AssetHandle;
        }

        book_keeper* BitmapKeeper = 0;
        GetBitmap(AssetManager, AssetHandle, &BitmapKeeper);
        if(!BitmapKeeper->Loaded)
        {
          BitMapHandles[BitMapIndex++] = AssetHandle;
        }
      }break;
      case render_buffer_entry_type::LINE:
      {
        ++NrLines;
      }break;
  }
#endif

  // Accept fragment if it closer to the camera than the former one
  // Enable depth test
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);

  r32 R = 0x1E / (r32) 0xFF;
  r32 G = 0x46 / (r32) 0xFF;
  r32 B = 0x5A / (r32) 0xFF;
  glClearColor(R,G,B, 1.f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_BLEND);
  
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // TODO: Screen Width is now also used for the resolution. Should we decouple ScreenHeightPixels and ResolutionHeightPixels?
  r32 DesiredAspectRatio = 1.77968526f;
  DesiredAspectRatio = (r32)Commands->ScreenWidthPixels / (r32)Commands->ScreenHeightPixels;
  OpenGLSetViewport( DesiredAspectRatio, Commands->ScreenWidthPixels, Commands->ScreenHeightPixels );
  glActiveTexture(GL_TEXTURE0);
  m4 Identity = M4Identity();
  opengl_program PhongShadingProgram = Commands->OpenGL.PhongShadingProgram;
  glUseProgram( PhongShadingProgram.Program);

  SetUniformM4(PhongShadingProgram, open_gl_uniform::m4_Projection, RenderGroup->ProjectionMatrix);
  SetUniformM4(PhongShadingProgram, open_gl_uniform::m4_View, RenderGroup->ViewMatrix);

  v3 CameraPosition = GetPositionFromMatrix(&RenderGroup->ViewMatrix);
  SetUniformV4( PhongShadingProgram, open_gl_uniform::v4_CameraPosition, V4(CameraPosition,1));
  v4 LightColor    = V4(0,0,0,1);

  // For each render group
  for( push_buffer_header* Entry = RenderGroup->First; Entry != 0; Entry = Entry->Next )
  {
    u8* Head = (u8*) Entry;
    u8* Body = Head + sizeof(push_buffer_header);
    setOpenGLState(Entry->RenderState);
    switch(Entry->Type)
    {
      case render_buffer_entry_type::LIGHT:
      {
        entry_type_light* Light = (entry_type_light*) Body;
        SetUniformV4(PhongShadingProgram, open_gl_uniform::v4_LightPosition, Light->M*V4(0,0,0,1));
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
        #if 0
        DrawAsset(&PhongShadingProgram, AssetManager, &RenderGroup->Arena, AssetTest, LightColor);
        #else
          SetUniformM4(PhongShadingProgram, open_gl_uniform::m4_Model, AssetTest->M);
          SetUniformM4(PhongShadingProgram, open_gl_uniform::m4_Normal, AssetTest->NM);
          SetUniformM4(PhongShadingProgram, open_gl_uniform::m4_Texture, AssetTest->TM);

          u32 AssetHandle = AssetTest->AssetHandle;
          material* Material   = GetMaterial(AssetManager, AssetHandle);
          u32 SurfaceSmoothness = 3;
          v4 AmbientColor   = Blend(&LightColor, &Material->AmbientColor);
          AmbientColor.W = 1;
          v4 DiffuseColor   = Blend(&LightColor, &Material->DiffuseColor) * (1.f / 3.1415f);
          DiffuseColor.W = 1;
          v4 SpecularColor  = Blend(&LightColor, &Material->SpecularColor) * ( SurfaceSmoothness + 8.f ) / (8.f*3.1415f);
          SpecularColor.W = 1;
          
          SetUniformV4(PhongShadingProgram, open_gl_uniform::v4_AmbientProduct, AmbientColor);
          SetUniformV4(PhongShadingProgram, open_gl_uniform::v4_DiffuseProduct, DiffuseColor);
          SetUniformV4(PhongShadingProgram, open_gl_uniform::v4_SpecularProduct, SpecularColor);
          SetUniformS( PhongShadingProgram, open_gl_uniform::s_Shininess, Material->Shininess);

          book_keeper* BitmapKeeper = 0;
          bitmap* RenderTarget = GetBitmap(AssetManager, AssetHandle, &BitmapKeeper);
          BindTextureToGPU(RenderTarget, BitmapKeeper);

          book_keeper* ObjectKeeper = 0;
          mesh_indeces* Object = GetObject(AssetManager, AssetHandle, &ObjectKeeper);
          glBindVertexArray( OpenGL->ElementVAO );

          // TODO: Use Unsinged Short
          glDrawElementsBaseVertex( GL_TRIANGLES, ObjectKeeper->Location.Count, GL_UNSIGNED_INT,
            (GLvoid*)(ObjectKeeper->Location.Index),
             ObjectKeeper->Location.VertexOffset);
          glBindVertexArray(0);
        #endif
      }break;
      case render_buffer_entry_type::LINE:
      {
        entry_type_line* Line = (entry_type_line*) Body;
        v3 Start = Line->Start;
        v3 End = Line->End;

        v3 xp = Normalize(End-Start);
        v3 vc = Normalize(CameraPosition - Start);

        // Make sure vc and x are not parallel
        if( Abs( (vc * xp) - 1.0f ) <  0.0001f )
        {
          break;
        }

        v3 yp = Normalize(CrossProduct(vc, xp));
        v3 zp = Normalize(CrossProduct(xp, yp));

        // Rotates from WorldCoordinateSystem to NewCoordinateSystem
        // RotMat * V3(1,0,0) = xp
        // RotMat * V3(0,1,0) = yp
        // RotMat * V3(0,0,1) = zp
        m4 RotMat = M4( xp.X, yp.X, zp.X, 0,
                        xp.Y, yp.Y, zp.Y, 0,
                        xp.Z, yp.Z, zp.Z, 0,
                        0,   0,   0, 1);

        r32 w  = Norm(End-Start);
        r32 h = Line->LineThickness;

        m4 ScaleMat = M4Identity();
        ScaleMat.E[0] = w;
        ScaleMat.E[5] = h;

        v3 MidPoint = (Start + End) / 2;
        m4 TransMat = M4Identity();
        TransMat.E[3]  = MidPoint.X;
        TransMat.E[7]  = MidPoint.Y;
        TransMat.E[11] = MidPoint.Z;

        m4 M = TransMat*RotMat*ScaleMat;

        m4 TM = M4Identity();
        SetUniformM4(PhongShadingProgram, open_gl_uniform::m4_Model, M);
        SetUniformM4(PhongShadingProgram, open_gl_uniform::m4_Texture, TM);


        u32 ObjectIndex = GetEnumeratedObjectIndex(AssetManager, predefined_mesh::QUAD);
        book_keeper* ObjectKeeper = 0;
        mesh_indeces* Object = GetObjectFromIndex(AssetManager, ObjectIndex, &ObjectKeeper);
        mesh_data* MeshData = GetMeshFromIndex(AssetManager, Object->MeshHandle);

        PushMeshToGPU(&RenderGroup->Arena, Object, ObjectKeeper, MeshData);

        material* Material = GetMaterialFromIndex(AssetManager, Line->MaterialIndex);
        SetUniformV4( PhongShadingProgram, open_gl_uniform::v4_AmbientProduct, Material->AmbientColor);
        SetUniformV4( PhongShadingProgram, open_gl_uniform::v4_DiffuseProduct, Material->DiffuseColor);
        SetUniformV4( PhongShadingProgram, open_gl_uniform::v4_SpecularProduct, Material->SpecularColor);
        SetUniformS( PhongShadingProgram, open_gl_uniform::s_Shininess, Material->Shininess);
        book_keeper* BitmapKeeper = 0;
        // Funnily enough texture "null" maps to index 0.
        // However TODO: Make a quick lookup to bitmaps similar to GetEnumeratedObjectIndex
        bitmap* RenderTarget = GetBitmapFromIndex(AssetManager, 0, &BitmapKeeper);
        BindTextureToGPU(RenderTarget, BitmapKeeper);

        OpenGLDraw( ObjectKeeper->BufferHandle.VAO,  DATA_TYPE_TRIANGLE, Object->Count, 0 );
      }break;
    }
  }


  
  RenderGroup = Commands->DebugRenderGroup;
  if( !RenderGroup->First) {return;}
  temporary_memory TempMem = BeginTemporaryMemory(&RenderGroup->Arena);

  {
    // TODO: Make these lookups fast
    book_keeper* ObjectKeeper = 0;
    u32 QuadIndex = GetAssetIndex(AssetManager, asset_type::OBJECT, "quad");
    mesh_indeces* QuadObject = GetObjectFromIndex(AssetManager, QuadIndex, &ObjectKeeper);
    mesh_data* QuadMesh = GetMeshFromIndex(AssetManager, QuadObject->MeshHandle);
    PushMeshToGPU(&RenderGroup->Arena, QuadObject, ObjectKeeper, QuadMesh);

    u32 TextEntries = RenderGroup->BufferCounts[(u32)render_buffer_entry_type::TEXT];
    u32 OverlayQuadEntries = RenderGroup->BufferCounts[(u32)render_buffer_entry_type::OVERLAY_QUAD];
    text_data* TextBuffer = PushArray(&RenderGroup->Arena, TextEntries, text_data);
    overlay_quad_data* QuadBuffer = PushArray(&RenderGroup->Arena, OverlayQuadEntries, overlay_quad_data);
    u32 QuadInstnceIndex = 0;
    u32 TextInstnceIndex = 0;
    for( push_buffer_header* Entry = RenderGroup->First; Entry != 0; Entry = Entry->Next )
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
          TexData.QuadRect = Text->QuadRect;
          TexData.UVRect = Text->UVRect;
          TexData.Color = V4(1,1,1,1);
          TextBuffer[TextInstnceIndex++] = TexData;
        }break;
      }
    }

    local_persist u32 quadInstanceVBO = 0;
    local_persist u32 textInstanceVBO = 0;
    if(!quadInstanceVBO)
    {
      glGenBuffers(1, &quadInstanceVBO);
      glGenBuffers(1, &textInstanceVBO);
    }

    opengl_program QuadOverlayProgram = Commands->OpenGL.QuadOverlayProgram;
    glUseProgram(QuadOverlayProgram.Program);
    SetUniformM4(QuadOverlayProgram, open_gl_uniform::m4_Projection, RenderGroup->ProjectionMatrix);

    // No need to clearh the depth buffer if we disable depth test
    glDisable(GL_DEPTH_TEST);
    // Enable Textures
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // The geometry and instance buffer is bound to this Index
    glBindVertexArray( ObjectKeeper->BufferHandle.VAO );
    glBindBuffer(GL_ARRAY_BUFFER, quadInstanceVBO);
    glEnableVertexAttribArray(3);
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(overlay_quad_data), (void *)(OffsetOf(overlay_quad_data,QuadRect)));
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(overlay_quad_data), (void *)OffsetOf(overlay_quad_data,Color));
    glVertexAttribDivisor(3, 1);
    glVertexAttribDivisor(4, 1);
    //glBindBuffer(GL_ARRAY_BUFFER, 0);
    u32 QuadBufferSize = sizeof(overlay_quad_data)*OverlayQuadEntries;
    // TODO: Investigate BufferSubData
    glBufferData(GL_ARRAY_BUFFER, QuadBufferSize, QuadBuffer, GL_STREAM_DRAW);

    glDrawElementsInstanced( GL_TRIANGLES, QuadObject->Count, GL_UNSIGNED_INT, 0, OverlayQuadEntries);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    opengl_program TextRenderProgram = Commands->OpenGL.TextOverlayProgram;
    glUseProgram(TextRenderProgram.Program);

    SetUniformM4(TextRenderProgram, open_gl_uniform::m4_Projection, RenderGroup->ProjectionMatrix);    

    local_persist b32 FontDataAddedToTextures = false;
    if(!FontDataAddedToTextures)
    {
      FontDataAddedToTextures = true;

      u32 FontTextureIndex = GetAssetIndex(AssetManager, asset_type::BITMAP, "debug_font");
      bitmap* FontRenderTarget = GetBitmapFromIndex(AssetManager, FontTextureIndex);
      
      glBindTexture(GL_TEXTURE_2D_ARRAY, Commands->OpenGL.TextureArray);
      u32 MipLevel = 0;
      u32 TextureSlot = 0;
      glTexSubImage3D(GL_TEXTURE_2D_ARRAY,
        MipLevel,
        0, 0, 0, // x0,y0,TextureSlot
        FontRenderTarget->Width, FontRenderTarget->Height, 1,
        Commands->OpenGL.DefaultTextureFormat, GL_UNSIGNED_BYTE, FontRenderTarget->Pixels);
    }

    // The geometry and instance buffer is bound to this Index
    glBindVertexArray( ObjectKeeper->BufferHandle.VAO );
    glBindBuffer(GL_ARRAY_BUFFER, textInstanceVBO);
    glEnableVertexAttribArray(3);
    glEnableVertexAttribArray(4);
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(text_data), (void *)(OffsetOf(text_data,QuadRect)));
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(text_data), (void *)(OffsetOf(text_data,UVRect)));
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(text_data), (void *)OffsetOf(text_data,Color));
    glVertexAttribDivisor(3, 1);
    glVertexAttribDivisor(4, 1);
    glVertexAttribDivisor(5, 1);
    u32 TextBufferSize = sizeof(text_data)*TextEntries;

    glBufferData(GL_ARRAY_BUFFER, TextBufferSize, TextBuffer, GL_STREAM_DRAW);



    // glDrawElementsInstanced is used when we have two buffers (1 Buffer with a geometry and another buffer with Instance Data )
    // https://stackoverflow.com/questions/24516993/is-it-possible-to-use-index-buffer-objects-ibo-with-the-function-glmultidrawe
    
    // glDrawElements uses 1 VertexBuffer and 1 IndexBuffer and draws the VertexBuffer using an offset into IndexBuffer
    
    // glMultiDrawElements uses 1 VertexBuffer and 1 IndexBuffer and draws n number of whats in the VertexBuffer using an offset into IndexBuffer specific for each n
    
    // glDrawElementsInstanced uses 2 VertexBuffers and 1 IndexBuffer. 1 VertexBuffer is coupled with the IndexBuffer and the other VertexBuffer has data to be applied per element
    glDrawElementsInstanced( GL_TRIANGLES, QuadObject->Count, GL_UNSIGNED_INT, 0, TextEntries);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
  }

  EndTemporaryMemory(TempMem);

}