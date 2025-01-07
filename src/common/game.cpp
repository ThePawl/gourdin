#include "game.h"
#include "camera.h"
#include "log.h"

#include <sstream>

#define LION_MIN_SPAWN_DIST 20

Game::Game (Engine& engine):
  _focusedCharacter(nullptr),
  _lockedView(false),
  _displayLog(true),
  _engine(engine),
  _interface(),
  _popupMenu(this),
  _povCamera(false),
  _huntHasStarted(false),
  _maxSimultaneousLions(5),
  _nbLions(0),
  _bestScore(0),
  _msHuntDuration(120000),
  _msCenterTextDisplayDuration(1000) {}

void Game::init(LoadingScreen& loadingScreen) {
  loadingScreen.updateAndRender("Initializing interface", 80);
  _interface.setEngineTexture(_engine.getColorBuffer());
  _interface.init();

  loadingScreen.updateAndRender("Waiting for subdivision tasks to finish", 85);

  setLockedView(false);

#if !MOBILE
  _interface.setTextTopCenter("Best score: 0");
#endif

  _popupMenu.addEntry("Predator", &Game::createLion);
  _popupMenu.addEntry("Tribe", &Game::genTribe);

  update(0);
  _engine.waitForTasksToFinish();
  loadingScreen.updateAndRender("Finished initialization", 100);
}

void Game::updateCamera() const {
  Camera& cam = Camera::getInstance();

  if (!_lockedView)
    cam.setAdditionalHeight(0);

  cam.setHeight(_engine.getHeight(cam.getPointedPos()));
  cam.apply();

  // Check if the camera is in the ground and adjust accordingly
  if (!_lockedView) {
    float heightOnCameraRealPos = _engine.getHeight(cam.getProjectedPos());
    if (heightOnCameraRealPos > cam.getPos().z - 20) {
      cam.setAdditionalHeight(heightOnCameraRealPos - cam.getPos().z + 20);
      cam.apply();
    }
  }
}

void Game::update(int msElapsed) {
  Log& logText = Log::getInstance();
  logText.addFPSandCamInfo();

  if (_displayLog)
    _interface.setTextTopRight(logText.getText());
  else
    _interface.setTextTopRight("");

  logText.clear();

#ifndef GL_ES_VERSION_3_0
  if (Clock::isGlobalTimerPaused())
    _interface.setTextCenter("PAUSED", 1);
#endif

  // Check if the hunt has ended
  if (_huntHasStarted && _huntStart.getElapsedTime() > _msHuntDuration)
    interruptHunt();

  if (_huntHasStarted)
    _interface.setTextTopRight(getHuntText());

  // Remove the dead elements from the selected elements
  std::vector<Lion*> toDelete;
  for (auto it = _selection.begin(); it != _selection.end(); it++) {
   if ((*it)->isDead())
     toDelete.push_back(*it);
  }
  for (int i = 0; i < toDelete.size(); i++) {
   _selection.erase(toDelete[i]);
  }

  if (_focusedCharacter != nullptr && _focusedCharacter->isDead())
    _focusedCharacter = nullptr;

  updateCamera();
  _engine.update(msElapsed);
}

void Game::render() const {
  Camera& cam = Camera::getInstance();

  _engine.renderToFBO();

  glViewport(0, 0, (GLint) cam.getWindowW(), (GLint) cam.getWindowH());
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  _interface.renderEngine();
  _interface.renderStaminaBars(_selection);

  if (!_lockedView)
    _interface.renderRectSelect();

  _interface.renderMinimap(_engine);
  _interface.renderText();

  _popupMenu.bindShadersAndDraw();

  glViewport(0, 0, (GLint) cam.getW(), (GLint) cam.getH());
}

void Game::setLockedView(bool lockedView) {
  _lockedView = lockedView;

  if (_lockedView) {
    setFocusedCharacter(_focusedCharacter);
    _interface.setTextTopLeft(getInfoTextLockedView());
  }

  else
    _interface.setTextTopLeft(getInfoTextGlobalView());
}

std::string Game::getInfoTextCommon() const {
  std::ostringstream text;

#if !MOBILE
  text << "Esc: " << "Quit engine" << std::endl
       << "P: " << "Pause" << std::endl
       << "F: " << "Fullscreen/Windowed" << std::endl
       << "L: " << "Hide/Display log" << std::endl;
#endif

  return text.str();
}

