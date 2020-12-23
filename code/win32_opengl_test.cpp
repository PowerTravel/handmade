
/*

  If not building via visual studios console window you need to invoke vcvarsall.bat x64 before compiling
  Visual studio can put vcvarsall.bat at different locations, but this is mine for vs 2019:
  call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

  Compile with: 

  cl /Zi -nologo win32_opengl_test.cpp /link user32.lib gdi32.lib opengl32.lib

  Note: It worked for ginkgobitter on a Ryzen processor. @ him when you find the solution.
*/


#include <windows.h>
#include <gl/gl.h>
#include <stdint.h>

#define WGL_CONTEXT_MAJOR_VERSION_ARB             0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB             0x2092
#define WGL_CONTEXT_FLAGS_ARB                     0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB              0x9126
#define WGL_CONTEXT_DEBUG_BIT_ARB                 0x0001
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB          0x00000001
#define WGL_DRAW_TO_WINDOW_ARB                    0x2001
#define WGL_ACCELERATION_ARB                      0x2003
#define WGL_SUPPORT_OPENGL_ARB                    0x2010
#define WGL_DOUBLE_BUFFER_ARB                     0x2011
#define WGL_PIXEL_TYPE_ARB                        0x2013
#define WGL_COLOR_BITS_ARB                        0x2014
#define WGL_ALPHA_BITS_ARB                        0x201B
#define WGL_DEPTH_BITS_ARB                        0x2022
#define WGL_STENCIL_BITS_ARB                      0x2023
#define WGL_FULL_ACCELERATION_ARB                 0x2027
#define WGL_TYPE_RGBA_ARB                         0x202B
#define WGL_SAMPLE_BUFFERS_ARB                    0x2041
#define WGL_SAMPLES_ARB                           0x2042
#define GL_FRAGMENT_SHADER                        0x8B30
#define GL_VERTEX_SHADER                          0x8B31
#define GL_COMPILE_STATUS                         0x8B81
#define GL_INFO_LOG_LENGTH                        0x8B84
#define GL_LINK_STATUS                            0x8B82
#define GL_VALIDATE_STATUS                        0x8B83
#define GL_ARRAY_BUFFER                           0x8892
#define GL_STATIC_DRAW                            0x88E4
#define GL_DEBUG_OUTPUT                           0x92E0

bool GlobalRunning = true;

static LRESULT CALLBACK
MainWindowCallback( HWND Window,
          UINT Message,
          WPARAM WParam,
          LPARAM LParam)
{
  LRESULT Result = 0;
  switch(Message)
  {
    // User x-out
    case WM_CLOSE:
    {
      GlobalRunning = false;
    }break;
    default:
    {
      // Windows default window message handling.
      Result = DefWindowProc(Window, Message, WParam, LParam);
    }break;
  }

  return Result;
}

void* _GetOpenGLFunction( char* name )
{
  void* GLProgram = wglGetProcAddress(name);
  if( (GLProgram == NULL) ||
     (GLProgram == (void*) 0x1) ||
     (GLProgram == (void*) 0x2) ||
     (GLProgram == (void*) 0x3) ||
     (GLProgram == (void*)  -1) )
  {
    exit(1);
  }
  return GLProgram;
}

#define GetOpenGLFunction( type, name ) ( (type*) _GetOpenGLFunction( name ))

typedef uint64_t GLuint64;
typedef int64_t GLint64;
typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;
typedef char GLchar;

typedef BOOL  WINAPI wgl_swap_interval_ext(int interval);
typedef HGLRC WINAPI wgl_create_context_attrib_arb(HDC hDC, HGLRC hSharedContext, const int *attribList);
typedef BOOL  WINAPI wgl_choose_pixel_format_arb(HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);

