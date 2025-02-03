#pragma once

#include <glm/glm.hpp>

class InterfaceParameters	{
public:
	static const InterfaceParameters& getInstance() {
    static InterfaceParameters instance;
    return instance;
  }

	InterfaceParameters(InterfaceParameters const&) = delete;
	void operator=     (InterfaceParameters const&) = delete;

  float staminaBarWidth() const;
  float staminaBarHeight() const;
  float sizeTextSmall() const;
  float sizeTextMedium() const;
  float sizeTextBig() const;
  float loadingBarSize() const;
	float marginsSize(float textSize) const;
  glm::vec4 colorFrame() const;
  glm::vec4 colorBackground() const;
	glm::vec4 colorHighlight() const;

  inline float getInterfaceZoomFactor() const {return _interfaceZoomFactor;}

private:
	InterfaceParameters();

  float _interfaceZoomFactor = 1.f;
};
