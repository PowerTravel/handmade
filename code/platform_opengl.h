#pragma once

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

#define GL_MULTISAMPLE                    0x809D
#define GL_SAMPLE_ALPHA_TO_COVERAGE       0x809E
#define GL_SAMPLE_ALPHA_TO_ONE            0x809F
#define GL_SAMPLE_COVERAGE                0x80A0
#define GL_SAMPLE_BUFFERS                 0x80A8
#define GL_SAMPLES                        0x80A9
#define GL_SAMPLE_COVERAGE_VALUE          0x80AA
#define GL_SAMPLE_COVERAGE_INVERT         0x80AB
#define GL_TEXTURE_2D_MULTISAMPLE         0x9100
#define GL_TEXTURE_3D                     0x806F
#define GL_MAX_SAMPLES                    0x8D57
#define GL_MAX_COLOR_TEXTURE_SAMPLES      0x910E
#define GL_MAX_DEPTH_TEXTURE_SAMPLES      0x910F
#define GL_ONE_MINUS_CONSTANT_COLOR       0x8002

#define GL_FRAGMENT_SHADER                0x8B30
#define GL_VERTEX_SHADER                  0x8B31



// glGetShaderiv — return a parameter from a shader object
#define GL_SHADER_TYPE                    0x8B4F
#define GL_DELETE_STATUS                  0x8B80
#define GL_COMPILE_STATUS                 0x8B81
#define GL_INFO_LOG_LENGTH                0x8B84
#define GL_SHADER_SOURCE_LENGTH           0x8B88

// glGetProgramiv — return a parameter from a program object
#define GL_LINK_STATUS                    0x8B82
#define GL_VALIDATE_STATUS                0x8B83
#define GL_INFO_LOG_LENGTH                0x8B84
#define GL_ATTACHED_SHADERS               0x8B85
#define GL_ACTIVE_UNIFORMS                0x8B86
#define GL_ACTIVE_UNIFORM_MAX_LENGTH      0x8B87
#define GL_ACTIVE_ATTRIBUTES              0x8B89
#define GL_ACTIVE_ATTRIBUTE_MAX_LENGTH    0x8B8A

// glBindBuffer, glBufferData
#define GL_ARRAY_BUFFER                   0x8892
#define GL_ATOMIC_COUNTER_BUFFER          0x92C0
#define GL_COPY_READ_BUFFER               0x8F36
#define GL_COPY_WRITE_BUFFER              0x8F37
#define GL_DISPATCH_INDIRECT_BUFFER       0x90EE
#define GL_DRAW_INDIRECT_BUFFER           0x8F3F
#define GL_ELEMENT_ARRAY_BUFFER           0x8893
#define GL_PIXEL_PACK_BUFFER              0x88EB
#define GL_PIXEL_UNPACK_BUFFER            0x88EC
#define GL_QUERY_BUFFER                   0x9192
#define GL_SHADER_STORAGE_BUFFER          0x90D2
#define GL_TEXTURE_BUFFER                 0x8C2A
#define GL_TRANSFORM_FEEDBACK_BUFFER      0x8C8E
#define GL_UNIFORM_BUFFER                 0x8A11

// glBufferData
#define GL_STREAM_DRAW                    0x88E0
#define GL_STREAM_READ                    0x88E1
#define GL_STREAM_COPY                    0x88E2
#define GL_STATIC_DRAW                    0x88E4
#define GL_STATIC_READ                    0x88E5
#define GL_STATIC_COPY                    0x88E6
#define GL_DYNAMIC_DRAW                   0x88E8
#define GL_DYNAMIC_READ                   0x88E9
#define GL_DYNAMIC_COPY                   0x88EA

#define GL_UNIFORM_BUFFER_BINDING         0x8A28
#define GL_PROGRAM_POINT_SIZE             0x8642
#define GL_ELEMENT_ARRAY_BUFFER_BINDING   0x8895

#define GL_CLAMP_TO_BORDER                0x812D
#define GL_CLAMP_TO_EDGE                  0x812F
#define GL_MIRRORED_REPEAT                0x8370

#include "math/vector_math.h"

struct opengl_handles
{
	u32 VAO;
	u32 VBO;
	u32 EBO;
	u32 TextureHandle;
};

struct opengl_vertex
{
	v3 v;
	v3 vn;
	v2 vt;
};

struct opengl_info
{
	char* Vendor;
	char* Renderer;
	char* Version;
	char* ShadingLanguageVersion;
	char* Extensions;
	b32 EXT_texture_sRGB_decode;
	b32 GL_ARB_framebuffer_sRGB;
	b32 GL_ARB_vertex_shader;
	b32 GL_blend_func_separate;
};

// A big list of ALL possible uniform names
enum class open_gl_uniform
{
  m4_Model,
  m4_Normal,
  m4_Texture,
  m4_Projection,
  m4_View,
  v4_AmbientProduct,
  v4_DiffuseProduct,
  v4_SpecularProduct,
  v4_LightPosition,
  v4_CameraPosition,
  v2_TextureCoordinate,
  s_Shininess,
  count  // 13
};


#define OPEN_GL_UNIFORM_NAME_SIZE 32
#define OPEN_GL_UNIFORM_COUNT 12

struct uniform_map
{
	open_gl_uniform Enum;
	c8 Name[OPEN_GL_UNIFORM_NAME_SIZE];
};

struct opengl_program
{
	u32 Program;
  u32 VAO;
	s32 Uniforms[OPEN_GL_UNIFORM_COUNT];
};

struct opengl_buffer_object
{
	u32 VertexArrayObject;
	u32 VertexBuffer;
	u32 UVBuffer;
	u32 NormalBuffer;
	u32 FaceBuffer;
};


struct open_gl
{
  // Render Programs
  bool Initialized;
  opengl_program PhongShadingProgram;
  opengl_program QuadOverlayProgram;
  opengl_program TextOverlayProgram;

  // Frame Buffers (Intermediate Render Targets)

  // Texture Queue (Keeping all our textures?)

  // Non-Instanced Geoms
  // Single VAO InstancedVertexArray (For textured vertices)
  // Single IndexArray (Each object has an offset into this array)


  // Instanced Geoms 
  // Single VAO InstancedVertexArray (For shared object data) (geometries, uv-coords, normals)
  // Single IndexArray
  // Single VAO InstanceArray        (For per object data, position, color, texture index) 
};

void InitOpenGL(open_gl* OpenGL);
void OpenGLInitExtensions();
