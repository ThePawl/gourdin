#pragma once

#include <set>

#include "camera.h"
#include "chronometer.h"
#include "engine.h"
#include "opengl.h"

#include "interfaceParameters.h"

#include "coloredRectangles.h"
#include "text.h"
#include "texture.h"
#include "texturedRectangle.h"

class igLayout {
public:
  igLayout();
  void init();

  void renderEngine() const;
  void renderMinimap(const Engine& engine) const;
  void renderText() const;
  void renderRectSelect() const;
  void renderStaminaBars(std::set<Lion*> selection) const;

  glm::vec2 getMinimapClickCoords(const glm::ivec2& clickPos) const;
  void setTextTopLeft(const std::string& string);
  void setTextTopRight(const std::string& string);
  void setTextTopCenter(const std::string& string);
  void setTextCenter(const std::string& string, int msDuration = -1);
  void setTextBottomCenter(const std::string& string, int msDuration = -1);
  void clearText();

  inline void setEngineTexture(const Texture* tex) {_texRectEngine.reset(new TexturedRectangle(tex, Camera::getInstance().getWindowRect()));}
  void setRectSelect(glm::ivec4 rect);

private:
  const Camera& cam;
  const InterfaceParameters& interfaceParams;

  ColoredRectangles _rectSelect;
  Text _textTopLeft;
  Text _textTopRight;
  Text _textTopCenter;
  Text _textCenter;
  Text _textBottomCenter;

  Texture _minimapTexture;
  std::unique_ptr<TexturedRectangle> _minimapRect;

  Chronometer _textCenterChrono;
  Chronometer _textBottomCenterChrono;

  std::unique_ptr<const TexturedRectangle> _texRectEngine;
};
