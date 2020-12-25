#pragma once
/*
 * This file will bootstrap OpenGL on windows.
 * It declares globaly OpenGL functions and gets
 * using wglGetProcAddress
 */

#include <gl/gl.h>
#include "types.h"
#include "platform_opengl.h"

global_variable GLuint OpenGLDefaultInternalTextureFormat;

typedef BOOL WINAPI wgl_swap_interval_ext(int interval);
global_variable wgl_swap_interval_ext* wglSwapIntervalEXT;

typedef HGLRC WINAPI wgl_create_context_attrib_arb(HDC hDC, HGLRC hSharedContext, const int *attribList);

typedef void WINAPI gl_blend_equation_separate( GLenum modeRGB, GLenum modeAlpha );
global_variable gl_blend_equation_separate* glBlendEquationSeparate;

typedef uint64_t GLuint64;
typedef int64_t GLint64;
typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;
typedef char GLchar;

// Shader
typedef GLuint WINAPI gl_create_program (void);
typedef void   WINAPI gl_link_program(GLuint program);
typedef void   WINAPI gl_use_program(GLuint program);
typedef void   WINAPI gl_validate_program(GLuint program);
typedef void   WINAPI gl_get_program_iv(GLuint program, GLenum pname, GLint *params);
typedef void   WINAPI gl_get_program_info_log(GLuint program, GLsizei maxLength, GLsizei *length, GLchar *infoLog);

global_variable gl_create_program*       glCreateProgram;
global_variable gl_link_program*         glLinkProgram;
global_variable gl_use_program*          glUseProgram;
global_variable gl_validate_program*     glValidateProgram;
global_variable gl_get_program_iv*       glGetProgramiv;
global_variable gl_get_program_info_log* glGetProgramInfoLog;

typedef GLuint WINAPI gl_create_shader(  GLenum shaderType );
typedef void   WINAPI gl_delete_shader(  GLuint shader );
typedef void   WINAPI gl_attach_shader(  GLuint program, GLuint shader );
typedef void   WINAPI gl_detach_shader(  GLuint program, GLuint shader );
typedef void   WINAPI gl_shader_source(  GLuint shader,  GLsizei count, const GLchar** string, const GLint* length );
typedef void   WINAPI gl_compile_shader( GLuint shader );
typedef void   WINAPI gl_get_shader_iv ( GLuint shader, GLenum pname, GLint *params);
typedef void   WINAPI gl_get_shader_info_log(GLuint shader, GLsizei maxLength, GLsizei *length, GLchar *infoLog);

global_variable gl_create_shader*  glCreateShader;
global_variable gl_delete_shader*  glDeleteShader;
global_variable gl_attach_shader*  glAttachShader;
global_variable gl_detach_shader*  glDetachShader;
global_variable gl_shader_source*  glShaderSource;
global_variable gl_compile_shader* glCompileShader;
global_variable gl_get_shader_iv*  glGetShaderiv;
global_variable gl_get_shader_info_log* glGetShaderInfoLog;


// Vertex Array Object
// Generate
typedef void WINAPI gl_gen_vertex_arrays(GLsizei n, GLuint *array);
typedef void WINAPI gl_bind_vertex_array(GLuint array);
typedef void WINAPI gl_delete_vertex_arrys(GLsizei n, GLuint *array);
typedef GLboolean WINAPI gl_is_vertex_array(GLuint array);

typedef void WINAPI gl_vertex_attrib_pointer( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid * pointer);
typedef void WINAPI gl_vertex_attrib_i_pointer( GLuint index, GLint size, GLenum type, GLsizei stride, const GLvoid * pointer);
typedef void WINAPI gl_vertex_attrib_l_pointer( GLuint index, GLint size, GLenum type, GLsizei stride, const GLvoid * pointer);
typedef void WINAPI gl_enable_vertex_attrib_array( GLuint index);
typedef void WINAPI gl_disable_vertex_attrib_array( GLuint index);
typedef void WINAPI gl_vertex_attrib_divisor(GLuint index, GLuint divisor);
typedef void WINAPI gl_enable_vertex_array_attrib( GLuint vaobj, GLuint index);
typedef void WINAPI gl_disable_vertex_array_attrib( GLuint vaobj, GLuint index);

