#include "controller.h"

#include <set>
#include <string>
#include <sstream>

#include "camera.h"
#include "clock.h"
#include "lion.h"

#include <SDL.h>

Controller::Controller(SDL2pp::Window& window) :
  _running(true),
  _loadingScreen(window),
  _engine(),
  _game(_engine),
  _eventHandlerLockedView(_game, window),
  _eventHandlerGlobalView(_game, window),
  _window(window) {

  Camera& cam = Camera::getInstance();
  cam.setWindowSize(window.GetWidth(), window.GetHeight());

  float highDPIScaleX, highDPIScaleY;

#ifdef _WIN32
  const float systemDefaultDPI = 96.f;
  float dpi;
  if (SDL_GetDisplayDPI(0, NULL, &dpi, NULL) != 0)
  {
    // Failed to get DPI, so just return the default value.
    dpi = systemDefaultDPI;
  }
  highDPIScaleX = highDPIScaleY = dpi / systemDefaultDPI;
#else
  int drawableWidth, drawableHeight;
  int windowWidth, windowHeight;

  SDL_GetWindowSize(window.Get(), &windowWidth, &windowHeight);
  SDL_GL_GetDrawableSize(window.Get(), &drawableWidth, &drawableHeight);

  highDPIScaleX = (float)drawableWidth / windowWidth;
  highDPIScaleY = (float)drawableHeight / windowHeight;
#endif

  cam.setHighDPIScale(highDPIScaleX, highDPIScaleY);
  cam.resizeGameViewport(window.GetWidth() * highDPIScaleX, window.GetHeight() * highDPIScaleY);
  cam.reset();
}

void Controller::init() {
  _engine.init(_loadingScreen);
  _game.init(_loadingScreen);
}


void Controller::run() {
  Clock frameClock;
  bool previousViewWasLocked = false;

  while (_running) {
    _msElapsed = frameClock.restart();

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (_game.isViewLocked())
        _running = _eventHandlerLockedView.handleEvent(event);
      else
        _running = _eventHandlerGlobalView.handleEvent(event);

      if (!_running)
        break;
    }

    if (_game.isViewLocked() != previousViewWasLocked) {
      if (_game.isViewLocked())
        _eventHandlerLockedView.gainFocus();
      else
        _eventHandlerGlobalView.gainFocus();

      previousViewWasLocked = _game.isViewLocked();
    }

    if (_game.isViewLocked())
      _eventHandlerLockedView.onGoingEvents(_msElapsed);
    else
      _eventHandlerGlobalView.onGoingEvents(_msElapsed);

    _game.update(_msElapsed);
    _game.render();
    SDL_GL_SwapWindow(_window.Get());
  }
}
