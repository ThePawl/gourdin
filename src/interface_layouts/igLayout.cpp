#include "igLayout.h"

igLayout::igLayout():
  cam(Camera::getInstance()),
  interfaceParams(InterfaceParameters::getInstance()),
  _rectSelect(glm::vec4(1), false) {}

void igLayout::init() {
  // Minimap
  _minimapTexture.loadFromFile("res/map/map.png");
  _minimapRect.reset(new TexturedRectangle(&_minimapTexture, glm::ivec4(
    cam.getWindowW() - _minimapTexture.getSize().x * 2 * interfaceParams.getInterfaceZoomFactor(),
    cam.getWindowH() - _minimapTexture.getSize().y * 2 * interfaceParams.getInterfaceZoomFactor(),
    _minimapTexture.getSize().x * 2 * interfaceParams.getInterfaceZoomFactor(),
    _minimapTexture.getSize().y * 2 * interfaceParams.getInterfaceZoomFactor()
  )));
}

void igLayout::renderEngine() const {
  TexturedRectangle::bindDefaultShader();
  _texRectEngine->draw();
  Shader::unbind();
}

void igLayout::renderMinimap(const Engine& engine) const {
  glm::vec4 minimapTextureRect = _minimapRect->getTextureRect();

  // Background texture
  TexturedRectangle::bindDefaultShader();
  _minimapRect->draw();
  Shader::unbind();

  // Highlight visible chunks
  std::vector<glm::ivec4> greyRects;

  glm::vec2 miniChunkSize = glm::vec2(minimapTextureRect.z / (float) NB_CHUNKS,
                                      minimapTextureRect.w / (float) NB_CHUNKS);

  glm::vec4 miniChunk(minimapTextureRect.x, minimapTextureRect.y, miniChunkSize);

  for (int i = 0; i < NB_CHUNKS; i++) {
    miniChunk.y = minimapTextureRect.y;
    for (int j = 0; j < NB_CHUNKS; j++) {

      if (!engine.isChunkVisible(i, NB_CHUNKS - 1 - j))
        greyRects.push_back(miniChunk);

      miniChunk.y += miniChunkSize.y;
    }
    miniChunk.x += miniChunkSize.x;
  }

  ColoredRectangles grey(glm::vec4(0,0,0,0.4), greyRects);
  grey.bindShaderAndDraw();

  // Position of the viewer

  glm::vec2 viewerPos( minimapTextureRect.x + (float) minimapTextureRect.z * cam.getPointedPos().x / MAX_COORD,
                       minimapTextureRect.y + (float) minimapTextureRect.w * (1 - cam.getPointedPos().y / MAX_COORD));

  glm::vec2 viewer = glm::vec2(minimapTextureRect.z, minimapTextureRect.w) / 10.f;

  ColoredRectangles opaqueGrey(glm::vec4(0.2,0.2,0.2,1), std::vector<glm::ivec4>(1,
    glm::ivec4(viewerPos.x - viewer.x/2.f,
               viewerPos.y - viewer.y/2.f,
               viewer)
  ));

  opaqueGrey.bindShaderAndDraw();

  // Texture frame
  ColoredRectangles frame(interfaceParams.colorFrame(), minimapTextureRect, false);
  frame.bindShaderAndDraw();
}

void igLayout::renderText() const {
  _textTopLeft.bindShaderAndDraw();
  _textTopRight.bindShaderAndDraw();
  _textTopCenter.bindShaderAndDraw();

  if (_textCenterChrono.isStillRunning())
    _textCenter.bindShaderAndDraw();

  if (_textBottomCenterChrono.isStillRunning())
    _textBottomCenter.bindShaderAndDraw();
}

void igLayout::setTextTopLeft(const std::string& string) {
  _textTopLeft.setText(string, interfaceParams.sizeTextSmall());
}

void igLayout::setTextTopRight(const std::string& string) {
  _textTopRight.setText(string, interfaceParams.sizeTextSmall());
  _textTopRight.setPosition(glm::ivec2(cam.getWindowW() - _textTopRight.getSize().x, 0));
}

