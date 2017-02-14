#pragma once

#include <GL/glew.h>
#include <SFML/OpenGL.hpp>

#include <vector>

#include "igElement.h"

class igElementDisplay {
public:
  igElementDisplay();

  void loadElements(const std::vector<igElement*>& visibleElmts);
  void drawElements() const;

private:
  void allocateNewBuffers(size_t nbNewBuffers);
  void fillBufferData(std::vector<igElement*> elements);
  void processSpree(const std::vector<igElement*>& visibleElmts,
    size_t& currentSpreeLength, size_t& firstIndexSpree);

  GLuint _vbo;
  std::vector<GLuint> _texIDs;
  std::vector<GLuint> _nbElemsInSpree;
};