global_variable gl_gen_vertex_arrays* glGenVertexArrays;
global_variable gl_bind_vertex_array* glBindVertexArray;
global_variable gl_delete_vertex_arrys* glDeleteVertexArrays;
global_variable gl_is_vertex_array* glIsVertexArray;
global_variable gl_vertex_attrib_pointer* glVertexAttribPointer;
global_variable gl_vertex_attrib_i_pointer* glVertexAttribIPointer;
global_variable gl_vertex_attrib_l_pointer* glVertexAttribLPointer;
global_variable gl_enable_vertex_attrib_array* glEnableVertexAttribArray;
global_variable gl_disable_vertex_attrib_array* glDisableVertexAttribArray;
global_variable gl_vertex_attrib_divisor* glVertexAttribDivisor;
global_variable gl_enable_vertex_array_attrib* glEnableVertexArrayAttrib;
global_variable gl_disable_vertex_array_attrib* glDisableVertexArrayAttrib;


typedef GLenum WINAPI gl_get_error( void );
//global_variable gl_get_error* glGetError;


// Buffer Object
typedef void WINAPI gl_gen_buffers(GLsizei n, GLuint *arrays);
typedef void WINAPI gl_bind_buffer(GLenum target, GLuint buffer);
typedef void WINAPI gl_buffer_data( GLenum target, GLsizeiptr size, const GLvoid * data, GLenum usage );
typedef void WINAPI gl_buffer_sub_data(GLenum target, GLintptr offset, GLsizeiptr size, const void * data);
typedef void WINAPI gl_named_buffer_data( GLuint buffer, GLsizeiptr size, const GLvoid *data, GLenum usage);
typedef GLboolean WINAPI gl_is_buffer( GLuint buffer);
typedef void WINAPI gl_delete_buffer(GLsizei n, GLuint *buffer);

global_variable gl_gen_buffers* glGenBuffers;
global_variable gl_bind_buffer* glBindBuffer;
global_variable gl_buffer_data* glBufferData;
global_variable gl_buffer_sub_data* glBufferSubData;
global_variable gl_named_buffer_data* glNamedBufferData;
global_variable gl_is_buffer* glIsBuffer;
global_variable gl_delete_buffer* glDeleteBuffers;

typedef void WINAPI gl_draw_elements(GLenum mode, GLsizei count, GLenum type, const GLvoid * indices);
global_variable gl_draw_elements*  glDrawElementsA;

typedef void WINAPI gl_draw_elements_instanced(GLenum mode, GLsizei count, GLenum type, const GLvoid * indices, GLsizei instancecount);
global_variable gl_draw_elements_instanced*  glDrawElementsInstanced;

typedef void WINAPI gl_draw_elements_base_vertex ( GLenum mode, GLsizei count, GLenum type, GLvoid *indices, GLint basevertex );
global_variable gl_draw_elements_base_vertex* glDrawElementsBaseVertex;

typedef void WINAPI gl_draw_elements_instanced_base_vertex( GLenum mode, GLsizei count, GLenum type, void *indices, GLsizei instancecount, GLint basevertex);
global_variable gl_draw_elements_instanced_base_vertex* glDrawElementsInstancedBaseVertex;
typedef void gl_multi_draw_elements(GLenum mode, const GLsizei * count,GLenum type, const void * const * indices, GLsizei drawcount);
global_variable gl_multi_draw_elements* glMultiDrawElements;