void igLayout::setTextTopCenter(const std::string& string) {
    _textTopCenter.setText(string, interfaceParams.sizeTextSmall());
    _textTopCenter.setPosition(glm::ivec2(cam.getWindowW() / 2 - _textTopCenter.getSize().x / 2, 0));
}

void igLayout::setTextCenter(const std::string& string, int msDuration) {
  _textCenter.setText(string, interfaceParams.sizeTextBig());
  _textCenter.setPosition(glm::ivec2(cam.getWindowW() / 2 - _textCenter.getSize().x / 2,
                                     cam.getWindowH() / 2 - _textCenter.getSize().y / 2));

  _textCenterChrono.reset(msDuration);
}

void igLayout::setTextBottomCenter(const std::string& string, int msDuration) {
  _textBottomCenter.setText(string, interfaceParams.sizeTextMedium());
  _textBottomCenter.setPosition(glm::ivec2(cam.getWindowW() / 2 - _textBottomCenter.getSize().x / 2,
                                           cam.getWindowH() - _textBottomCenter.getSize().y * 2));

  _textBottomCenterChrono.reset(msDuration);
}

void igLayout::renderRectSelect() const {
  _rectSelect.bindShaderAndDraw();
}

void igLayout::renderStaminaBars(std::set<Lion*> selection) const {
  std::vector<glm::ivec4> staminaBarsRects;
  std::vector<glm::ivec4> outlinesRects;

  for(auto it = selection.begin(); it != selection.end(); ++it) {
    glm::ivec4 corners = (*it)->getScreenRect();

    // Otherwise the selected element is outside the screen
    if (corners.z != 0) {
      float maxHeightFactor = (*it)->getMaxHeightFactor(); // The lifeBar must not change when switching animations

      staminaBarsRects.push_back(glm::ivec4(
        corners.x + corners.z/2 - interfaceParams.staminaBarWidth() / 2.f,
        corners.y - corners.w*maxHeightFactor + corners.w - interfaceParams.staminaBarWidth() / 4.f,
        interfaceParams.staminaBarWidth() * (*it)->getStamina() / 100.f,
        interfaceParams.staminaBarHeight()
      ));

      outlinesRects.push_back(glm::ivec4(
        corners.x + corners.z/2 - interfaceParams.staminaBarWidth() / 2.f,
        corners.y - corners.w*maxHeightFactor + corners.w - interfaceParams.staminaBarWidth() / 4.f,
        interfaceParams.staminaBarWidth(),
        interfaceParams.staminaBarHeight()
      ));
    }
  }

  ColoredRectangles staminaBars(glm::vec4(0,1,0,1), staminaBarsRects);
  ColoredRectangles outlines(glm::vec4(0,0,0,1), outlinesRects, false);
  staminaBars.bindShaderAndDraw();
  outlines.bindShaderAndDraw();
}

glm::vec2 igLayout::getMinimapClickCoords(const glm::ivec2& clickPos) const {
  glm::ivec4 minimapTexRect = _minimapRect->getTextureRect();

  glm::vec2 coords = glm::vec2( (clickPos.x - minimapTexRect.x) / (float) minimapTexRect.z,
                                (clickPos.y - minimapTexRect.y) / (float) minimapTexRect.w);

  return coords;
}

void igLayout::setRectSelect(glm::ivec4 rect) {
  glm::ivec4 absoluteRect;
  if (rect.z > 0) {
    absoluteRect.x = rect.x;
    absoluteRect.z = rect.z;
  }
  else {
    absoluteRect.x = rect.x + rect.z;
    absoluteRect.z = - rect.z;
  }

  if (rect.w > 0) {
    absoluteRect.y = rect.y;
    absoluteRect.w = rect.w;
  }
  else {
    absoluteRect.y = rect.y + rect.w;
    absoluteRect.w = - rect.w;
  }

  _rectSelect.setRectangles(absoluteRect);
}