typedef GLuint WINAPI gl_create_program(void);
typedef void   WINAPI gl_link_program(GLuint program);
typedef void   WINAPI gl_use_program(GLuint program);
typedef void   WINAPI gl_validate_program(GLuint program);
typedef void   WINAPI gl_get_program_iv(GLuint program, GLenum pname, GLint *params);
typedef void   WINAPI gl_get_program_info_log(GLuint program, GLsizei maxLength, GLsizei *length, GLchar *infoLog);
typedef GLuint WINAPI gl_create_shader(  GLenum shaderType );
typedef void   WINAPI gl_delete_shader(  GLuint shader );
typedef void   WINAPI gl_attach_shader(  GLuint program, GLuint shader );
typedef void   WINAPI gl_detach_shader(  GLuint program, GLuint shader );
typedef void   WINAPI gl_shader_source(  GLuint shader,  GLsizei count, const GLchar** string, const GLint* length );
typedef void   WINAPI gl_compile_shader( GLuint shader );
typedef void   WINAPI gl_get_shader_iv ( GLuint shader, GLenum pname, GLint *params);
typedef void   WINAPI gl_get_shader_info_log(GLuint shader, GLsizei maxLength, GLsizei *length, GLchar *infoLog);
typedef void   WINAPI gl_gen_vertex_arrays(GLsizei n, GLuint *array);
typedef void   WINAPI gl_bind_vertex_array(GLuint array);
typedef void   WINAPI gl_vertex_attrib_pointer( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid * pointer);
typedef void   WINAPI gl_enable_vertex_attrib_array( GLuint index);
typedef void   WINAPI gl_disable_vertex_array_attrib( GLuint vaobj, GLuint index);
typedef void   WINAPI gl_gen_buffers(GLsizei n, GLuint *arrays);
typedef void   WINAPI gl_bind_buffer(GLenum target, GLuint buffer);
typedef void   WINAPI gl_buffer_data( GLenum target, GLsizeiptr size, const GLvoid * data, GLenum usage );

static wgl_swap_interval_ext*           wglSwapIntervalEXT;
static gl_create_program*               glCreateProgram;
static gl_link_program*                 glLinkProgram;
static gl_use_program*                  glUseProgram;
static gl_validate_program*             glValidateProgram;
static gl_get_program_iv*               glGetProgramiv;
static gl_get_program_info_log*         glGetProgramInfoLog;
static gl_create_shader*                glCreateShader;
static gl_delete_shader*                glDeleteShader;
static gl_attach_shader*                glAttachShader;
static gl_detach_shader*                glDetachShader;
static gl_shader_source*                glShaderSource;
static gl_compile_shader*               glCompileShader;
static gl_get_shader_iv*                glGetShaderiv;
static gl_get_shader_info_log*          glGetShaderInfoLog;
static gl_gen_vertex_arrays*            glGenVertexArrays;
static gl_bind_vertex_array*            glBindVertexArray;
static gl_vertex_attrib_pointer*        glVertexAttribPointer;
static gl_enable_vertex_attrib_array*   glEnableVertexAttribArray;
static gl_disable_vertex_array_attrib*  glDisableVertexArrayAttrib;
static gl_gen_buffers*                  glGenBuffers;
static gl_bind_buffer*                  glBindBuffer;
static gl_buffer_data*                  glBufferData;
static wgl_choose_pixel_format_arb      wglChoosePixelFormatARB;


typedef void (APIENTRY  *GLDEBUGPROCARB)(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam);
typedef void WINAPI gl_debug_message_callback_arb(GLDEBUGPROCARB callback, const void* userParam);
static gl_debug_message_callback_arb* glDebugMessageCallbackARB;

void ErrorCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
{
  OutputDebugStringA(message);
}

