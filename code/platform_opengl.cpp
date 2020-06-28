
#include "render_push_buffer.h"
#include "math/affine_transformations.h"
#include "bitmap.h"

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
    Assert(0);
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
    void* ProgramMessage;
    GLint ProgramMessageSize;
    glGetProgramiv( Program, GL_INFO_LOG_LENGTH, &ProgramMessageSize );

    ProgramMessage = VirtualAlloc(0, ProgramMessageSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    void* Tmp      = VirtualAlloc(0, ProgramMessageSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    glGetProgramInfoLog(Program, ProgramMessageSize, NULL, (GLchar*) ProgramMessage);
    snprintf((char*)Tmp, ProgramMessageSize, "%s",(char*) ProgramMessage);
    VirtualFree(ProgramMessage, 0, MEM_RELEASE );
    VirtualFree(Tmp, 0, MEM_RELEASE );
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
  Assert(Program->Uniforms[(u32)(Enum)] >= 0);
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

opengl_program OpenGLCreateTexturedQuadOverlayProgram( )
{
  char VertexShaderCode[] = {
"#version  330 core\n\
layout (location = 0) in vec3 vertice;\n\
layout (location = 2) in vec2 textureCoordinate;\n\
\n\
uniform mat4 P;  // Projection Matrix - Transforms points from ScreenSpace to UnitQube.\n\
uniform mat4 M;  // Model Matrix - Transforms points from ModelSpace to WorldSpace.\n\
uniform mat4 TM; // Texture Model Matrix. Shifts texture coordinates in a bitmap\n\
out vec2 texCoord;\n\
\n\
void main()\n\
{\n\
  gl_Position = P*M*vec4(vertice,1);\n\
  vec4 tmpTex = TM*vec4(textureCoordinate,0,1);\n\
  texCoord = vec2(tmpTex.x,tmpTex.y);\n\
}\n\
"};

  char FragmentShaderCode[] ={
"#version 330 core\n\
out vec4 fragColor;\n\
\n\
in vec2 texCoord;\n\
uniform sampler2D ourTexture;\n\
uniform vec4 ambientProduct;\n\
\n\
void main() \n\
{\n\
  fragColor = texture(ourTexture, texCoord) * ambientProduct;\n\
}\n\
"};

  opengl_program Result = {};
  Result.Program = OpenGLCreateProgram( VertexShaderCode, FragmentShaderCode );
  glUseProgram(Result.Program);
  DeclareUniform(&Result, open_gl_uniform::m4_Projection);
  DeclareUniform(&Result, open_gl_uniform::m4_Model);
  DeclareUniform(&Result, open_gl_uniform::m4_Texture);
  DeclareUniform(&Result, open_gl_uniform::v4_AmbientProduct);
  glUseProgram(0);

  return Result;
}

opengl_program OpenGLCreateProgram3D()
{
   global_variable char VertexShaderCode[] = {
"#version  330 core\n\
layout (location = 0) in vec3 vertice;\n\
layout (location = 1) in vec3 verticeNormal;\n\
layout (location = 2) in vec2 textureCoordinate;\n\
\n\
uniform mat4 M;  // Model Matrix - Transforms points from ModelSpace to WorldSpace.\n\
                 // Includes Translation, Rotation and Scaling\n\
uniform mat4 NM; // Normal Model Matrix = Transpose( RigidInverse(M) );\n\
                 // Keeps normals correct under shearing scaling\n\
uniform mat4 TM; // Texture Model Matrix. Shifts texture coordinates in a bitmap\n\
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
out vec4 vertexColor;\n\
out vec2 texCoord;\n\
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
  float Kd = max(dot(L,N), 0.0);\n\
\n\
  vec4 E = normalize( cameraPosition - Vertice );\n\
  vec4 H = normalize(L+E);\n\
  float Ks = pow(max(dot(H,N), 0.0 ), shininess);\n\
\n\
  vertexColor = ambientProduct + Kd*(diffuseProduct + Ks*specularProduct);\n\
\n\
  vec4 tmpTex = TM*vec4(textureCoordinate,0,1);\n\
  texCoord = vec2(tmpTex.x,tmpTex.y);\n\
  gl_Position = P*V*Vertice;\n\
}\n\
"};

  global_variable char FragmentShaderCode[] =
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
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(opengl_vertex), (GLvoid*) NULL);           // Vertecis
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(opengl_vertex), (GLvoid*) sizeof(v3) );    // Normals
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(opengl_vertex), (GLvoid*) (2*sizeof(v3))); // Textures


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