std::string Game::getInfoTextLockedView() const {
  std::ostringstream text;

#if !MOBILE
  text << "1: " << "God camera" << std::endl
       << "2: " << "POV camera" << std::endl;
  if (_povCamera)
    text << "Arrows: " << "Move camera around" << std::endl;
  else
    text << "A-E:  " << "Rotate camera" << std::endl;
  text << "WASD: " << "Move focused character" << std::endl
       << "LShift: " << "Make the focused character run" << std::endl
       << "SPACE+WASD: " << "Change focused character to closest in given direction" << std::endl
       << std::endl
       << "Click to move the character in the center" << std::endl
       << "Click on another character to change the focus" << std::endl
       << "Click and drag to rotate the camera" << std::endl;
#else
  text << "Click to make the character move in the given direction" << std::endl
       << "Long click to make the character move to the click" << std::endl
       << "Double click to make a predator run" << std::endl
       << "Click on another character to change focus" << std::endl;
  if (_povCamera)
    text << "Click on the minimap to switch back to the external camera" << std::endl;
  else
    text << "Click on the focused character to stop moving" << std::endl
         << "Click on the minimap to switch to POV camera" << std::endl;
#endif

  return getInfoTextCommon() + text.str();
}

std::string Game::getInfoTextGlobalView() const {
  std::ostringstream text;

#if !MOBILE
  text << "Left-Right / A-D: " << "Rotate camera" << std::endl
       << "Up-Down    / W-S: " << "Go forwards/backwards" << std::endl
       << "A/Return: " << "Select all lions" << std::endl
       << "E: " << "Change scroll speed" << std::endl
       << "T: " << "Create tribe" << std::endl;
  if (!_huntHasStarted)
    text << "H: " << "Start new hunt!" << std::endl;
  else
    text << "H: " << "Interrupt current hunt" << std::endl;
  text << "Z: " << "Switch to wireframe display" << std::endl
       << std::endl
       << "Click on the minimap to change position" << std::endl
       << "Right-click to create a lion" << std::endl
       << "Select it with the left mouse button" << std::endl
       << "Move it around with the right button" << std::endl
       << "Lshift/Rshift: " << "Make it run" << std::endl
       << "Delete: " << "Kill selected lion" << std::endl
       << std::endl
       << "Go hunt them juicy antilopes!" << std::endl;
#else
  text << "Long click to make a predator appear" << std::endl
       << "Click on it to select it" << std::endl
       << "Double click to go back to last selected predator" << std::endl;
#endif

  return getInfoTextCommon() + text.str();
}

std::string Game::getHuntText() const {
  std::ostringstream text;

  text << "Predators: " << _nbLions << "/" << _maxSimultaneousLions << std::endl
       << "Kills: " << Lion::getNbKilled() << std::endl
       << "Time left: " << (_msHuntDuration - _huntStart.getElapsedTime()) / 1000 << std::endl;

  return text.str();
}

void Game::changeFocusInDirection(glm::vec2 direction) {
  Camera& cam = Camera::getInstance();
  float threshold = sqrt(2)/2.f;

  Controllable* closestMovingElement = _focusedCharacter;
  float closestDist = MAX_COORD;

  for (auto ctrl = _engine.getControllableElements().begin(); ctrl != _engine.getControllableElements().end(); ctrl++) {
    glm::vec2 toChar = (*ctrl)->getPos() - _focusedCharacter->getPos();
    float distance = glm::length(toChar);

    // Character is in the right direction, with +- M_PI/4 margin
    if (glm::dot(toChar, direction)/distance > threshold) {

      // Checks whether the character is visible on the screen
      glm::ivec4 screenRect = (*ctrl)->getScreenRect();
      if (screenRect.y > (int) cam.getH())
        continue;
      if (screenRect.x > (int) cam.getW())
        continue;
      if (screenRect.y + screenRect.w < 0)
        continue;
      if (screenRect.x + screenRect.z < 0)
        continue;


      if (distance < closestDist) {
        closestDist = distance;
        closestMovingElement = (*ctrl);
      }
    }
  }

  setFocusedCharacter(closestMovingElement);
}

