#include "eventHandler.h"
#include "camera.h"
#include "clock.h"
#include "utils.h"

Uint32 EventHandler::SDL_USER_CLICK = SDL_RegisterEvents(1);
Uint32 EventHandler::SDL_USER_DOUBLE_CLICK = SDL_RegisterEvents(1);
Uint32 EventHandler::SDL_USER_LONG_CLICK_BEGIN = SDL_RegisterEvents(1);
Uint32 EventHandler::SDL_USER_LONG_CLICK_MOTION = SDL_RegisterEvents(1);
Uint32 EventHandler::SDL_USER_LONG_CLICK_END = SDL_RegisterEvents(1);
Uint32 EventHandler::SDL_USER_DRAG_BEGIN = SDL_RegisterEvents(1);
Uint32 EventHandler::SDL_USER_DRAG_MOTION = SDL_RegisterEvents(1);
Uint32 EventHandler::SDL_USER_DRAG_END = SDL_RegisterEvents(1);
bool EventHandler::_duringLongClick = false;
bool EventHandler::_duringDrag = false;
bool EventHandler::_cantSendLongClick = false;
bool EventHandler::_mouseDown = false;
size_t EventHandler::_nbFingers = 0;

EventHandler::EventHandler(Game& game, SDL2pp::Window& window):
  _game(game),
  _window(window),
  _fullscreen(true),
  _beginDrag(DEFAULT_OUTSIDE_WINDOW_COORD),
  _pendingClick(DEFAULT_OUTSIDE_WINDOW_COORD),
  _gonnaBeDoubleClick(false) {

  SDL_SetEventFilter(EventHandler::HandleAppEvents, NULL);
}

bool EventHandler::isCloseEnoughToBeginClickToDefineClick(glm::ivec2 pos) const {
  glm::vec2 beginDrag(_beginDrag);
  glm::vec2 posFloat(pos);

  return glm::length(beginDrag - posFloat) < MAX_DIST_FOR_CLICK;
}

void EventHandler::sendEvent(Uint32 type, glm::ivec2 pos) const {
  SDL_Event event;
  SDL_zero(event);
  event.type = type;

  event.user.data1 = (void*) ((intptr_t) pos.x);
  event.user.data2 = (void*) ((intptr_t) pos.y);
  SDL_PushEvent(&event);
}

bool EventHandler::handleEvent(const SDL_Event& event) {
  bool running = true;

  switch (event.type) {
    case SDL_QUIT:
      running = false;
      break;

    case SDL_WINDOWEVENT:
      if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
        Camera& cam = Camera::getInstance();
        cam.resize(event.window.data1, event.window.data2);
      }
      break;

    case SDL_FINGERDOWN:
      _nbFingers++;
      if (_nbFingers >= 2)
        _cantSendLongClick = true;
      break;

    case SDL_FINGERUP:
      _nbFingers--;
      if (_nbFingers == 0)
        _cantSendLongClick = false;
      break;

    case SDL_MOUSEBUTTONDOWN:
      if (event.button.which == SDL_TOUCH_MOUSEID || event.button.button == SDL_BUTTON_LEFT)
        _mouseDown = true;

      _beginDrag.x = event.button.x;
      _beginDrag.y = event.button.y;

      if (_pendingClick != DEFAULT_OUTSIDE_WINDOW_COORD) {

        if (_clickBegin.getElapsedTime() < DOUBLECLICK_MS &&
          isCloseEnoughToBeginClickToDefineClick(_pendingClick)) {

          _gonnaBeDoubleClick = true;
        }
      }

      _clickBegin.restart();
      break;

    case SDL_MOUSEMOTION:
      _currentCursorPos.x = event.button.x;
      _currentCursorPos.y = event.button.y;

      if (_mouseDown && getNbFingers() < 2) {
        if (isDuringLongClick())
          sendEvent(SDL_USER_LONG_CLICK_MOTION, _currentCursorPos);

        else if (!isCloseEnoughToBeginClickToDefineClick(_currentCursorPos) && !isDraggingCursor()) {
          sendEvent(SDL_USER_DRAG_BEGIN, _currentCursorPos);
          _duringDrag = true;
        }

        else if (isDraggingCursor())
          sendEvent(SDL_USER_DRAG_MOTION, _currentCursorPos);
      }
      break;

    case SDL_MOUSEBUTTONUP: {
      if (event.button.which == SDL_TOUCH_MOUSEID || event.button.button == SDL_BUTTON_LEFT) {
        _mouseDown = false;

        glm::ivec2 releasedPos(event.button.x, event.button.y);

        if (isCloseEnoughToBeginClickToDefineClick(releasedPos)) {

          if (_clickBegin.getElapsedTime() < LONGCLICK_MS) {

            if (_gonnaBeDoubleClick) {
              sendEvent(SDL_USER_DOUBLE_CLICK, _pendingClick);
              _pendingClick = DEFAULT_OUTSIDE_WINDOW_COORD;
            }

            else {
              sendEvent(SDL_USER_CLICK, _beginDrag);
              _pendingClick = _beginDrag;
            }
          }
        }

        if (isDuringLongClick()) {
          sendEvent(SDL_USER_LONG_CLICK_END, releasedPos);

          _pendingClick = DEFAULT_OUTSIDE_WINDOW_COORD;
          _duringLongClick = false;
        }

        else if (isDraggingCursor()) {
          sendEvent(SDL_USER_DRAG_END, releasedPos);
          _duringDrag = false;
        }

        _beginDrag = DEFAULT_OUTSIDE_WINDOW_COORD;
      }
    }

    _gonnaBeDoubleClick = false;
    break;

    case SDL_KEYDOWN:
      switch(event.key.keysym.sym) {
        case SDLK_ESCAPE:
          running = false;
          break;

#ifdef __ANDROID__
        case SDLK_AC_BACK:
          running = false;
          break;
#endif

        case SDLK_f:
          if (_fullscreen) {
            SDL_SetWindowFullscreen(_window.Get(), 0);
            SDL_SetWindowGrab(_window.Get(), SDL_FALSE);
          }
          else {
            SDL_SetWindowFullscreen(_window.Get(), SDL_WINDOW_FULLSCREEN_DESKTOP);
            SDL_SetWindowGrab(_window.Get(), SDL_TRUE);
          }
          _fullscreen = !_fullscreen;
          break;

        case SDLK_l:
          _game.switchLog();
          break;

        case SDLK_p:
          if (Clock::isGlobalTimerPaused())
            Clock::resumeGlobalTimer();
          else
            Clock::pauseGlobalTimer();
          break;
      }
      break;
  }

  // userEventLog(event);

  return running;
}

