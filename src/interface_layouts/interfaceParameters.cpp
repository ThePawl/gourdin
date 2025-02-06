#include "interfaceParameters.h"
#include "utils.h"

#include <SDL.h>

#ifdef __ANDROID__
  #include <jni.h>

  #define NORMALIZATION_DPI_VALUE 537.882019
#endif

#if MOBILE
  #define STAMINA_BAR_WIDTH (60.f * _interfaceZoomFactor)
  #define STAMINA_BAR_HEIGHT (2.f * _interfaceZoomFactor)

  #define SIZE_TEXT_SMALL (30.f * _interfaceZoomFactor)
  #define SIZE_TEXT_MEDIUM (38.f * _interfaceZoomFactor)
  #define SIZE_TEXT_BIG (90.f * _interfaceZoomFactor)

  #define LOADING_BAR_SIZE (1300.f * _interfaceZoomFactor)

#else
  #define STAMINA_BAR_WIDTH (20.f * _interfaceZoomFactor)
  #define STAMINA_BAR_HEIGHT (4.f * _interfaceZoomFactor)

  #define SIZE_TEXT_SMALL (10 * _interfaceZoomFactor)
  #define SIZE_TEXT_MEDIUM (14 * _interfaceZoomFactor)
  #define SIZE_TEXT_BIG (33 * _interfaceZoomFactor)

  #define LOADING_BAR_SIZE (500.f * _interfaceZoomFactor)
#endif

#define MARGINS_FACTOR (_interfaceZoomFactor * 1.f/3.f)

#define COLOR_FRAME glm::vec4(0.52, 0.34, 0.138, 1)
#define COLOR_BACKGROUND glm::vec4(205 / 256.f, 157 / 256.f, 102 / 256.f, 0.70)
#define COLOR_HIGHLIGHT  glm::vec4(205 / 256.f, 157 / 256.f, 102 / 256.f, 0.90)

InterfaceParameters::InterfaceParameters() {
#ifdef __ANDROID__
  // Get the screen DPI from the android application
  JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
  jobject activity = (jobject)SDL_AndroidGetActivity();
  jclass clazz(env->GetObjectClass(activity));
  jmethodID method_id = env->GetMethodID(clazz, "getXDPI", "()F");

  float screenHorizontalDPI = env->CallFloatMethod(activity, method_id);

  env->DeleteLocalRef(activity);
  env->DeleteLocalRef(clazz);
  _interfaceZoomFactor = screenHorizontalDPI / NORMALIZATION_DPI_VALUE;
#elif defined(_WIN32)
  const float systemDefaultDPI = 96.f;
  float dpi;
  if (SDL_GetDisplayDPI(0, NULL, &dpi, NULL) != 0)
  {
    // Failed to get DPI, so just return the default value.
    dpi = systemDefaultDPI;
  }
  _interfaceZoomFactor = dpi / systemDefaultDPI;
#endif
}

float InterfaceParameters::staminaBarWidth() const {return STAMINA_BAR_WIDTH;}
float InterfaceParameters::staminaBarHeight() const {return STAMINA_BAR_HEIGHT;}
float InterfaceParameters::sizeTextSmall() const {return SIZE_TEXT_SMALL;}
float InterfaceParameters::sizeTextMedium() const {return SIZE_TEXT_MEDIUM;}
float InterfaceParameters::sizeTextBig() const {return SIZE_TEXT_BIG;}
float InterfaceParameters::loadingBarSize() const {return LOADING_BAR_SIZE;}
float InterfaceParameters::marginsSize(float textSize) const {return textSize * MARGINS_FACTOR;}
glm::vec4 InterfaceParameters::colorFrame() const {return COLOR_FRAME;}
glm::vec4 InterfaceParameters::colorBackground() const {return COLOR_BACKGROUND;}
glm::vec4 InterfaceParameters::colorHighlight() const {return COLOR_HIGHLIGHT;}
