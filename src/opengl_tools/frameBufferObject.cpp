#include "frameBufferObject.h"

#include <SDL_log.h>

GLuint FrameBufferObject::defaultFBOID = -1;

FrameBufferObject::FrameBufferObject() :
  _fboID(0) {
  glGenFramebuffers(1, &_fboID);
}

FrameBufferObject::~FrameBufferObject () {
  glDeleteFramebuffers(1, &_fboID);
}

void FrameBufferObject::setDefaultFBOID(GLuint id) {
  if (defaultFBOID == -1) {
    defaultFBOID = id;
  }
}

FrameBufferObject::FrameBufferObject(FrameBufferObject&& other) noexcept:
  _fboID(other._fboID),
  _colorBuffer(std::move(other._colorBuffer)),
  _depthBuffer(std::move(other._depthBuffer)) {}

void FrameBufferObject::init(size_t width, size_t height,
  GLenum colorBufferInternalFormat, GLenum colorBufferFormat, GLenum colorBufferType) {

  _colorBuffer.bind();
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, colorBufferInternalFormat, width, height, 0, colorBufferFormat, colorBufferType, 0);

   _depthBuffer.bind();
   glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height,
             0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  Texture::unbind();

  bind();
  _colorBuffer.attachToBoundFBO(GL_COLOR_ATTACHMENT0);
  _depthBuffer.attachToBoundFBO(GL_DEPTH_ATTACHMENT);

  if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Error in FrameBufferObject::init, unable to create FBO");
    switch (glCheckFramebufferStatus(GL_FRAMEBUFFER)) {
      case GL_FRAMEBUFFER_UNDEFINED:                     SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "GL_FRAMEBUFFER_UNDEFINED"); break;
      case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:         SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT"); break;
      case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"); break;
      case GL_FRAMEBUFFER_UNSUPPORTED:                   SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "GL_FRAMEBUFFER_UNSUPPORTED"); break;
      case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE"); break;
#ifndef GL_ES_VERSION_3_0
      case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER"); break;
      case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER"); break;
      case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS"); break;
#endif
    }
  }

  unbind();
}