void EventHandler::onGoingEvents(int msElapsed) {
  if (!isDuringLongClick() && _mouseDown && !_cantSendLongClick) {
    if (_clickBegin.getElapsedTime() > LONGCLICK_MS && !isDraggingCursor()) {

      sendEvent(SDL_USER_LONG_CLICK_BEGIN, _currentCursorPos);
      _duringLongClick = true;
    }
  }

  (void) msElapsed;
}

void EventHandler::userEventLog(const SDL_Event& event) const {
  if (event.type == SDL_USER_CLICK)
    SDL_Log("Click: (%d,%d)", (int) ((intptr_t) event.user.data1), (int) ((intptr_t) event.user.data2));
  if (event.type == SDL_USER_DOUBLE_CLICK)
    SDL_Log("Double click: (%d,%d)", (int) ((intptr_t) event.user.data1), (int) ((intptr_t) event.user.data2));
  if (event.type == SDL_USER_LONG_CLICK_BEGIN)
    SDL_Log("Long click begin: (%d,%d)", (int) ((intptr_t) event.user.data1), (int) ((intptr_t) event.user.data2));
  if (event.type == SDL_USER_LONG_CLICK_MOTION)
    SDL_Log("Long click motion: (%d,%d)", (int) ((intptr_t) event.user.data1), (int) ((intptr_t) event.user.data2));
  if (event.type == SDL_USER_LONG_CLICK_END)
    SDL_Log("Long click end: (%d,%d)", (int) ((intptr_t) event.user.data1), (int) ((intptr_t) event.user.data2));
  if (event.type == SDL_USER_DRAG_BEGIN)
    SDL_Log("Drag begin: (%d,%d)", (int) ((intptr_t) event.user.data1), (int) ((intptr_t) event.user.data2));
  if (event.type == SDL_USER_DRAG_MOTION)
    SDL_Log("Drag motion: (%d,%d)", (int) ((intptr_t) event.user.data1), (int) ((intptr_t) event.user.data2));
  if (event.type == SDL_USER_DRAG_END)
    SDL_Log("Drag end: (%d,%d)", (int) ((intptr_t) event.user.data1), (int) ((intptr_t) event.user.data2));
}

int EventHandler::HandleAppEvents(void *userdata, SDL_Event *event) {
  (void) userdata;

  switch (event->type) {
    case SDL_APP_DIDENTERFOREGROUND:
      Clock::resumeGlobalTimer();
      return 0;

    case SDL_APP_WILLENTERBACKGROUND:
      Clock::pauseGlobalTimer();
      return 0;

    default:
        return 1;
  }
}