bool InitOpenGL(HINSTANCE Instance, HINSTANCE PrevInstance, HWND* ReturnWindow, HGLRC* ReturnOpenGLRC, HDC* ReturnWindowDC)
{
  WNDCLASSA WindowClass = {};
  WindowClass.style = CS_HREDRAW|CS_VREDRAW;//|CS_OWNDC;
  WindowClass.lpfnWndProc = MainWindowCallback;
  WindowClass.hInstance = PrevInstance;
  WindowClass.hCursor = LoadCursor(0, IDC_ARROW);
  WindowClass.lpszClassName = "OpenGLTest";

  if(!RegisterClass(&WindowClass))
    return false;

   HWND DummyWindow = CreateWindowEx(
      0,
      WindowClass.lpszClassName,
      "OpenGLTest",
      WS_OVERLAPPEDWINDOW|WS_VISIBLE,
      0, 0,
      1, 1,
      NULL, NULL,
      Instance,NULL);

  HDC DummyDC = GetDC(DummyWindow);
  
  PIXELFORMATDESCRIPTOR DummyPixelFormat{};
  DummyPixelFormat.nSize = sizeof(PIXELFORMATDESCRIPTOR);
  DummyPixelFormat.nVersion = 1;
  DummyPixelFormat.dwFlags  = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
  DummyPixelFormat.iPixelType = PFD_TYPE_RGBA;
  DummyPixelFormat.cColorBits = 32;
  DummyPixelFormat.cAlphaBits = 8;
  DummyPixelFormat.cDepthBits = 24;

  int DummyPixelFormatID = ChoosePixelFormat(DummyDC, &DummyPixelFormat);
  if(!DummyPixelFormatID)
    return false;

  if(!SetPixelFormat( DummyDC, DummyPixelFormatID, &DummyPixelFormat ))
    return false;

  HGLRC DummyRC = wglCreateContext(DummyDC);
  if(!wglMakeCurrent(DummyDC, DummyRC) )
    return false;

  wgl_choose_pixel_format_arb*   wglChoosePixelFormatARB = (wgl_choose_pixel_format_arb* ) wglGetProcAddress("wglChoosePixelFormatARB");
  wgl_create_context_attrib_arb* wglCreateContextAttribsARB = (wgl_create_context_attrib_arb* ) wglGetProcAddress("wglCreateContextAttribsARB");

  ////////////////////// Second Pass ////////////////////

  HWND Window = CreateWindowEx(
      0,
      WindowClass.lpszClassName,
      "OpenGLTest",
      WS_OVERLAPPEDWINDOW|WS_VISIBLE,
      CW_USEDEFAULT, // X  CW_USEDEFAULT means the system decides for us.
      CW_USEDEFAULT, // Y
      CW_USEDEFAULT, // W
      CW_USEDEFAULT, // H
      0,
      0,
      Instance,
      NULL);

  HDC WindowDC = GetDC(Window);

  const int pixelAttribs[] = {
    WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
    WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
    WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
    WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
    WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
    WGL_COLOR_BITS_ARB, 32,
    WGL_ALPHA_BITS_ARB, 8,
    WGL_DEPTH_BITS_ARB, 24,
    WGL_STENCIL_BITS_ARB, 8,
    WGL_SAMPLE_BUFFERS_ARB, GL_TRUE,
    WGL_SAMPLES_ARB, 4,
    0
  };


  int PixelFormatID;
  uint32_t NrFormats;
  const bool status = wglChoosePixelFormatARB(WindowDC, pixelAttribs, NULL, 1, &PixelFormatID, &NrFormats);
  if (status == false || NrFormats == 0)
    return false;

  PIXELFORMATDESCRIPTOR PixelFormat;
  DescribePixelFormat(WindowDC, PixelFormatID, sizeof(PIXELFORMATDESCRIPTOR), &PixelFormat);
  SetPixelFormat(WindowDC, PixelFormatID, &PixelFormat);

  int Attribs[] =
  {
    WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
    WGL_CONTEXT_MINOR_VERSION_ARB, 0,
    WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
    WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
    0,
  };
  
  HGLRC OpenGLRC = wglCreateContextAttribsARB(WindowDC, 0, Attribs);
  if(!OpenGLRC)
    return false;

  wglMakeCurrent(NULL, NULL);
  wglDeleteContext(DummyRC);
  ReleaseDC(DummyWindow, DummyDC);
  DestroyWindow(DummyWindow);

  if(!wglMakeCurrent(WindowDC, OpenGLRC))
    return false;

  glCreateProgram            = GetOpenGLFunction(gl_create_program,               "glCreateProgram");
  glLinkProgram              = GetOpenGLFunction(gl_link_program,                 "glLinkProgram");
  glUseProgram               = GetOpenGLFunction(gl_use_program,                  "glUseProgram");
  glValidateProgram          = GetOpenGLFunction(gl_validate_program,             "glValidateProgram");
  glGetProgramiv             = GetOpenGLFunction(gl_get_program_iv,               "glGetProgramiv");
  glGetProgramInfoLog        = GetOpenGLFunction(gl_get_program_info_log,         "glGetProgramInfoLog");
  glCreateShader             = GetOpenGLFunction(gl_create_shader,                "glCreateShader");
  glDeleteShader             = GetOpenGLFunction(gl_delete_shader,                "glDeleteShader");
  glAttachShader             = GetOpenGLFunction(gl_attach_shader,                "glAttachShader");
  glDetachShader             = GetOpenGLFunction(gl_detach_shader,                "glDetachShader");
  glShaderSource             = GetOpenGLFunction(gl_shader_source,                "glShaderSource");
  glCompileShader            = GetOpenGLFunction(gl_compile_shader,               "glCompileShader");
  glGetShaderiv              = GetOpenGLFunction(gl_get_shader_iv,                "glGetShaderiv");
  glGetShaderInfoLog         = GetOpenGLFunction(gl_get_shader_info_log,          "glGetShaderInfoLog");
  glGenVertexArrays          = GetOpenGLFunction(gl_gen_vertex_arrays,            "glGenVertexArrays");
  glBindVertexArray          = GetOpenGLFunction(gl_bind_vertex_array,            "glBindVertexArray");
  glVertexAttribPointer      = GetOpenGLFunction(gl_vertex_attrib_pointer,        "glVertexAttribPointer");
  glEnableVertexAttribArray  = GetOpenGLFunction(gl_enable_vertex_attrib_array,   "glEnableVertexAttribArray");
  glDisableVertexArrayAttrib = GetOpenGLFunction(gl_disable_vertex_array_attrib,  "glDisableVertexArrayAttrib");
  glGenBuffers               = GetOpenGLFunction(gl_gen_buffers,                  "glGenBuffers");
  glBindBuffer               = GetOpenGLFunction(gl_bind_buffer,                  "glBindBuffer");
  glBufferData               = GetOpenGLFunction(gl_buffer_data,                  "glBufferData");
  wglSwapIntervalEXT         = GetOpenGLFunction(wgl_swap_interval_ext,           "wglSwapIntervalEXT");
  glDebugMessageCallbackARB  = GetOpenGLFunction(gl_debug_message_callback_arb,   "glDebugMessageCallbackARB");

  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallbackARB(ErrorCallback, 0); 
  wglSwapIntervalEXT(1);

  float R = 0x1E / (float) 0xFF;
  float G = 0x46 / (float) 0xFF;
  float B = 0x5A / (float) 0xFF;
  glClearColor(R,G,B, 1.f);

  *ReturnWindow = Window;
  *ReturnOpenGLRC = OpenGLRC;
  *ReturnWindowDC = WindowDC;

  return true;
}


