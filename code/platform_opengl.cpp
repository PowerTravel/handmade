
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
out vec4 highlightColor;
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

  vertexColor = ambientProduct;
  highlightColor =  Kd*(diffuseProduct + Ks*specularProduct);

  vec4 tmpTex = TM*vec4(textureCoordinate,0,1);
  texCoord = vec2(tmpTex.x,tmpTex.y);
  gl_Position = P*V*Vertice;
}
)FOO";

  char* FragmentShaderCode = R"FOO(
#version 330 core
in vec4  vertexColor;
in vec2  texCoord;
in vec4  highlightColor;

out vec4 fragColor;

uniform sampler2D ourTexture;

void main() 
{
  vec4 tc = texture(ourTexture, texCoord);
  float blend = 0.6;
  fragColor.x = tc.x * vertexColor.x * blend;
  fragColor.y = tc.y * vertexColor.y * blend;
  fragColor.z = tc.z * vertexColor.z * blend;
  fragColor.w = tc.w;
  fragColor = fragColor + highlightColor;
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

  OpenGL->BufferSize = Megabytes(8);

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
    OpenGL->TextBaseOffset = OpenGL->BufferSize/2;
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
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(text_data), (void *)(OpenGL->TextBaseOffset + OffsetOf(text_data,QuadRect)));
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(text_data), (void *)(OpenGL->TextBaseOffset + OffsetOf(text_data,UVRect)));
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(text_data), (void *)(OpenGL->TextBaseOffset + OffsetOf(text_data,Color)));
    glVertexAttribDivisor(3, 1);
    glVertexAttribDivisor(4, 1);
    glVertexAttribDivisor(5, 1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

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


void PushObjectToGPU(open_gl* OpenGL, game_asset_manager* AssetManager, object_handle ObjectHandle)
{ 
  buffer_keeper* ObjectKeeper = 0;
  mesh_indeces* Object = GetAsset(AssetManager, ObjectHandle, &ObjectKeeper);
  
  buffer_keeper* MeshKeeper = 0;
  mesh_data* MeshData = GetAsset(AssetManager, Object->MeshHandle, &MeshKeeper);

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

  Assert(ObjectKeeper->ReferenceCount);
  ObjectKeeper->Count = GLBuffer.IndexCount;
  ObjectKeeper->Index = ((u8*) 0 + EBOOffset);
  ObjectKeeper->VertexOffset = VBOOffset/sizeof(opengl_vertex);

  //glBindVertexArray(0);
  EndTemporaryMemory(TempMem);
}

void PushBitmapToGPU(open_gl* OpenGL, game_asset_manager* AssetManager, bitmap_handle BitmapHandle)
{
  bitmap_keeper* BitmapKeeper = 0;
  bitmap* RenderTarget = GetAsset(AssetManager, BitmapHandle, &BitmapKeeper);

  Assert(RenderTarget->BPP == 32);

  if(!RenderTarget->Special)
  {
    glBindTexture( GL_TEXTURE_2D_ARRAY, OpenGL->TextureArray );
    u32 MipLevel = 0;
    u32 TextureSlot = 0;
    BitmapKeeper->Special = false;
    BitmapKeeper->TextureSlot = OpenGL->TextureCount++;
    Assert(OpenGL->TextureCount < OpenGL->MaxTextureCount);

    glTexSubImage3D(GL_TEXTURE_2D_ARRAY,
      MipLevel,
      0, 0, 0, // x0,y0,TextureSlot
      RenderTarget->Width, RenderTarget->Height, 1,
      OpenGL->DefaultTextureFormat, GL_UNSIGNED_BYTE, RenderTarget->Pixels);
  }else{

    BitmapKeeper->TextureSlot = OpenGL->SpecialTextureCount++;
    BitmapKeeper->Special = true;
    Assert(OpenGL->SpecialTextureCount < OpenGL->MaxSpecialTextureCount);
    glBindTexture( GL_TEXTURE_2D, OpenGL->SpecialTextures[BitmapKeeper->TextureSlot] );
    glTexImage2D( GL_TEXTURE_2D,  0, GL_RGBA8,
            RenderTarget->Width,  RenderTarget->Height,
            0, GL_BGRA_EXT, GL_UNSIGNED_BYTE,
            RenderTarget->Pixels);
  }

}

#define DRAW_INSTANCED 0
void OpenGLRenderGroupToOutput( game_render_commands* Commands)
{
  TIMED_FUNCTION();
  render_group* RenderGroup = Commands->MainRenderGroup;
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

  // Accept fragment if it closer to the camera than the former one
  // Enable depth test
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  
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
        SetUniformM4(PhongShadingProgram, open_gl_uniform::m4_Model, AssetTest->M);
        SetUniformM4(PhongShadingProgram, open_gl_uniform::m4_Normal, AssetTest->NM);
        SetUniformM4(PhongShadingProgram, open_gl_uniform::m4_Texture, AssetTest->TM);

        instance_handle AssetHandle = AssetTest->AssetHandle;
        material* Material = GetMaterial(AssetManager, AssetHandle);
        u32 SurfaceSmoothness = 3;
        v4 AmbientColor = Blend(&LightColor, &Material->AmbientColor);
        AmbientColor.W = 1;
        v4 DiffuseColor = Blend(&LightColor, &Material->DiffuseColor) * (1.f / 3.1415f);
        DiffuseColor.W = 1;
        v4 SpecularColor = Blend(&LightColor, &Material->SpecularColor) * ( SurfaceSmoothness + 8.f ) / (8.f*3.1415f);
        SpecularColor.W = 1;

        SetUniformV4(PhongShadingProgram, open_gl_uniform::v4_AmbientProduct, AmbientColor);
        SetUniformV4(PhongShadingProgram, open_gl_uniform::v4_DiffuseProduct, DiffuseColor);
        SetUniformV4(PhongShadingProgram, open_gl_uniform::v4_SpecularProduct, SpecularColor);
        SetUniformS(PhongShadingProgram, open_gl_uniform::s_Shininess, Material->Shininess);

        bitmap_keeper* BitmapKeeper = 0;
        bitmap* RenderTarget = GetBitmap(AssetManager, AssetHandle, &BitmapKeeper);
        
        if(BitmapKeeper->Special)
        {
          glBindTexture( GL_TEXTURE_2D, OpenGL->SpecialTextures[BitmapKeeper->TextureSlot]);
        }

        buffer_keeper* ObjectKeeper = 0;
        mesh_indeces* Object = GetObject(AssetManager, AssetHandle, &ObjectKeeper);
        glBindVertexArray( OpenGL->ElementVAO );

        // TODO: Use Unsinged Short
        glDrawElementsBaseVertex( GL_TRIANGLES, ObjectKeeper->Count, GL_UNSIGNED_INT,
          (GLvoid*)(ObjectKeeper->Index),
           ObjectKeeper->VertexOffset);
        glBindVertexArray(0);
      }break;
    }
  }


  
  RenderGroup = Commands->DebugRenderGroup;
  if( !RenderGroup->First) {return;}
 
  temporary_memory TempMem = BeginTemporaryMemory(&RenderGroup->Arena);

  {

    u32 TextEntryCount = RenderGroup->BufferCounts[(u32)render_buffer_entry_type::TEXT];
    u32 OverlayQuadEntryCount = RenderGroup->BufferCounts[(u32)render_buffer_entry_type::OVERLAY_QUAD];
    { // Send data to VBO
      text_data* TextBuffer = PushArray(&RenderGroup->Arena, TextEntryCount, text_data);
      overlay_quad_data* QuadBuffer = PushArray(&RenderGroup->Arena, OverlayQuadEntryCount, overlay_quad_data);
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

      u32 QuadBufferSize = sizeof(overlay_quad_data)*OverlayQuadEntryCount;
      u32 TextBufferSize = sizeof(text_data)*TextEntryCount;

      glBindBuffer(GL_ARRAY_BUFFER, OpenGL->InstanceVBO);
      glBufferSubData( GL_ARRAY_BUFFER,        // Target
                       OpenGL->QuadBaseOffset, // Offset
                       QuadBufferSize,         // Size
                       (GLvoid*) QuadBuffer);  // Data
      glBufferSubData( GL_ARRAY_BUFFER,        // Target
                       OpenGL->TextBaseOffset, // Offset
                       TextBufferSize,         // Size
                       (GLvoid*) TextBuffer);  // Data
      glBindBuffer(GL_ARRAY_BUFFER, 0);

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

    buffer_keeper* ElementObjectKeeper = 0;
    object_handle ObjectHandle = GetEnumeratedObjectHandle(AssetManager, predefined_mesh::QUAD);
    GetAsset(AssetManager, ObjectHandle, &ElementObjectKeeper);

    glBindVertexArray( OpenGL->QuadVAO );
    glDrawElementsInstancedBaseVertex(
      GL_TRIANGLES,                           // Mode,
      ElementObjectKeeper->Count,             // Nr of Elements (Triangles*3)
      GL_UNSIGNED_INT,                        // Index Data Type  
      (GLvoid*)(ElementObjectKeeper->Index),  // Pointer somewhere in the index buffer
      OverlayQuadEntryCount,                  // How many Instances to draw
      ElementObjectKeeper->VertexOffset);     // Base Offset into the geometry vbo
    glBindVertexArray(0);

    opengl_program TextRenderProgram = Commands->OpenGL.TextOverlayProgram;
    glUseProgram(TextRenderProgram.Program);

    SetUniformM4(TextRenderProgram, open_gl_uniform::m4_Projection, RenderGroup->ProjectionMatrix);    

    local_persist b32 FontDataAddedToTextures = false;
    if(!FontDataAddedToTextures)
    {
      FontDataAddedToTextures = true;

      bitmap_handle FontHandle;
      GetHandle(AssetManager, "debug_font", &FontHandle);
      bitmap* FontRenderTarget = GetAsset(AssetManager, FontHandle);
      
      glBindTexture(GL_TEXTURE_2D_ARRAY, Commands->OpenGL.TextureArray);
      u32 MipLevel = 0;
      u32 TextureSlot = 0;
      glTexSubImage3D(GL_TEXTURE_2D_ARRAY,
        MipLevel,
        0, 0, 0, // x0,y0,TextureSlot
        FontRenderTarget->Width, FontRenderTarget->Height, 1,
        Commands->OpenGL.DefaultTextureFormat, GL_UNSIGNED_BYTE, FontRenderTarget->Pixels);
    }

    // glDrawElementsInstanced is used when we have two buffers (1 Buffer with a geometry and another buffer with Instance Data )
    // https://stackoverflow.com/questions/24516993/is-it-possible-to-use-index-buffer-objects-ibo-with-the-function-glmultidrawe
    
    // glDrawElements uses 1 VertexBuffer and 1 IndexBuffer and draws the VertexBuffer using an offset into IndexBuffer
    
    // glMultiDrawElements uses 1 VertexBuffer and 1 IndexBuffer and draws n number of whats in the VertexBuffer using an offset into IndexBuffer specific for each n
    
    glBindVertexArray(OpenGL->TextVAO);
    glDrawElementsInstancedBaseVertex(
      GL_TRIANGLES,                           // Mode,
      ElementObjectKeeper->Count,             // Nr of Elements (Triangles*3)
      GL_UNSIGNED_INT,                        // Index Data Type  
      (GLvoid*)(ElementObjectKeeper->Index),  // Pointer somewhere in the index buffer
      TextEntryCount,                         // How many Instances to draw
      ElementObjectKeeper->VertexOffset);     // Base Offset into the geometry vbo
    glBindVertexArray(0);
  }

  EndTemporaryMemory(TempMem);

}