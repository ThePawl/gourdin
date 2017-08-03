#include "event_handler.h"
#include "camera.h"
#include "clock.h"
#include "utils.h"

#define DOUBLECLICK_MS 300

Uint32 EventHandler::SDL_USER_FINGER_CLICK = SDL_RegisterEvents(1);
Uint32 EventHandler::SDL_USER_FINGER_DOUBLE_CLICK = SDL_RegisterEvents(1);
size_t EventHandler::_nbFingers = 0;

EventHandler::EventHandler(Game& game):
  _beginDragLeft(DEFAULT_OUTSIDE_WINDOW_COORD),
  _pendingClick(DEFAULT_OUTSIDE_WINDOW_COORD),
  _game(game) {}

bool EventHandler::handleEvent(const SDL_Event& event, EventHandlerType& currentHandler) {
  Camera& cam = Camera::getInstance();
  bool running = true;

  switch (event.type) {
    case SDL_QUIT:
      running = false;
      break;

    case SDL_WINDOWEVENT:
      if (event.window.event == SDL_WINDOWEVENT_RESIZED)
        cam.resize(event.window.data1, event.window.data2);
      break;

    case SDL_FINGERDOWN:
      _nbFingers++;

      if (getNbFingers() == 1 && _pendingClick == DEFAULT_OUTSIDE_WINDOW_COORD) {
        _beginDragTouch.x = (event.tfinger.x * cam.getWindowW());
        _beginDragTouch.y = (event.tfinger.y * cam.getWindowH());
        _doubleClickBegin.restart();
      }
      break;

    case SDL_FINGERUP: {
      _nbFingers--;

      glm::vec2 beginDragTouch(_beginDragTouch);
      glm::vec2 releasedPos(event.tfinger.x * cam.getWindowW(), event.tfinger.y * cam.getWindowH());

      if (_nbFingers == 0 && _doubleClickBegin.getElapsedTime() < DOUBLECLICK_MS &&
        glm::length(beginDragTouch - releasedPos) < MAX_DIST_FOR_CLICK) {

        glm::vec2 pendingClick(_pendingClick);

        if (_pendingClick == DEFAULT_OUTSIDE_WINDOW_COORD)
          _pendingClick = _beginDragTouch;

        else if (glm::length(pendingClick - releasedPos) < MAX_DIST_FOR_CLICK) {
          SDL_Event doubleClickEvent;
          SDL_zero(doubleClickEvent);
          doubleClickEvent.type = SDL_USER_FINGER_DOUBLE_CLICK;

          doubleClickEvent.user.data1 = (void*) ((uintptr_t) _pendingClick.x);
          doubleClickEvent.user.data2 = (void*) ((uintptr_t) _pendingClick.y);
          SDL_PushEvent(&doubleClickEvent);

          _pendingClick = DEFAULT_OUTSIDE_WINDOW_COORD;
        }
      }
    }
    break;

    case SDL_KEYDOWN:
      switch(event.key.keysym.sym) {
        case SDLK_ESCAPE:
          running = false;
          break;

        case SDLK_l:
          _game.switchLog();
          break;

        case SDLK_m:
          if (currentHandler == HDLR_GAME)
            currentHandler = HDLR_SANDBOX;
          else
            currentHandler = HDLR_GAME;
          break;

        case SDLK_p:
          if (Clock::isGlobalTimerPaused())
            Clock::resumeGlobalTimer();
          else
            Clock::pauseGlobalTimer();
          break;
      }
      break;

    case SDL_MOUSEBUTTONDOWN:
      if (event.button.button == SDL_BUTTON_LEFT)
        _beginDragLeft = glm::ivec2(event.button.x, event.button.y);
      break;

    case SDL_MOUSEBUTTONUP:
      if (event.button.button == SDL_BUTTON_LEFT)
        _beginDragLeft = DEFAULT_OUTSIDE_WINDOW_COORD;
      break;
  }

  return running;
}

void EventHandler::onGoingEvents(int msElapsed) {
  if (_pendingClick != DEFAULT_OUTSIDE_WINDOW_COORD && _doubleClickBegin.getElapsedTime() > DOUBLECLICK_MS) {
    SDL_Event clickEvent;
    SDL_zero(clickEvent);
    clickEvent.type = SDL_USER_FINGER_CLICK;

    clickEvent.user.data1 = (void*) ((uintptr_t) _pendingClick.x);
    clickEvent.user.data2 = (void*) ((uintptr_t) _pendingClick.y);
    SDL_PushEvent(&clickEvent);
    _pendingClick = DEFAULT_OUTSIDE_WINDOW_COORD;
  }

  (void) msElapsed;
}

std::pair<float, float> EventHandler::solveAcosXplusBsinXequalC(float a, float b, float c) {
  // The code is a transcription of Wolfram Alpha solution
  std::pair<float,float> res(0,0);

  if (a == -c) {
    res.first = 180;
    res.second = 180;
  }

  else {
    float underRoot = a*a + b*b - c*c;
    if (underRoot >= 0) {
      res.first  = 2 * atan((b - sqrt(underRoot)) / (a + c)) / RAD;
      res.second = 2 * atan((b + sqrt(underRoot)) / (a + c)) / RAD;

      // Results are in range (-180, 180], make them in range [0,360)
      if (res.first < 0)
        res.first += 360;
      if (res.second < 0)
        res.second += 360;
    }

    // Ensure the ascending order
    if (res.second < res.first)
      std::swap(res.first,res.second);

  }
  return res;
}

float EventHandler::absDistBetweenAngles(float a, float b) {
  return std::min(std::abs(a-b),std::abs(std::abs(a-b)-360));
}

void EventHandler::makeThetaFitInAllowedZone(float& theta, const glm::vec3& normal, float minDotProduct) {
  if (glm::dot(normal, ut::carthesian(1, theta, 90)) < minDotProduct) {
    // New theta with given phi
    std::pair<float,float> thetasLim = solveAcosXplusBsinXequalC(
      normal.x, normal.y, minDotProduct);

    std::pair<float,float> distsToThetasLim;
    distsToThetasLim.first  = absDistBetweenAngles(theta, thetasLim.first);
    distsToThetasLim.second = absDistBetweenAngles(theta, thetasLim.second);

    if (distsToThetasLim.first < distsToThetasLim.second)
      theta = thetasLim.first;
    else
      theta = thetasLim.second;
  }
}
