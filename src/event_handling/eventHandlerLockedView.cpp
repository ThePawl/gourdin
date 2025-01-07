#include "eventHandlerLockedView.h"

#include "camera.h"

#if MOBILE
  #define ROTATION_ANGLE_MOUSE (0.1f / interfaceParams.getInterfaceZoomFactor())
#else
  #define ROTATION_ANGLE_MOUSE 0.1f
#endif

#define ROTATION_ANGLE_PMS 0.06f // PMS = per millisecond
#define CHARACTER_TIME_TRANSFER_MS 100
#define CAMERA_TIME_TRANSFER_CHANGE_VIEWTYPE_MS 200

#define MAX_GROUND_ANGLE_FOR_CAM_POV (-10.f)
#define GROUND_ANGLE_TOLERANCE_GOD 15.f

EventHandlerLockedView::EventHandlerLockedView(Game& game, SDL2pp::Window& window) :
  EventHandler::EventHandler(game, window),
  _beginDrag(0),
  _previousFocusedPos(0),
  _previousCameraParams(0.f),
  _nextCameraParams(0.f),
  _maxScalarProductWithGroundPOV(-sin(RAD*MAX_GROUND_ANGLE_FOR_CAM_POV)),
  _minScalarProductWithGroundGod(-sin(RAD*GROUND_ANGLE_TOLERANCE_GOD)),
  _previousCameraLock(PreviousCameraLock::NO_LOCK) {}

void EventHandlerLockedView::handleKeyPressed(const SDL_Event& event) {
  Camera& cam = Camera::getInstance();

  glm::vec2 moveFocused = _game.getFocusedPos();
  float theta = cam.getTheta()*M_PI/180.f;

  switch(event.key.keysym.scancode) {
    case SDL_SCANCODE_1:
      if (_game.getPovCamera())
        resetCamera(false, CAMERA_TIME_TRANSFER_CHANGE_VIEWTYPE_MS);
      break;

    case SDL_SCANCODE_2:
      if (!_game.getPovCamera())
        resetCamera(true, CAMERA_TIME_TRANSFER_CHANGE_VIEWTYPE_MS);
      break;

    case SDL_SCANCODE_S:
      moveFocused += glm::vec2(cos(theta), sin(theta));
      break;

    case SDL_SCANCODE_D:
      moveFocused += glm::vec2(cos(theta+M_PI/2.f), sin(theta+M_PI/2.f));
      break;

    case SDL_SCANCODE_W:
      moveFocused += glm::vec2(cos(theta+M_PI), sin(theta+M_PI));
      break;

    case SDL_SCANCODE_A:
      moveFocused += glm::vec2(cos(theta-M_PI/2.f), sin(theta-M_PI/2.f));
      break;

    case SDL_SCANCODE_LSHIFT:
      _game.makeLionsRun();
      break;

  }

  // Switch selection to closest character in the direction given by moveFocused
  const Uint8 *keyboardState = SDL_GetKeyboardState(NULL);
  if (keyboardState[SDL_SCANCODE_SPACE] &&
      moveFocused != _game.getFocusedPos()) {

    _previousFocusedPos = _game.getFocusedPos();
    _game.changeFocusInDirection(moveFocused - _game.getFocusedPos());
    _characterTransferTimer.reset(CHARACTER_TIME_TRANSFER_MS);
  }
}

void EventHandlerLockedView::handleKeyReleased(const SDL_Event& event) {
  const Uint8 *keyboardState = SDL_GetKeyboardState(NULL);
  switch(event.key.keysym.scancode) {
    case SDL_SCANCODE_W:
    case SDL_SCANCODE_A:
    case SDL_SCANCODE_S:
    case SDL_SCANCODE_D:
    // The user is not moving the character any more
      if (!keyboardState[SDL_SCANCODE_W] &&
          !keyboardState[SDL_SCANCODE_A] &&
          !keyboardState[SDL_SCANCODE_S] &&
          !keyboardState[SDL_SCANCODE_D] &&
          !keyboardState[SDL_SCANCODE_SPACE])
        _game.stopMoving();
      break;

    case SDL_SCANCODE_LSHIFT:
      _game.stopLionsRun();
      break;
  }
}

