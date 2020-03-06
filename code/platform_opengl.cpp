
#include "render_push_buffer.h"
#include "affine_transformations.h"
#include "component_mesh.h"
#include "component_surface.h"
#include "component_collider.h"
#include "component_camera.h"
#include "string.h"
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

opengl_program2D OpenGLCreateShaderProgram2D()
{
  char VertexShaderCode[] = {
"#version  330 core\n\
layout (location = 0) in vec3 vertice;\n\
\n\
uniform mat4 M;  // Model Matrix - Transforms points from ModelSpace to WorldSpace.\n\
                 // Includes Translation, Rotation and Scaling\n\
uniform mat4 V;  // View Matrix - Camera Position and Orientation in WorldSpace\n\
uniform mat4 P;  // Projection Matrix - Scales the visible world into the unit cube.\n\
\n\
void main()\n\
{\n\
  gl_Position = P*V*M*vec4(vertice,1);\n\
}\n\
"};

  char FragmentShaderCode[] ={
"#version 330 core\n\
out vec4 fragColor;\n\
\n\
uniform vec2 texCoord;\n\
uniform sampler2D ourTexture;\n\
\n\
void main() \n\
{\n\
  fragColor = texture(ourTexture, texCoord);\n\
}\n\
"};

  opengl_program2D Result = {};
  Result.Program = OpenGLCreateProgram( VertexShaderCode, FragmentShaderCode );
  glUseProgram(Result.Program);
  Result.M =  glGetUniformLocation(Result.Program, "M");
  Result.P =  glGetUniformLocation(Result.Program, "P");
  Result.V =  glGetUniformLocation(Result.Program, "V");
  Result.texCoord   = glGetUniformLocation(Result.Program, "texCoord");
  glUseProgram(0);

  return Result;
}

opengl_program3D OpenGLCreateShaderProgram3D()
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
out vec2  texCoord;\n\
\n\
void main()\n\
{\n\
\n\
  gl_PointSize = 10.0;\n\
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

  opengl_program3D Result = {};
  Result.Program = OpenGLCreateProgram( VertexShaderCode, FragmentShaderCode );
  glUseProgram(Result.Program);
  Result.M =  glGetUniformLocation(Result.Program, "M");
  Result.NM = glGetUniformLocation(Result.Program, "NM");
  Result.TM = glGetUniformLocation(Result.Program, "TM");
  Result.P =  glGetUniformLocation(Result.Program, "P");
  Result.V =  glGetUniformLocation(Result.Program, "V");
  Result.lightPosition   = glGetUniformLocation(Result.Program, "lightPosition");
  Result.cameraPosition  = glGetUniformLocation(Result.Program, "cameraPosition");
  Result.ambientProduct  = glGetUniformLocation(Result.Program, "ambientProduct");
  Result.diffuseProduct  = glGetUniformLocation(Result.Program, "diffuseProduct");
  Result.specularProduct = glGetUniformLocation(Result.Program, "specularProduct");
  Result.attenuation     = glGetUniformLocation(Result.Program, "attenuation");
  Result.shininess       = glGetUniformLocation(Result.Program, "shininess");
  glUseProgram(0);

  return  Result;
}

void OpenGLSendMeshToGPU( u32* VAO, u32* VBO, const u32 NrIndeces, const u32* IndexData, const u32 NrVertecies, const opengl_vertex* VertexData)
{
  Assert(VertexData);
  Assert(IndexData);

  if(*VAO==0)
  {
    Assert(*VBO == 0);
    // Generate vao
    glGenVertexArrays(1, VAO);

    // Generate a generic buffer
    glGenBuffers(1, VBO);
  }

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


  // TODO: Have a proper buffer object state containing VAO,VBO and EBO so we don't 'have' to query it
  GLuint EBO;
  glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, (GLint*) &EBO );
  if(!EBO)
  {
    glGenBuffers(1, &EBO);
  }
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
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

void LoadOrBindTexture( bitmap* Bitmap )
{
  local_persist bitmap EmptyBitmap = {};
  u8 WhitePixel[4] = {255,255,255,255};
  EmptyBitmap.Width  = 1;
  EmptyBitmap.Height = 1;
  EmptyBitmap.Pixels = (void*) WhitePixel;

  bitmap* RenderTarget = Bitmap ? Bitmap : &EmptyBitmap;
  if(!RenderTarget->Handle)
  {
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
  }

  glBindTexture( GL_TEXTURE_2D, RenderTarget->Handle );
}

