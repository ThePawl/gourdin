#include "loadingScreen.h"

#include "camera.h"
#include "coloredRectangles.h"
#include "interfaceParameters.h"
#include "text.h"

#define PROGRESS_BAR_TEXT_SIZE interfaceParams.sizeTextMedium()
#define PROGRESS_BAR_VERTICAL_OFFSET (2.5 * textCenter.getSize().y / 2)

LoadingScreen::LoadingScreen(SDL2pp::Window& window):
  _window(window) {}

void LoadingScreen::updateAndRender(std::string task, float progress) {
  assert(progress <= 100 && progress >= 0);

  if (!_previousTask.empty())
    SDL_Log("%s: %d ms", _previousTask.c_str(), _timeSincePreviousTaskWasLaunched.getElapsedTime());

  _previousTask = task;
  _timeSincePreviousTaskWasLaunched.restart();

  Camera& cam = Camera::getInstance();
  const InterfaceParameters& interfaceParams = InterfaceParameters::getInstance();
  Text textCenter;
  textCenter.setText("Loading", interfaceParams.sizeTextBig());

  textCenter.setPosition(glm::ivec2(cam.getWindowW() / 2 - (textCenter.getSize().x / 2),
                                    cam.getWindowH() / 2 - (textCenter.getSize().y / 2)));

  Text textProgressBar;
  textProgressBar.setText(task, interfaceParams.sizeTextMedium());
  textProgressBar.setPosition(glm::ivec2(cam.getWindowW() / 2 - (textProgressBar.getSize().x / 2),
                                         cam.getWindowH() / 2 + PROGRESS_BAR_VERTICAL_OFFSET
                                         + interfaceParams.marginsSize(PROGRESS_BAR_TEXT_SIZE)));

  glm::ivec4 progressBarRect(
   cam.getWindowW() / 2 - interfaceParams.loadingBarSize() / 2,
   cam.getWindowH() / 2 + PROGRESS_BAR_VERTICAL_OFFSET,
   interfaceParams.loadingBarSize() * progress / 100.f,
   PROGRESS_BAR_TEXT_SIZE + 2 * interfaceParams.marginsSize(PROGRESS_BAR_TEXT_SIZE)
  );

  glm::ivec4 progressBarFrameRect(
    cam.getWindowW() / 2 - interfaceParams.loadingBarSize() / 2,
    cam.getWindowH() / 2 + PROGRESS_BAR_VERTICAL_OFFSET,
    interfaceParams.loadingBarSize(),
    PROGRESS_BAR_TEXT_SIZE + 2 * interfaceParams.marginsSize(PROGRESS_BAR_TEXT_SIZE)
  );

  ColoredRectangles progressBar(interfaceParams.colorBackground(), progressBarRect, true);
  ColoredRectangles progressBarFrame(interfaceParams.colorFrame(), progressBarFrameRect, false);

  glViewport(0, 0, (GLint) cam.getWindowW(), (GLint) cam.getWindowH());
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  textCenter.bindShaderAndDraw();
  progressBar.bindShaderAndDraw();
  textProgressBar.bindShaderAndDraw();
  progressBarFrame.bindShaderAndDraw();

  glViewport(0, 0, (GLint) cam.getW(), (GLint) cam.getH());

  SDL_GL_SwapWindow(_window.Get());

  // Necessary to update the loading bar on iPhone
  SDL_PumpEvents();
}