bool EventHandlerLockedView::handleEvent(const SDL_Event& event) {
  Camera& cam = Camera::getInstance();

  switch (event.type) {
    case SDL_MULTIGESTURE:
    case SDL_MOUSEWHEEL:
      _game.setLockedView(false);
      break;

    case SDL_KEYDOWN:
      handleKeyPressed(event);
      break;

    case SDL_KEYUP:
      handleKeyReleased(event);
      break;
  }

  if (event.type == SDL_USER_CLICK) {
    glm::ivec2 windowCoords((intptr_t) event.user.data1, (intptr_t) event.user.data2);

    glm::vec2 minimapCoord = _game.getInterface().getMinimapClickCoords(glm::ivec2(
      (intptr_t) event.user.data1,
      (intptr_t) event.user.data2
    ));

    if (minimapCoord.x >= 0 && minimapCoord.x <= 1 && minimapCoord.y >= 0 && minimapCoord.y <= 1) {
      if (_game.getPovCamera())
        resetCamera(false, CAMERA_TIME_TRANSFER_CHANGE_VIEWTYPE_MS);
      else
        resetCamera(true, CAMERA_TIME_TRANSFER_CHANGE_VIEWTYPE_MS);
    }

    else {
      glm::vec2 previousPos = _game.getFocusedPos();
      bool newSelectedCharacter = _game.pickCharacter(windowCoords);

      if (newSelectedCharacter) {
        if (_game.getFocusedPos() == previousPos)
          _game.stopMoving();

        else {
          _characterTransferTimer.reset(CHARACTER_TIME_TRANSFER_MS);
          _previousFocusedPos = previousPos;
        }
      }

      else {
        if (dynamic_cast<const Lion*>(_game.getFocusedCharacter()))
          _game.moveFocused(windowCoords, true);
        else
          _game.moveFocused(windowCoords, false);
      }
    }
  }

  else if (event.type == SDL_USER_DOUBLE_CLICK)
    _game.switchLionsRun();

  else if (event.type == SDL_USER_LONG_CLICK_END)
    _game.moveFocused(glm::ivec2((intptr_t) event.user.data1, (intptr_t) event.user.data2));

  else if (event.type == SDL_USER_DRAG_BEGIN) {
    _initialDragTheta = cam.getTheta();
    _initialDragPhi = cam.getPhi();
    _beginDrag = getBeginDrag();
  }

  else if (event.type == SDL_USER_DRAG_MOTION) {
#if MOBILE
    const InterfaceParameters& interfaceParams = InterfaceParameters::getInstance();
#endif
    if (_game.getPovCamera()) {
      cam.setTheta(_initialDragTheta + ((intptr_t) event.user.data1 - _beginDrag.x) * ROTATION_ANGLE_MOUSE);
      cam.setPhi  (_initialDragPhi   + ((intptr_t) event.user.data2 - _beginDrag.y) * ROTATION_ANGLE_MOUSE);

      float theta = cam.getTheta();
      float phi = cam.getPhi();
      handleCamBoundsPOVMode(theta, phi, false);
      cam.setValues(cam.getZoom(), theta, phi);
    }

    else {
      // Dragging the camera must be coherent when the mouse goes around the central character
      // The sense of rotation depends on which quarter of the screen the cursor is
      // If the quarter changes, we reset the origin of the dragging
      if (_beginDrag.x > cam.getWindowW() / 2.f)
          cam.rotate( ((intptr_t) event.user.data2 - _beginDrag.y) * ROTATION_ANGLE_MOUSE, 0);
      else
          cam.rotate(-((intptr_t) event.user.data2 - _beginDrag.y) * ROTATION_ANGLE_MOUSE, 0);

      if (_beginDrag.y < cam.getWindowH() / 2.f)
          cam.rotate( ((intptr_t) event.user.data1 - _beginDrag.x) * ROTATION_ANGLE_MOUSE, 0);
      else
          cam.rotate(-((intptr_t) event.user.data1 - _beginDrag.x) * ROTATION_ANGLE_MOUSE, 0);

      _beginDrag.x = (intptr_t) event.user.data1;
      _beginDrag.y = (intptr_t) event.user.data2;
    }
  }

  return EventHandler::handleEvent(event);
}