static char DebugVertexShaderCode[] = R"FOO(
#version  330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
out vec3 ourColor;
void main()
{
  ourColor = aColor;
  gl_Position = vec4(aPos,1.0f);
}
)FOO";
  
static char* DebugFragmentShaderCode = R"FOO(
#version 330 core
out vec4 fragColor;
in vec3 ourColor;
void main() 
{
  fragColor = vec4(ourColor,1.0f);
}
)FOO";

inline static void OpenGLPrintProgramLog(GLuint Program)
{
  GLint ProgramMessageSize;
  glGetProgramiv( Program, GL_INFO_LOG_LENGTH, &ProgramMessageSize );
  char* ProgramMessage = (char*) VirtualAlloc(0, ProgramMessageSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  glGetProgramInfoLog(Program, ProgramMessageSize, NULL, (GLchar*) ProgramMessage);
  OutputDebugStringA(ProgramMessage);
  VirtualFree(ProgramMessage, 0, MEM_RELEASE);
}

inline static void OpenGLPrintShaderLog(GLuint Shader)
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
    exit(2);
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
    exit(3);
  }

  glValidateProgram(Program);
  GLint Valid;
  glGetProgramiv(Program, GL_VALIDATE_STATUS, &Valid);
  if(!Valid)
  {
    OpenGLPrintProgramLog(Program);
    exit(4);
  }

  glDetachShader(Program, VertexShader);
  glDetachShader(Program, FragmentShader);
  
  glDeleteShader(VertexShader);
  glDeleteShader(FragmentShader);
  
  
  return Program;
}