void Game::setFocusedCharacter(Controllable* focusedCharacter) {
  _focusedCharacter = focusedCharacter;

  _selection.clear();
  Lion* focusedLion = dynamic_cast<Lion*>(_focusedCharacter);
  if (focusedLion)
    _selection.insert(focusedLion);
}

bool Game::pickCharacter(glm::ivec2 screenTarget) {
  for (auto ctrl = _engine.getControllableElements().begin(); ctrl != _engine.getControllableElements().end(); ctrl++) {

    glm::ivec4 spriteRect = (*ctrl)->getScreenRect();

    if (ut::contains(spriteRect,screenTarget)) {
      setFocusedCharacter(*ctrl);
      return true;
    }
  }

  return false;
}

void Game::moveFocused(glm::ivec2 screenTarget, bool perpetualMotion) {
  if (_focusedCharacter) {
    if (perpetualMotion)
      _focusedCharacter->setMovingDirection(_engine.get2DCoord(screenTarget) - _focusedCharacter->getPos());
    else
      _focusedCharacter->setTarget(_engine.get2DCoord(screenTarget));
  }
}

void Game::select(glm::ivec4 rect, bool add) {
  if (!add)
    _selection.clear();

  const std::set<Controllable*> controllableElements = _engine.getControllableElements();
  for (auto it = controllableElements.begin(); it != controllableElements.end(); it++) {
    Lion *lion = dynamic_cast<Lion*>(*it);
    if (lion && !lion->isDead()) {
      // controllableElements[i] is not selected yet, we can bother to calculate
      if (_selection.find(lion) == _selection.end()) {
        glm::ivec4 SpriteRect = lion->getScreenRect();

        int centerX, centerY;

        centerX = SpriteRect.x + SpriteRect.z / 2;
        centerY = SpriteRect.y + SpriteRect.w / 2;

        if (ut::contains(rect, glm::ivec2(centerX, centerY)))
          _selection.insert(lion);

        else if (   ut::contains(SpriteRect, glm::ivec2(rect.x, rect.y)) ||
                    ut::contains(SpriteRect, glm::ivec2(rect.x + rect.z, rect.y)) ||
                    ut::contains(SpriteRect, glm::ivec2(rect.x + rect.z, rect.y + rect.w)) ||
                    ut::contains(SpriteRect, glm::ivec2(rect.x, rect.y + rect.w))  ) {
          _selection.insert(lion);
        }
      }
    }
  }
}

void Game::selectAllLions() {
  _selection.clear();
  const std::set<Controllable*> controllableElements = _engine.getControllableElements();
  for (auto it = controllableElements.begin(); it != controllableElements.end(); it++) {
    Lion *lion = dynamic_cast<Lion*>(*it);
    if (lion && !lion->isDead()) {
      _selection.insert(lion);
    }
  }
}

void Game::createLion(glm::ivec2 screenTarget) {
  try {
    if (!_huntHasStarted)
      _engine.addLion(screenTarget);

    else {
      if (_nbLions >= _maxSimultaneousLions)
        throw std::runtime_error("Maximum number of predators reached");

      _engine.addLion(screenTarget, LION_MIN_SPAWN_DIST);
      _nbLions++;
    }
    displayInfo("Created predator");

  } catch (const std::runtime_error& e) {
    displayInfo(e.what());
  }
}

void Game::moveSelection(glm::ivec2 screenTarget) {
  glm::vec2 target = _engine.get2DCoord(screenTarget);

  for(auto it = _selection.begin(); it != _selection.end(); ++it) {
    Controllable* ctrl = dynamic_cast<Controllable*>(*it);
    if (ctrl) {
      ctrl->setTarget(target);
    }
  }
}

void Game::goBackToSelection() {
  if (!_selection.empty()) {
    glm::vec2 barycenter;
    float nbSelected = 0;

    for (auto it = _selection.begin(); it != _selection.end(); ++it) {
      barycenter += (*it)->getPos();
      nbSelected++;
    }
    Camera& cam = Camera::getInstance();
    cam.setPointedPos(barycenter / nbSelected);
  }
}

void Game::makeLionsRun() {
  for (auto it = _selection.begin(); it != _selection.end(); ++it) {
    (*it)->beginRunning();
  }
}