void EventHandlerLockedView::handleCamBoundsGodMode(float& theta) {
  glm::vec3 normal = _game.getEngine().getNormalOnCameraPointedPos();

  // The steeper the slope, the less the user can move around
  float actualMinScalarProduct = 1 - (1 - _minScalarProductWithGroundGod) * (1-sin(acos(normal.z)));

  if (glm::dot(normal, ut::carthesian(1, theta, 90)) < actualMinScalarProduct) {
    // New theta with given phi
    std::pair<float,float> thetasLim = ut::solveAcosXplusBsinXequalC(
      normal.x, normal.y, actualMinScalarProduct);

    if (thetasLim.first == 0 && thetasLim.second == 0) {
      theta = ut::spherical(normal).y;
      return;
    }

    glm::vec3 normalSpherical = ut::spherical(normal);

    if (ut::angleIsPositive(normalSpherical.y, thetasLim.first))
      std::swap(thetasLim.first, thetasLim.second);

    if (_previousCameraLock == PreviousCameraLock::TRIGO_NEG)
      theta = thetasLim.first;
    else if (_previousCameraLock == PreviousCameraLock::TRIGO_POS)
      theta = thetasLim.second;
    else {
      std::pair<float,float> distsToThetasLim;
      distsToThetasLim.first  = ut::absDistBetweenAngles(theta, thetasLim.first);
      distsToThetasLim.second = ut::absDistBetweenAngles(theta, thetasLim.second);

      if (distsToThetasLim.first < distsToThetasLim.second) {
        _previousCameraLock = PreviousCameraLock::TRIGO_NEG;
        theta = thetasLim.first;
      }
      else {
        _previousCameraLock = PreviousCameraLock::TRIGO_POS;
        theta = thetasLim.second;
      }
    }
  }

  else if (theta != _previousTheta)
    _previousCameraLock = PreviousCameraLock::NO_LOCK;
}

void EventHandlerLockedView::handleCamBoundsPOVMode(float& theta, float& phi, bool slideCameraWhenGoingDownwards) const {
  glm::vec3 normal = _game.getEngine().getNormalOnCameraPointedPos();

  // We make the camera move only if the new pointed direction is not too close to the ground in angle
  if (glm::dot(normal, ut::carthesian(1, theta, phi)) > _maxScalarProductWithGroundPOV) {

    float phiLim = getPhiLimForGivenTheta(theta, normal, _maxScalarProductWithGroundPOV);
    float phiIsTooFarByAmount = phiLim - phi;

    if (slideCameraWhenGoingDownwards) {
      if (phiIsTooFarByAmount > 0) {
        float normalTheta = ut::spherical(normal).y;

        if (ut::angleIsPositive(normalTheta, theta))
          theta += phiIsTooFarByAmount;
        else
          theta -= phiIsTooFarByAmount;

        if (ut::angleIsPositive(normalTheta, theta) !=
            ut::angleIsPositive(normalTheta, _previousTheta) &&
            ut::absDistBetweenAngles(theta, normalTheta + 180) < 90 &&
            phi != _previousPhi)
          theta = normalTheta + 180;

        phi = getPhiLimForGivenTheta(theta, normal, _maxScalarProductWithGroundPOV);
      }
    }

    else
      phi = phiLim;
  }

  if (phi < 0)
    phi = 0;
  if (phi > 180)
    phi = 180;
}