internal void
OpenGLRenderGroupToOutput( game_render_commands* Commands)
{
  render_group* RenderGroup = Commands->MainRenderGroup;

  if(!Commands->ProgramCount)
  {
    Commands->Programs[Commands->ProgramCount++] = OpenGLCreateTexturedQuadOverlayProgram();
    Commands->Programs[Commands->ProgramCount++] = OpenGLCreateProgram3D();
  }

  // Enable depth test
  glEnable(GL_DEPTH_TEST);
  // Enable Textures
  glEnable(GL_TEXTURE_2D);
  // Activates the gl_PointSize = 10.0; variable in the shader
  // TODO: Remove this, use a billboard instead
  glEnable(GL_PROGRAM_POINT_SIZE);
  // Accept fragment if it closer to the camera than the former one
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

  m4 Identity = M4Identity();
  opengl_program PhongShadingProgram = Commands->Programs[1];
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
        DrawAsset(&PhongShadingProgram, RenderGroup->AssetManager, &RenderGroup->Arena, AssetTest, LightColor);
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


        u32 ObjectIndex = GetEnumeratedObjectIndex(RenderGroup->AssetManager, predefined_mesh::QUAD);
        book_keeper* ObjectKeeper = 0;
        mesh_indeces* Object = GetObjectFromIndex(RenderGroup->AssetManager, ObjectIndex, &ObjectKeeper);
        mesh_data* MeshData = GetMeshFromIndex(RenderGroup->AssetManager, Object->MeshHandle);

        PushMeshToGPU(&RenderGroup->Arena, Object, ObjectKeeper, MeshData);

        // Todo:: The reason we need to always push a bitmap even if were not using it is because we keep
        //        using the same shader for everything. Fix switching of shaders.
        material* Material = GetMaterialFromIndex(RenderGroup->AssetManager, Line->MaterialIndex);
        SetUniformV4( PhongShadingProgram, open_gl_uniform::v4_AmbientProduct, Material->AmbientColor);
        SetUniformV4( PhongShadingProgram, open_gl_uniform::v4_DiffuseProduct, Material->DiffuseColor);
        SetUniformV4( PhongShadingProgram, open_gl_uniform::v4_SpecularProduct, Material->SpecularColor);
        SetUniformS( PhongShadingProgram, open_gl_uniform::s_Shininess, Material->Shininess);
        book_keeper* BitmapKeeper = 0;
        // Funnily enough texture "null" maps to index 0.
        // However TODO: Make a quick lookup to bitmaps similar to GetEnumeratedObjectIndex
        bitmap* RenderTarget = GetBitmapFromIndex(RenderGroup->AssetManager, 0, &BitmapKeeper);
        BindTextureToGPU(RenderTarget, BitmapKeeper);

        OpenGLDraw( ObjectKeeper->BufferHandle.VAO,  DATA_TYPE_TRIANGLE, Object->Count, 0 );
      }break;
    }
  }


  // OpenGLCreateTexturedQuadOverlayProgram
  opengl_program DebugOverlay = Commands->Programs[0];
  RenderGroup = Commands->DebugRenderGroup;

  glUseProgram(DebugOverlay.Program);

  SetUniformM4(DebugOverlay, open_gl_uniform::m4_Projection, RenderGroup->ProjectionMatrix);

  // No need to clearh the depth buffer if we disable depth test
  glDisable(GL_DEPTH_TEST);
  // Enable Textures
  glEnable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // For each render group
  for( push_buffer_header* Entry = RenderGroup->First; Entry != 0; Entry = Entry->Next )
  {
    u8* Head = (u8*) Entry;
    u8* Body = Head + sizeof(push_buffer_header);
    setOpenGLState(Entry->RenderState);
    switch(Entry->Type)
    {
      case render_buffer_entry_type::OVERLAY_QUAD:
      {
        entry_type_overlay_quad* Quad = (entry_type_overlay_quad*) Body;
        SetUniformM4(DebugOverlay, open_gl_uniform::m4_Model, Quad->M);
        SetUniformM4(DebugOverlay, open_gl_uniform::m4_Texture, Quad->TM);

        book_keeper* ObjectKeeper = 0;
        mesh_indeces* Object = GetObjectFromIndex(RenderGroup->AssetManager, Quad->ObjectIndex, &ObjectKeeper);
        mesh_data* MeshData = GetMeshFromIndex(RenderGroup->AssetManager, Object->MeshHandle);

        PushMeshToGPU(&RenderGroup->Arena, Object, ObjectKeeper, MeshData);

        SetUniformV4(DebugOverlay, open_gl_uniform::v4_AmbientProduct, Quad->Colour);

        book_keeper* BitmapKeeper = 0;
        bitmap* RenderTarget = GetBitmapFromIndex(RenderGroup->AssetManager, Quad->TextureIndex, &BitmapKeeper);
        BindTextureToGPU(RenderTarget, BitmapKeeper);

        Assert(ObjectKeeper->BufferHandle.VAO);
        OpenGLDraw( ObjectKeeper->BufferHandle.VAO,  DATA_TYPE_TRIANGLE, Object->Count, 0 );
      }break;
      /*
      case render_buffer_entry_type::RENDER_ASSET:
      {
        entry_type_render_asset* AssetTest = (entry_type_render_asset*) Body;
        glUniformMatrix4fv(TextOverlay.M,  1, GL_TRUE, AssetTest->M.E);
        glUniformMatrix4fv(TextOverlay.TM, 1, GL_TRUE, AssetTest->TM.E);

        u32 AssetHandle = AssetTest->AssetHandle;
        book_keeper* ObjectKeeper = 0;
        mesh_indeces* Object = GetObject(RenderGroup->AssetManager, AssetHandle, &ObjectKeeper);
        mesh_data* MeshData = GetMesh(RenderGroup->AssetManager, AssetHandle);

        PushMeshToGPU(&RenderGroup->Arena, Object, ObjectKeeper, MeshData);


        material* Material = GetMaterial(RenderGroup->AssetManager, AssetHandle);
        glUniform4fv( TextOverlay.ambientProduct,  1, Material->AmbientColor.E);
        glUniform4fv( TextOverlay.diffuseProduct,  1, Material->DiffuseColor.E);
        glUniform4fv( TextOverlay.specularProduct, 1, Material->SpecularColor.E);
        //glUniform1f(  TextOverlay.shininess, Material->Shininess);


        book_keeper* BitmapKeeper = 0;
        bitmap* RenderTarget = GetBitmap(RenderGroup->AssetManager, AssetHandle, &BitmapKeeper);
        BindTextureToGPU(RenderTarget, BitmapKeeper);

        Assert(ObjectKeeper->BufferHandle.VAO);
        OpenGLDraw( ObjectKeeper->BufferHandle.VAO,  DATA_TYPE_TRIANGLE, Object->Count, 0 );

      }break;
      */
    }
  }
}
#if 1

void DisplayBitmapViaOpenGL( u32 Width, u32 Height, void* Memory )
{
}

#endif