void Game::stopLionsRun() {
  for (auto it = _selection.begin(); it != _selection.end(); ++it) {
    (*it)->beginWalking();
  }
}

void Game::switchLionsRun() {
  bool makeThemAllRun = false;
  bool generalStrategyChosen = false;
  for (auto it = _selection.begin(); it != _selection.end(); ++it) {
    if (!generalStrategyChosen) {
      generalStrategyChosen = true;
      makeThemAllRun = !(*it)->isRunning();
    }
    if (generalStrategyChosen) {
      if (makeThemAllRun)
        (*it)->beginRunning();
      else
        (*it)->beginWalking();
    }
  }
}

void Game::killLion() {
  if (!_selection.empty()) {
    (*_selection.begin())->die();
    _nbLions--;
  }
}

void Game::benchmark() {
  Camera& cam = Camera::getInstance();

  cam.setPointedPos(glm::vec2(CHUNK_SIZE / 2, CHUNK_SIZE / 2));
  cam.setValues(5000, 225.f, INIT_PHI);

  Clock frameClock(ClockType::INDEPENDENT);

  for (int i = 0; i < 100; i++) {
    cam.setPointedPos(glm::vec2(CHUNK_SIZE / 2 + i / 100.f * CHUNK_SIZE * (NB_CHUNKS-1),
                                CHUNK_SIZE / 2 + i / 100.f * CHUNK_SIZE * (NB_CHUNKS-1)));
    update(0);
    render();
  }

  int msTotalElapsed = frameClock.getElapsedTime();

  SDL_Log("Rendered 100 frames in %d milliseconds.",  msTotalElapsed);
  SDL_Log("Average FPS: %f", 1.f / msTotalElapsed * 100 * 1000);

  cam.reset();
}

void Game::interruptHunt() {
  if (_huntHasStarted) {
    _huntHasStarted = false;
    if (Lion::getNbKilled() > _bestScore)
      _bestScore = Lion::getNbKilled();

    Log& logText = Log::getInstance();
    logText.clear();

    std::ostringstream bestScoreText;
    bestScoreText << "Best score: " << _bestScore;
    _interface.setTextTopCenter(bestScoreText.str());

    _interface.setTextTopRight("");

    if (_lockedView)
      _interface.setTextTopLeft(getInfoTextLockedView());
    else
      _interface.setTextTopLeft(getInfoTextGlobalView());

    std::ostringstream scoreText;
    scoreText << "Kills: " << Lion::getNbKilled();
    _interface.setTextCenter(scoreText.str(), _msCenterTextDisplayDuration);
  }
}

void Game::startNewHunt() {
  if (!_huntHasStarted) {
    std::vector<igMovingElement*> toDelete;

    const std::set<Controllable*>& lions = _engine.getControllableElements();
    for (auto it = lions.begin(); it != lions.end(); it++) {
      if (dynamic_cast<Lion*>(*it))
        toDelete.push_back(*it);
    }

    const std::set<Controllable*>& deadLions = _engine.getDeadControllableElements();
    for (auto it = deadLions.begin(); it != deadLions.end(); it++) {
      if (dynamic_cast<Lion*>(*it))
        toDelete.push_back(*it);
    }

    _engine.deleteElements(toDelete);
    _selection.clear();
    _nbLions = 0;
    Lion::resetNbKilled();
    _huntHasStarted = true;
    _huntStart.restart();

    _interface.setTextTopLeft(getInfoTextGlobalView());
    _interface.setTextCenter("Hunt Starts!", _msCenterTextDisplayDuration);
  }
}

void Game::genTribe() {
  Camera& cam = Camera::getInstance();
  genTribe(glm::ivec2(cam.getWindowW(), cam.getWindowH()) / 2);
}

void Game::genTribe(glm::ivec2 screenTarget) {
  deleteTribe();
  _tribe = _engine.genTribe(screenTarget);

  if (_tribe.size() == 0)
    displayInfo("Can't spawn a tribe here");
}

void Game::deleteTribe() {
  std::vector<igMovingElement*> tribe(_tribe.size());

  #pragma omp parallel for
  for (int i = 0; i < _tribe.size(); i++) {
    tribe[i] = (igMovingElement*) _tribe[i];
  }

  _engine.deleteElements(tribe);
  _tribe.clear();
}
