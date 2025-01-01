#pragma once

#include "texture.h"

#include <stddef.h> // size_t

class FrameBufferObject : public GLObjectInterface {
public:
  FrameBufferObject ();
  FrameBufferObject (FrameBufferObject&& other) noexcept;
  ~FrameBufferObject ();

  void init(size_t width, size_t height,
    GLenum colorBufferInternalFormat, GLenum colorBufferFormat, GLenum colorBufferType);

  inline const Texture* getColorBuffer() const {return &_colorBuffer;}
  inline const Texture* getDepthTexture() const {return &_depthBuffer;}

  static void setDefaultFBOID(GLuint id);

  inline void bind() const {glBindFramebuffer(GL_FRAMEBUFFER, _fboID);}
  static void unbind() { glBindFramebuffer(GL_FRAMEBUFFER, defaultFBOID); }

private:
  static GLuint defaultFBOID;

  GLuint _fboID;
  Texture _colorBuffer;
  Texture _depthBuffer;
};