typedef GLint WINAPI gl_get_uniform_location(GLuint program, const GLchar *name);
typedef void WINAPI gl_uniform_1f(GLint location, GLfloat v0);
typedef void WINAPI gl_uniform_2f(GLint location, GLfloat v0, GLfloat v1);
typedef void WINAPI gl_uniform_3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
typedef void WINAPI gl_uniform_4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
typedef void WINAPI gl_uniform_1i(GLint location, GLint v0);
typedef void WINAPI gl_uniform_2i(GLint location, GLint v0, GLint v1);
typedef void WINAPI gl_uniform_3i(GLint location, GLint v0, GLint v1, GLint v2);
typedef void WINAPI gl_uniform_4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
typedef void WINAPI gl_uniform_u1(GLint location, GLuint v0);
typedef void WINAPI gl_uniform_u2(GLint location, GLuint v0, GLuint v1);
typedef void WINAPI gl_uniform_u3(GLint location, GLuint v0, GLuint v1, GLuint v2);
typedef void WINAPI gl_uniform_u4(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3);
typedef void WINAPI gl_uniform_1fv(GLint location, GLsizei count, const GLfloat *value);
typedef void WINAPI gl_uniform_2fv(GLint location, GLsizei count, const GLfloat *value);
typedef void WINAPI gl_uniform_3fv(GLint location, GLsizei count, const GLfloat *value);
typedef void WINAPI gl_uniform_4fv(GLint location, GLsizei count, const GLfloat *value);
typedef void WINAPI gl_uniform_1iv(GLint location, GLsizei count, const GLint *value);
typedef void WINAPI gl_uniform_2iv(GLint location, GLsizei count, const GLint *value);
typedef void WINAPI gl_uniform_3iv(GLint location, GLsizei count, const GLint *value);
typedef void WINAPI gl_uniform_4iv(GLint location, GLsizei count, const GLint *value);
typedef void WINAPI gl_uniform_1uiv(GLint location, GLsizei count, const GLuint *value);
typedef void WINAPI gl_uniform_2uiv(GLint location, GLsizei count, const GLuint *value);
typedef void WINAPI gl_uniform_3uiv(GLint location, GLsizei count, const GLuint *value);
typedef void WINAPI gl_uniform_4uiv(GLint location, GLsizei count, const GLuint *value);
typedef void WINAPI gl_uniform_matrix_2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void WINAPI gl_uniform_matrix_3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void WINAPI gl_uniform_matrix_4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void WINAPI gl_uniform_matrix_2x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void WINAPI gl_uniform_matrix_3x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void WINAPI gl_uniform_matrix_2x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void WINAPI gl_uniform_matrix_4x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void WINAPI gl_uniform_matrix_3x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void WINAPI gl_uniform_matrix_4x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);

global_variable gl_get_uniform_location* glGetUniformLocation;
global_variable gl_uniform_1f* glUniform1f;
global_variable gl_uniform_2f* glUniform2f;
global_variable gl_uniform_3f* glUniform3f;
global_variable gl_uniform_4f* glUniform4f;
global_variable gl_uniform_1i* glUniform1i;
global_variable gl_uniform_2i* glUniform2i;
global_variable gl_uniform_3i* glUniform3i;
global_variable gl_uniform_4i* glUniform4i;
global_variable gl_uniform_u1* glUniform1ui;
global_variable gl_uniform_u2* glUniform2ui;
global_variable gl_uniform_u3* glUniform3ui;
global_variable gl_uniform_u4* glUniform4ui;
global_variable gl_uniform_1fv* glUniform1fv;
global_variable gl_uniform_2fv* glUniform2fv;
global_variable gl_uniform_3fv* glUniform3fv;
global_variable gl_uniform_4fv* glUniform4fv;
global_variable gl_uniform_1iv* glUniform1iv;
global_variable gl_uniform_2iv* glUniform2iv;
global_variable gl_uniform_3iv* glUniform3iv;
global_variable gl_uniform_4iv* glUniform4iv;
global_variable gl_uniform_1uiv* glUniform1uiv;
global_variable gl_uniform_2uiv* glUniform2uiv;
global_variable gl_uniform_3uiv* glUniform3uiv;
global_variable gl_uniform_4uiv* glUniform4uiv;
global_variable gl_uniform_matrix_2fv* glUniformMatrix2fv;
global_variable gl_uniform_matrix_3fv* glUniformMatrix3fv;
global_variable gl_uniform_matrix_4fv* glUniformMatrix4fv;
global_variable gl_uniform_matrix_2x3fv* glUniformMatrix2x3fv;
global_variable gl_uniform_matrix_3x2fv* glUniformMatrix3x2fv;
global_variable gl_uniform_matrix_2x4fv* glUniformMatrix2x4fv;
global_variable gl_uniform_matrix_4x2fv* glUniformMatrix4x2fv;
global_variable gl_uniform_matrix_3x4fv* glUniformMatrix3x4fv;
global_variable gl_uniform_matrix_4x3fv* glUniformMatrix4x3fv;