void CheckAndPrintProgramValidity(GLuint Program)
{
  static int frameIndex = 0;
  glValidateProgram(Program);
  GLint Valid;
  static bool stopAfterBecomingInvalid = false;
  glGetProgramiv(Program, GL_VALIDATE_STATUS, &Valid);
  char Buf[512];
  if(!stopAfterBecomingInvalid)
  {
    if(!Valid)
    {
      OpenGLPrintProgramLog(Program);
      stopAfterBecomingInvalid = true;

    }
    wsprintf(Buf, "Frame : %d  Program %d is %s\n", frameIndex++, Program, Valid ? "valid" : "invalid");
    OutputDebugStringA(Buf);
    
  }

  // This flushes the output buffer
  OutputDebugStringA("");
}

GLuint UploadTriangleToGPU()
{
  GLuint VAO = 0;
  GLuint VBO = 0;
    float vertices[] = {
      // positions         // colors
       0.5f, -0.5f, 0.0f,  1.0f, 0.0f, 0.0f,
      -0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,
       0.0f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f
  };
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);
  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), (GLvoid*) vertices, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (GLvoid*) 0); 
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (GLvoid*) (3*sizeof(float)));

  return VAO;
}

void PollInput()
{
  MSG Message;
  while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
  {
    switch(Message.message)
    {
      case WM_SYSKEYDOWN:
      case WM_KEYDOWN:
      {
        switch(Message.wParam)
        {
          case VK_ESCAPE:
          case 'Q':
          { 
            GlobalRunning = false;
          }break;
        }
      }break;
      
      default:
      {
        TranslateMessage(&Message);
        DispatchMessage(&Message);
      }break;
    }
  }
}


int CALLBACK
WinMain(  HINSTANCE Instance,
          HINSTANCE PrevInstance,
          LPSTR CommandLine,
          int ShowCode )
{
  HWND Window;
  HDC DeviceContext;
  HGLRC OpenGLRC;
  
  if(!InitOpenGL(Instance, PrevInstance, &Window, &OpenGLRC, &DeviceContext))
  {
    exit(5);
  }

  if (Window != NULL)
  {
    GLuint Program = OpenGLCreateProgram(DebugVertexShaderCode, DebugFragmentShaderCode);
    glUseProgram(Program);
    GLuint VAO = UploadTriangleToGPU();
    glBindVertexArray(VAO);

    while(GlobalRunning)
    {
      PollInput();

      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      
      // Prints the Frame Number, Program handle and if it's valid or not.
      // Stops printing after program becomes invalid
      CheckAndPrintProgramValidity(Program);

      glDrawArrays(GL_TRIANGLES, 0, 3);
      SwapBuffers(DeviceContext);
      Sleep(32);
    }
  }

  return 0;
}

