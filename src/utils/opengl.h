#pragma once

#ifdef _WIN32

  #include <GL/glew.h>

  #ifdef _MSC_VER
    #include <windows.h>
  #endif

  #define GL_GLEXT_PROTOTYPES
  #include <GL/gl.h>

#elif __ANDROID__

  #include <GLES3/gl3.h>

#elif __linux__

  #define GL_GLEXT_PROTOTYPES
  #include <GL/gl.h>

#elif __APPLE__
  #define GL_SILENCE_DEPRECATION
  #include "TargetConditionals.h"

  #if TARGET_OS_IPHONE

    #include <OpenGLES/ES3/gl3.h>

  #elif TARGET_OS_MAC

    #include <OpenGL/OpenGLAvailability.h>
    #include <OpenGL/gl3ext.h>
    #include <OpenGL/gl3.h>

  #endif
#endif

#define BUFFER_OFFSET(a) ((char*)NULL + (a))
