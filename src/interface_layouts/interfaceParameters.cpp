#include "interfaceParameters.h"

#include <SDL.h>

#ifdef __ANDROID__
  #include <jni.h>

  #define NORMALIZATION_DPI_VALUE 537.882019

  #define STAMINA_BAR_WIDTH (60.f * _androidInterfaceZoomFactor)
  #define STAMINA_BAR_HEIGHT (8.f * _androidInterfaceZoomFactor)

  #define SIZE_TEXT_SMALL (30.f * _androidInterfaceZoomFactor)
  #define SIZE_TEXT_MEDIUM (38.f * _androidInterfaceZoomFactor)
  #define SIZE_TEXT_BIG (90.f * _androidInterfaceZoomFactor)

  #define LOADING_BAR_SIZE (1000.f * _androidInterfaceZoomFactor)

#else
  #define STAMINA_BAR_WIDTH 20.f
  #define STAMINA_BAR_HEIGHT 4.f

  #define SIZE_TEXT_SMALL 10
  #define SIZE_TEXT_MEDIUM 14
  #define SIZE_TEXT_BIG 33

  #define LOADING_BAR_SIZE 500.f
#endif

#define MARGINS_FACTOR (_androidInterfaceZoomFactor * 1.f/3.f)

#define COLOR_FRAME glm::vec4(0.52, 0.34, 0.138, 1)
#define COLOR_BACKGROUND glm::vec4(205 / 256.f, 157 / 256.f, 102 / 256.f, 0.70)
#define COLOR_HIGHLIGHT  glm::vec4(205 / 256.f, 157 / 256.f, 102 / 256.f, 0.90)

InterfaceParameters::InterfaceParameters():
  _screenHorizontalDPI(0),
  _androidInterfaceZoomFactor(1) {
#ifdef __ANDROID__
  // Get the screen DPI from the android application
  JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
  jobject activity = (jobject)SDL_AndroidGetActivity();
  jclass clazz(env->GetObjectClass(activity));
  jmethodID method_id = env->GetMethodID(clazz, "getXDPI", "()F");

  _screenHorizontalDPI = env->CallFloatMethod(activity, method_id);

  env->DeleteLocalRef(activity);
  env->DeleteLocalRef(clazz);
  _androidInterfaceZoomFactor = _screenHorizontalDPI / NORMALIZATION_DPI_VALUE;
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