v4 Blend(v4* A, v4* B)
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

gl_vertex_buffer CreateGLVertexBuffer( game_render_commands* Commands,
  const u32 IndexCount,
  const u32* VerticeIndeces, const u32* TextureIndeces, const u32* NormalIndeces,
  const v3* VerticeData,     const v2* TextureData,     const v3* NormalData)
{
  Assert(VerticeIndeces && VerticeData);
  u32 MemoryNeeded = IndexCount * (sizeof(opengl_vertex) + 4*sizeof(u32));
  Assert( MemoryNeeded <= Commands->TemporaryMemory.Remaining() );
  u32* GLVerticeIndexArray = (u32*) Commands->TemporaryMemory.GetMemory(3*IndexCount*sizeof(u32));
  u32* GLIndexArray        = (u32*) Commands->TemporaryMemory.GetMemory(  IndexCount*sizeof(u32));

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

  opengl_vertex* VertexData = (opengl_vertex*) Commands->TemporaryMemory.GetMemory(VerticeArrayCount * sizeof(opengl_vertex));
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

void OpenGLPushBufferData(game_render_commands* Commands, render_buffer* Buffer)
{
  if(!Buffer->Fill)
  {
    return;
  }

  gl_vertex_buffer GLBuffer =  CreateGLVertexBuffer(Commands,
    Buffer->nvi, Buffer->vi, Buffer->ti, Buffer->ni,
    Buffer->v, Buffer->vt, Buffer->vn);

  u32* VAO = Buffer->VAO;
  u32* VBO = Buffer->VBO;
  OpenGLSendMeshToGPU(VAO, VBO, GLBuffer.IndexCount, GLBuffer.Indeces, GLBuffer.VertexCount, GLBuffer.VertexData);

  Commands->TemporaryMemory.Clear();
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

internal void
OpenGLRenderGroupToOutput( game_render_commands* Commands, s32 WindowWidth, s32 WindowHeight )
{
  Assert(Commands->TemporaryMemory.IsEmpty());


  render_push_buffer* RenderPushBuffer = (render_push_buffer*) Commands->RenderMemory.GetBase();

  if(!Commands->RenderProgram3D.Program)
  {
    Commands->RenderProgram3D = OpenGLCreateShaderProgram3D();
  }

  // Enable depth test
  glEnable(GL_DEPTH_TEST);
  // Enable Textures
  glEnable(GL_TEXTURE_2D);
  // Activates the gl_PointSize = 10.0; variable in the shader
  glEnable(GL_PROGRAM_POINT_SIZE);
  // Accept fragment if it closer to the camera than the former one
  glDepthFunc(GL_LESS);

  glClearColor(0.7,0.7,0.7,1);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  const r32 DesiredAspectRatio = 1.77968526f;
  OpenGLSetViewport( DesiredAspectRatio, WindowWidth, WindowHeight );

  // OpenGL uses Column major convention.
  // Our math library uses Row major convention which means we need to transpose the
  // matrices AND reverse the order of multiplication.
  // Transpose(A*B) = Transpose(B) * Transpose(A)
  m4 V = RenderPushBuffer->ViewMatrix;
  m4 P = RenderPushBuffer->ProjectionMatrix;

  m4 Identity = M4Identity();
  opengl_program3D* Prog = &Commands->RenderProgram3D;
  glUseProgram( Commands->RenderProgram3D.Program);

  glUniformMatrix4fv(Prog->P,  1, GL_TRUE, P.E);
  glUniformMatrix4fv(Prog->V,  1, GL_TRUE, V.E);
  glUniformMatrix4fv(Prog->TM, 1, GL_TRUE, Identity.E);
  glUniform4fv( Prog->cameraPosition, 1, GetCameraPosition(&V).E);
  v4 LightColor    = V4(0,0,0,1);

  // For each render group
  for( push_buffer_header* Entry = RenderPushBuffer->First; Entry != 0; Entry = Entry->Next )
  {
    u8* Head = (u8*) Entry;
    u8* Body = Head + sizeof(push_buffer_header);
    setOpenGLState(Entry->RenderState);
    switch(Entry->Type)
    {
      case render_buffer_entry_type::LIGHT:
      {
        r32 Attenuation = 1;
        glUniform1f(  Prog->attenuation, Attenuation);
        entry_type_light* Light = (entry_type_light*) Body;
        glUniform4fv( Prog->lightPosition,  1, (Light->M*V4(0,0,0,1)).E);
        LightColor    = Light->Color;
      }break;

      case render_buffer_entry_type::INDEXED_BUFFER:
      {
        entry_type_indexed_buffer* IndexedBuffer = (entry_type_indexed_buffer*) Body;

        glUniformMatrix4fv(Prog->M,  1, GL_TRUE, IndexedBuffer->M.E);
        glUniformMatrix4fv(Prog->NM, 1, GL_TRUE, IndexedBuffer->NM.E);;

        OpenGLPushBufferData(Commands, IndexedBuffer->Buffer);

        Assert(IndexedBuffer->Surface);
        component_surface* Surface = IndexedBuffer->Surface;
        material* Material = Surface->Material;
        LoadOrBindTexture(Material->DiffuseMap);

        u32 SurfaceSmoothnes = 3;
        v4 AmbientColor   = Material->DiffuseMap ? V4(0,0,0,1) : Blend(&LightColor, &Material->AmbientColor);
        v4 DiffuseColor   = Blend(&LightColor, &Material->DiffuseColor) * (1.f / 3.1415f);
        v4 SpecularColor  = Blend(&LightColor, &Material->SpecularColor) * ( SurfaceSmoothnes + 8.f ) / (8.f*3.1415f);
        glUniform4fv( Prog->ambientProduct,  1, AmbientColor.E);
        glUniform4fv( Prog->diffuseProduct,  1, DiffuseColor.E);
        glUniform4fv( Prog->specularProduct, 1, SpecularColor.E);
        glUniform1f( Prog->shininess, Material->Shininess);

        OpenGLDraw( *IndexedBuffer->Buffer->VAO, IndexedBuffer->DataType, IndexedBuffer->ElementLength, IndexedBuffer->ElementStart );

      }break;

      case render_buffer_entry_type::PRIMITIVE:
      {
        entry_type_primitive* Primitive = (entry_type_primitive*) Body;
        u32 VAO = 0;
        u32 NrIndeces = 0;
        u32 ElementType = 0;
        if(Primitive->PrimitiveType == primitive_type::POINT)
        {
          glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
          local_persist u32 PointVao = 0;
          local_persist u32 PointVbo = 0;
          if(!PointVao)
          {
            u32 idx = 0;
            opengl_vertex v = {};
            r32 length = 0.5;
            v.v = V3(0,0,0);
            v.vn = V3(0,0,1);
            v.vt = V2(0,0);
            OpenGLSendMeshToGPU(&PointVao, &PointVbo, 1, &idx, 1, &v );
          }

          VAO = PointVao;
          NrIndeces = 1;
          ElementType = DATA_TYPE_POINT;

        OpenGLDraw( VAO, DATA_TYPE_TRIANGLE, NrIndeces, 0 );
        }else if(Primitive->PrimitiveType == primitive_type::QUAD)
        {
          local_persist u32 QuadVAO = 0;
          local_persist u32 QuadVBO = 0;
          if(!QuadVAO)
          {
            opengl_vertex Vertex[4] = {};
            u32 Index[6] = {};
            GetQuad(Index, Vertex);
            OpenGLSendMeshToGPU(&QuadVAO, &QuadVBO, 6, Index, 4, Vertex );
          }
          VAO = QuadVAO;
          NrIndeces = 6;
          ElementType = DATA_TYPE_TRIANGLE;
        }else if(Primitive->PrimitiveType == primitive_type::VOXEL)
        {
          local_persist u32 VoxelVAO = 0;
          local_persist u32 VoxelVBO = 0;
          if(!VoxelVAO)
          {
            opengl_vertex Vertex[36] = {};
            u32 Index[36] = {};
            GetVoxel(Index, Vertex);
            OpenGLSendMeshToGPU(&VoxelVAO, &VoxelVBO, 36, Index, 36, Vertex );
          }
          VAO = VoxelVAO;
          NrIndeces = 36;
          ElementType = DATA_TYPE_TRIANGLE;
        }

        glUniformMatrix4fv(Prog->M,  1, GL_TRUE, Primitive->M.E);
        glUniformMatrix4fv(Prog->NM, 1, GL_TRUE, M4Identity().E);
        glUniformMatrix4fv(Prog->TM, 1, GL_TRUE, Primitive->TM.E);

        component_surface* Surface = Primitive->Surface;
        material* Material =  Surface->Material;
        u32 SurfaceSmoothnes = 3;
        v4 AmbientColor   = Material->DiffuseMap ? V4(0,0,0,1) : Blend(&LightColor, &Material->AmbientColor);
        v4 DiffuseColor   = Blend(&LightColor, &Material->DiffuseColor) * (1.f / 3.1415f);
        v4 SpecularColor  = Blend(&LightColor, &Material->SpecularColor) * ( SurfaceSmoothnes + 8.f ) / (8.f*3.1415f);
        LoadOrBindTexture(Material->DiffuseMap);
        glUniform4fv( Prog->ambientProduct,  1, AmbientColor.E);
        glUniform4fv( Prog->diffuseProduct,  1, DiffuseColor.E);
        glUniform4fv( Prog->specularProduct, 1, SpecularColor.E);
        glUniform1f(  Prog->shininess, Material->Shininess);

        OpenGLDraw( VAO, ElementType, NrIndeces, 0 );
      }break;
    }
  }

#if 0
  // Here we do overlays so clear GL_DEPTH_BUFFER_BIT
  glClear(GL_DEPTH_BUFFER_BIT);
  for( push_buffer_header* Entry = RenderPushBuffer->First; Entry != 0; Entry = Entry->Next )
  {
    u8* Head = (u8*) Entry;
    u8* Body = Head + sizeof(push_buffer_header);
    switch(Entry->Type)
    {
      case render_buffer_entry_type::POINT:
      {
        glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
        entry_type_point* Point = (entry_type_point*) Body;
        local_persist u32 PointVao = 0;
        if(!PointVao)
        {
          u32 idx = 0;
          opengl_vertex v = {};
          r32 length = 0.5;
          v.v = V3(0,0,0);
          v.vn = V3(0,0,1);
          v.vt = V2(0,0);
          OpenGLSendMeshToGPU(&PointVao, 1, &idx, 1, &v );
        }
        LoadTexture(&EmptyBitmap);
        glUniformMatrix4fv(Prog->M,  1, GL_TRUE, Point->M.E);
        glUniformMatrix4fv(Prog->NM, 1, GL_TRUE, M4Identity().E);
        glUniformMatrix4fv(Prog->TM, 1, GL_TRUE, M4Identity().E);
        glUniform4fv( Prog->ambientProduct,  1, V4(1,1,1,1).E);
        glUniform4fv( Prog->diffuseProduct,  1, V4(1,1,1,1).E);
        glUniform4fv( Prog->specularProduct, 1, V4(1,1,1,1).E);
        glUniform1f(  Prog->shininess, 1);

        OpenGLDraw( PointVao, 6 );
      }break;
      case render_buffer_entry_type::WIREBOX:
      {
        entry_type_wirebox* WireBox = (entry_type_wirebox*) Body;
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        if(WireBox->CullFace)
        {
          glEnable(GL_CULL_FACE);
        }else{
          glDisable(GL_CULL_FACE);
        }
        if(!WireBox->Mesh->VAO)
        {
          PushColliderMeshToOpenGL( Commands, WireBox->Mesh );
        }
        LoadTexture(&EmptyBitmap);
        glUniformMatrix4fv(Prog->M,  1, GL_TRUE, WireBox->M.E);
        glUniformMatrix4fv(Prog->NM, 1, GL_TRUE, M4Identity().E);
        glUniformMatrix4fv(Prog->TM, 1, GL_TRUE, M4Identity().E);
        glUniform4fv( Prog->ambientProduct,  1, V4(1,1,1,1).E);
        glUniform4fv( Prog->diffuseProduct,  1, V4(1,1,1,1).E);
        glUniform4fv( Prog->specularProduct, 1, V4(1,1,1,1).E);
        glUniform1f(  Prog->shininess, 1);

        OpenGLDraw( WireBox->Mesh->VAO, 6 );
        break;
      }
    }

  }
#endif
  Assert(Commands->TemporaryMemory.IsEmpty());
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