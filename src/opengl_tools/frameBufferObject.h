#pragma once

#include "texture.h"

class FrameBufferObject {
public:
  FrameBufferObject ();

  void init(size_t width, size_t height,
    GLenum colorBufferInternalFormat, GLenum colorBufferFormat, GLenum colorBufferType);

  inline const Texture* getColorBuffer() const {return &_colorBuffer;}
  inline const Texture* getDepthTexture() const {return &_depthBuffer;}

  inline void bind() const {glBindFramebuffer(GL_FRAMEBUFFER, _fboIndex);}
  static void unbind() {glBindFramebuffer(GL_FRAMEBUFFER, 0);}

private:
  GLuint _fboIndex;
  Texture _colorBuffer;
  Texture _depthBuffer;
};