typedef void WINAPI gl_tex_image_3d ( GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void * data);
global_variable gl_tex_image_3d* glTexImage3D;

typedef void WINAPI gl_tex_sub_image_3d( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void * pixels);
global_variable gl_tex_sub_image_3d* glTexSubImage3D;

typedef void WINAPI gl_active_texture(GLenum texture);
global_variable gl_active_texture* glActiveTexture;

//void GLAPIENTRY ErrorCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
/* ... */
//glDebugMessageCallback( reinterpret_cast<GLDEBUGPROC>(&ErrorCallback), NULL);
typedef void (APIENTRY  *GLDEBUGPROCARB)(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam);
typedef void WINAPI debug_message_callback_arb(GLDEBUGPROCARB callback, const void* userParam);
global_variable debug_message_callback_arb* glDebugMessageCallbackARB;



#if 0
typedef void WINAPI debug_message_control_arb(enum source, enum type, enum severity, sizei count, const uint* ids, boolean enabled);
global_variable debug_message_control_arb* DebugMessageControlARB;
    void DebugMessageControlARB(enum source,
                                enum type,
                                enum severity,
                                sizei count,
                                const uint* ids,
                                boolean enabled);

    void DebugMessageInsertARB(enum source,
                               enum type,
                               uint id,
                               enum severity,
                               sizei length, 
                               const char* buf);

    
    
    uint GetDebugMessageLogARB(uint count,
                               sizei bufSize,
                               enum* sources,
                               enum* types,
                               uint* ids,
                               enum* severities,
                               sizei* lengths, 
                               char* messageLog);
    
    void GetPointerv(enum pname,
                     void** params);
#endif                     

void* _GetOpenGLFunction( char* name )
{
  void* GLProgram = wglGetProcAddress(name);
  if( (GLProgram == NULL) ||
     (GLProgram == (void*) 0x1) ||
     (GLProgram == (void*) 0x2) ||
     (GLProgram == (void*) 0x3) ||
     (GLProgram == (void*)  -1) )
  {
    INVALID_CODE_PATH
  }
  return GLProgram;
}

#define GetOpenGLFunction( type, name ) ( (type*) _GetOpenGLFunction( name ))