void EventHandlerLockedView::onGoingEvents(int msElapsed) {
  EventHandler::onGoingEvents(msElapsed);

  Camera& cam = Camera::getInstance();
  const Uint8 *keyboardState = SDL_GetKeyboardState(NULL);

  float characterTransferProgress = _characterTransferTimer.getPercentageElapsed();

  if (characterTransferProgress == 1.f)
    cam.setPointedPos(_game.getFocusedPos());
  else
    cam.setPointedPos(_previousFocusedPos + characterTransferProgress *
      (_game.getFocusedPos() - _previousFocusedPos));

  float camTransferProgress = _camTransferTimer.getPercentageElapsed();

  if (camTransferProgress < 1.f) {
    glm::vec4 currentParams = _previousCameraParams + camTransferProgress *
      (_nextCameraParams - _previousCameraParams);

    cam.setValues(currentParams.x, currentParams.y, currentParams.z);
    cam.setAdditionalHeight(currentParams.w);
  }

  else if (_camTransferTimer.firstTimeFinished()) {
    cam.setValues(_nextCameraParams.x, _nextCameraParams.y, _nextCameraParams.z);
    cam.setAdditionalHeight(_nextCameraParams.w);
  }

  else {
    float theta = cam.getTheta();
    float phi   = cam.getPhi();

    if (_game.getPovCamera()) {
      if (!EventHandler::isDraggingCursor()) {
        if (keyboardState[SDL_SCANCODE_LEFT])
          theta += ROTATION_ANGLE_PMS * msElapsed;

        if (keyboardState[SDL_SCANCODE_RIGHT])
          theta -= ROTATION_ANGLE_PMS * msElapsed;

        if (keyboardState[SDL_SCANCODE_UP])
          phi += ROTATION_ANGLE_PMS * msElapsed;

        if (keyboardState[SDL_SCANCODE_DOWN])
          phi -= ROTATION_ANGLE_PMS * msElapsed;

        handleCamBoundsPOVMode(theta, phi, true);
      }
    }

    else {
      if (keyboardState[SDL_SCANCODE_E])
        theta += ROTATION_ANGLE_PMS * msElapsed;

      if (keyboardState[SDL_SCANCODE_Q])
        theta -= ROTATION_ANGLE_PMS * msElapsed;

      handleCamBoundsGodMode(theta);
    }

    cam.setValues(cam.getZoom(), theta, phi);

    if (!keyboardState[SDL_SCANCODE_SPACE]) {
      glm::vec2 moveFocused = _game.getFocusedPos();
      float thetaRad = cam.getTheta()*M_PI/180.f;

      if (keyboardState[SDL_SCANCODE_S])
        moveFocused += glm::vec2(cos(thetaRad), sin(thetaRad));
      if (keyboardState[SDL_SCANCODE_D])
        moveFocused += glm::vec2(cos(thetaRad+M_PI/2.f), sin(thetaRad+M_PI/2.f));
      if (keyboardState[SDL_SCANCODE_W])
        moveFocused += glm::vec2(cos(thetaRad+M_PI), sin(thetaRad+M_PI));
      if (keyboardState[SDL_SCANCODE_A])
        moveFocused += glm::vec2(cos(thetaRad-M_PI/2.f), sin(thetaRad-M_PI/2.f));

      if (moveFocused != _game.getFocusedPos())
        _game.setTarget(moveFocused);
    }
  }

  _previousPhi = cam.getPhi();
  _previousTheta = cam.getTheta();
}

void EventHandlerLockedView::resetCamera(bool pov, int msTransferDuration) {
  Camera& cam = Camera::getInstance();

  _game.setPovCamera(pov);

  if (pov) {
    float theta = _thetaInPreviousView;
    float phi = _phiInPreviousView;
    handleCamBoundsPOVMode(theta, phi, false);
    _nextCameraParams = glm::vec4(0.1, theta, phi, _game.getCharacterHeight());

    _thetaInPreviousView = cam.getTheta();
  }

  else {
    float theta = _thetaInPreviousView;
    handleCamBoundsGodMode(theta);
    _nextCameraParams = glm::vec4(MIN_R, theta, INIT_PHI, 0);

    _thetaInPreviousView = cam.getTheta();
    _phiInPreviousView = cam.getPhi();
  }

  float newTheta = cam.getTheta();

  if (ut::absDistBetweenAngles(_nextCameraParams.y, newTheta) < std::abs(_nextCameraParams.y - newTheta)) {
    if (_nextCameraParams.y > newTheta)
      newTheta += 360;
    else
      newTheta -= 360;
  }

  _previousCameraParams = glm::vec4(cam.getZoom(), newTheta, cam.getPhi(), cam.getAdditionalHeight());

  _camTransferTimer.reset(msTransferDuration);
  _previousFocusedPos = cam.getPointedPos();
  _characterTransferTimer.reset(msTransferDuration);
}

void EventHandlerLockedView::gainFocus() {
  Camera& cam = Camera::getInstance();

  float distanceCamHasToTravel = glm::length(glm::vec3(cam.getPointedPos() - _game.getFocusedPos(), cam.getZoom() - MIN_R));

  _thetaInPreviousView = cam.getTheta();

  resetCamera(false, std::max(CHARACTER_TIME_TRANSFER_MS, (int) (10 * std::sqrt(distanceCamHasToTravel))));

  glm::vec3 terrainNormal = ut::spherical(_game.getEngine().getNormalOnCameraPointedPos());
  _thetaInPreviousView = terrainNormal.y + 180;
  _phiInPreviousView = 90.f - cam.getFov() / 2.f;
}

float EventHandlerLockedView::getPhiLimForGivenTheta(float theta, glm::vec3 normal, float maxDotProduct) {
  return ut::solveAcosXplusBsinXequalC(
    normal.z, cos(theta*RAD)*normal.x + sin(theta*RAD)*normal.y, maxDotProduct).first;
}