internal void*
Win32RenderAlloc(umm Size)
{
  void* Result = VirtualAlloc(0, Size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
  return Result;
}

internal open_gl
Win32InitOpenGL(HWND Window, HDC* ReturnWindowDC)
{
  HDC  WindowDC = GetDC(Window);
  
  PIXELFORMATDESCRIPTOR DesiredPixelFormat = {};
  DesiredPixelFormat.nSize = sizeof(PIXELFORMATDESCRIPTOR);
  DesiredPixelFormat.nVersion = 1;
  DesiredPixelFormat.dwFlags  = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
  DesiredPixelFormat.iPixelType = PFD_TYPE_RGBA;
  DesiredPixelFormat.cColorBits = 32;
  DesiredPixelFormat.cAlphaBits = 8;
  DesiredPixelFormat.cDepthBits = 24;
  DesiredPixelFormat.iLayerType = PFD_MAIN_PLANE;
  
  s32 SuggestedPixelFormatIndex = ChoosePixelFormat(WindowDC, &DesiredPixelFormat);
  PIXELFORMATDESCRIPTOR SuggestedPixelFormat;
  DescribePixelFormat(WindowDC, SuggestedPixelFormatIndex,
                      sizeof( SuggestedPixelFormat ),   &SuggestedPixelFormat );
  SetPixelFormat( WindowDC, SuggestedPixelFormatIndex, &SuggestedPixelFormat );
  
  
  HGLRC OpenGLRC = wglCreateContext(WindowDC);
  if( wglMakeCurrent(WindowDC, OpenGLRC) )
  {
    wgl_create_context_attrib_arb* wglCreateContextAttribsARB = (wgl_create_context_attrib_arb* ) wglGetProcAddress("wglCreateContextAttribsARB");
    if(wglCreateContextAttribsARB)
    {
      // Note(Jakob): This is a modern version of OpenGL
      s32 Attribs[] =
      {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
        WGL_CONTEXT_MINOR_VERSION_ARB, 0,
        WGL_CONTEXT_FLAGS_ARB, 0 // WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
#if HANDMADE_INTERNAL
        | WGL_CONTEXT_DEBUG_BIT_ARB
#endif
        ,
        WGL_CONTEXT_PROFILE_MASK_ARB,
        WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        0,
        
      };
      
      HGLRC SharedContext = 0;
      HGLRC ModernGLRC = wglCreateContextAttribsARB(WindowDC, SharedContext, Attribs);
      
      if(ModernGLRC)
      {
        if( wglMakeCurrent(WindowDC, ModernGLRC))
        {
          wglDeleteContext(OpenGLRC);
          OpenGLRC = ModernGLRC;
        }else{
          INVALID_CODE_PATH
        }
      }else{
        INVALID_CODE_PATH
      }
      
    }else{
      // Note(Jakob): This is a antiquated version of OpenGL.
      INVALID_CODE_PATH
    }
    
    glBlendEquationSeparate     = GetOpenGLFunction( gl_blend_equation_separate,     "glBlendEquationSeparate");
    glCreateProgram             = GetOpenGLFunction( gl_create_program,              "glCreateProgram");
    glLinkProgram               = GetOpenGLFunction( gl_link_program,                "glLinkProgram");
    glValidateProgram           = GetOpenGLFunction( gl_validate_program,            "glValidateProgram");
    glUseProgram                = GetOpenGLFunction( gl_use_program,                 "glUseProgram");
    glGetProgramiv              = GetOpenGLFunction( gl_get_program_iv,              "glGetProgramiv");
    glGetProgramInfoLog         = GetOpenGLFunction( gl_get_program_info_log,        "glGetProgramInfoLog");
    glCreateShader              = GetOpenGLFunction( gl_create_shader,               "glCreateShader");
    glDeleteShader              = GetOpenGLFunction( gl_delete_shader,               "glDeleteShader");
    glAttachShader              = GetOpenGLFunction( gl_attach_shader,               "glAttachShader");
    glDetachShader              = GetOpenGLFunction( gl_detach_shader,               "glDetachShader");
    glShaderSource              = GetOpenGLFunction( gl_shader_source,               "glShaderSource");
    glCompileShader             = GetOpenGLFunction( gl_compile_shader,              "glCompileShader");
    glGetShaderiv               = GetOpenGLFunction( gl_get_shader_iv,               "glGetShaderiv");
    glGetShaderInfoLog          = GetOpenGLFunction( gl_get_shader_info_log,         "glGetShaderInfoLog");
    glGenVertexArrays           = GetOpenGLFunction( gl_gen_vertex_arrays,           "glGenVertexArrays");
    glBindVertexArray           = GetOpenGLFunction( gl_bind_vertex_array,           "glBindVertexArray");
    glDeleteVertexArrays        = GetOpenGLFunction( gl_delete_vertex_arrys,         "glDeleteVertexArrays");
    glIsVertexArray             = GetOpenGLFunction( gl_is_vertex_array,             "glIsVertexArray");
    glVertexAttribPointer       = GetOpenGLFunction( gl_vertex_attrib_pointer,       "glVertexAttribPointer");
    glVertexAttribIPointer      = GetOpenGLFunction( gl_vertex_attrib_i_pointer,     "glVertexAttribIPointer");
    glVertexAttribLPointer      = GetOpenGLFunction( gl_vertex_attrib_l_pointer,     "glVertexAttribLPointer");
    glVertexAttribPointer       = GetOpenGLFunction( gl_vertex_attrib_pointer,       "glVertexAttribPointer");
    glVertexAttribIPointer      = GetOpenGLFunction( gl_vertex_attrib_i_pointer,     "glVertexAttribIPointer");
    glVertexAttribLPointer      = GetOpenGLFunction( gl_vertex_attrib_l_pointer,     "glVertexAttribLPointer");
    glEnableVertexAttribArray   = GetOpenGLFunction( gl_enable_vertex_attrib_array,  "glEnableVertexAttribArray");
    glDisableVertexAttribArray  = GetOpenGLFunction( gl_disable_vertex_attrib_array, "glDisableVertexAttribArray");
    glVertexAttribDivisor       = GetOpenGLFunction( gl_vertex_attrib_divisor,       "glVertexAttribDivisor");
    glEnableVertexArrayAttrib   = GetOpenGLFunction( gl_enable_vertex_array_attrib,  "glEnableVertexArrayAttrib");
    glDisableVertexArrayAttrib  = GetOpenGLFunction( gl_disable_vertex_array_attrib, "glDisableVertexArrayAttrib");
    glGenBuffers                = GetOpenGLFunction( gl_gen_buffers,                 "glGenBuffers");
    glBindBuffer                = GetOpenGLFunction( gl_bind_buffer,                 "glBindBuffer");
    glBufferData                = GetOpenGLFunction( gl_buffer_data,                 "glBufferData");
    glBufferSubData             = GetOpenGLFunction( gl_buffer_sub_data,             "glBufferSubData");
    glNamedBufferData           = GetOpenGLFunction( gl_named_buffer_data,           "glNamedBufferData");
    glIsBuffer                  = GetOpenGLFunction( gl_is_buffer,                   "glIsBuffer");
    glDeleteBuffers             = GetOpenGLFunction( gl_delete_buffer,               "glDeleteBuffers");
    //glDrawElementsA             = GetOpenGLFunction( gl_draw_elements,               "glDrawElements");
    glDrawElementsInstanced     = GetOpenGLFunction( gl_draw_elements_instanced,     "glDrawElementsInstanced");
    glDrawElementsBaseVertex    = GetOpenGLFunction( gl_draw_elements_base_vertex,   "glDrawElementsBaseVertex" );
    glDrawElementsInstancedBaseVertex = GetOpenGLFunction( gl_draw_elements_instanced_base_vertex,   "glDrawElementsInstancedBaseVertex" );
    glMultiDrawElements         = GetOpenGLFunction( gl_multi_draw_elements,         "glMultiDrawElements" );
    glGetUniformLocation        = GetOpenGLFunction( gl_get_uniform_location,        "glGetUniformLocation");
    glUniform1f                 = GetOpenGLFunction( gl_uniform_1f,                  "glUniform1f");
    glUniform2f                 = GetOpenGLFunction( gl_uniform_2f,                  "glUniform2f");
    glUniform3f                 = GetOpenGLFunction( gl_uniform_3f,                  "glUniform3f");
    glUniform4f                 = GetOpenGLFunction( gl_uniform_4f,                  "glUniform4f");
    glUniform1i                 = GetOpenGLFunction( gl_uniform_1i,                  "glUniform1i");
    glUniform2i                 = GetOpenGLFunction( gl_uniform_2i,                  "glUniform2i");
    glUniform3i                 = GetOpenGLFunction( gl_uniform_3i,                  "glUniform3i");
    glUniform4i                 = GetOpenGLFunction( gl_uniform_4i,                  "glUniform4i");
    glUniform1ui                = GetOpenGLFunction( gl_uniform_u1,                  "glUniform1ui");
    glUniform2ui                = GetOpenGLFunction( gl_uniform_u2,                  "glUniform2ui");
    glUniform3ui                = GetOpenGLFunction( gl_uniform_u3,                  "glUniform3ui");
    glUniform4ui                = GetOpenGLFunction( gl_uniform_u4,                  "glUniform4ui");
    glUniform1fv                = GetOpenGLFunction( gl_uniform_1fv,                 "glUniform1fv");
    glUniform2fv                = GetOpenGLFunction( gl_uniform_2fv,                 "glUniform2fv");
    glUniform3fv                = GetOpenGLFunction( gl_uniform_3fv,                 "glUniform3fv");
    glUniform4fv                = GetOpenGLFunction( gl_uniform_4fv,                 "glUniform4fv");
    glUniform1iv                = GetOpenGLFunction( gl_uniform_1iv,                 "glUniform1iv");
    glUniform2iv                = GetOpenGLFunction( gl_uniform_2iv,                 "glUniform2iv");
    glUniform3iv                = GetOpenGLFunction( gl_uniform_3iv,                 "glUniform3iv");
    glUniform4iv                = GetOpenGLFunction( gl_uniform_4iv,                 "glUniform4iv");
    glUniform1uiv               = GetOpenGLFunction( gl_uniform_1uiv,                "glUniform1uiv");
    glUniform2uiv               = GetOpenGLFunction( gl_uniform_2uiv,                "glUniform2uiv");
    glUniform3uiv               = GetOpenGLFunction( gl_uniform_3uiv,                "glUniform3uiv");
    glUniform4uiv               = GetOpenGLFunction( gl_uniform_4uiv,                "glUniform4uiv");
    glUniformMatrix2fv          = GetOpenGLFunction( gl_uniform_matrix_2fv,          "glUniformMatrix2fv");
    glUniformMatrix3fv          = GetOpenGLFunction( gl_uniform_matrix_3fv,          "glUniformMatrix3fv");
    glUniformMatrix4fv          = GetOpenGLFunction( gl_uniform_matrix_4fv,          "glUniformMatrix4fv");
    glUniformMatrix2x3fv        = GetOpenGLFunction( gl_uniform_matrix_2x3fv,        "glUniformMatrix2x3fv");
    glUniformMatrix3x2fv        = GetOpenGLFunction( gl_uniform_matrix_3x2fv,        "glUniformMatrix3x2fv");
    glUniformMatrix2x4fv        = GetOpenGLFunction( gl_uniform_matrix_2x4fv,        "glUniformMatrix2x4fv");
    glUniformMatrix4x2fv        = GetOpenGLFunction( gl_uniform_matrix_4x2fv,        "glUniformMatrix4x2fv");
    glUniformMatrix3x4fv        = GetOpenGLFunction( gl_uniform_matrix_3x4fv,        "glUniformMatrix3x4fv");
    glUniformMatrix4x3fv        = GetOpenGLFunction( gl_uniform_matrix_4x3fv,        "glUniformMatrix4x3fv");
    glTexImage3D                = GetOpenGLFunction( gl_tex_image_3d,                "glTexImage3D");
    glTexSubImage3D             = GetOpenGLFunction( gl_tex_sub_image_3d,            "glTexSubImage3D");
    glActiveTexture             = GetOpenGLFunction( gl_active_texture,              "glActiveTexture");
//  glTexStorage3D              = GetOpenGLFunction( gl_tex_storage_3d,              "glTexStorage3D");
    glDebugMessageCallbackARB   = GetOpenGLFunction( debug_message_callback_arb,     "glDebugMessageCallbackARB");
  }else{
    INVALID_CODE_PATH
  }
  //ReleaseDC(Window, WindowDC);
  GLenum error = glGetError();


  wglSwapIntervalEXT = GetOpenGLFunction( wgl_swap_interval_ext, "wglSwapIntervalEXT");
  // Sets the VSync on
  if(wglSwapIntervalEXT)
  {
    // TODO: Turning Vsync off.
    //       It seems broken, wglSwapIntervalEXT(1) slows down fps something fierce
    wglSwapIntervalEXT(0);
  }
  error = glGetError();
  open_gl OpenGL = {};
  InitOpenGL(&OpenGL);
  *ReturnWindowDC = WindowDC;
  error = glGetError();
  return OpenGL;
  
}