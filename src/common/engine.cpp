#include "engine.h"

#include "opengl.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include "camera.h"
#include "log.h"
#include "reliefGenerator.h"

#include <ctime>

Engine::Engine() :
  _wireframe(false),
  _contentGenerator(_terrainGeometry),
  _ocean(2),
  _mapInfoExtractor(_terrainGeometry),
  _reliefGenerator(_mapInfoExtractor),
  _terrain(NB_CHUNKS*NB_CHUNKS) {}

Engine::~Engine() {
  _chunkSubdivider.join();
}

void Engine::init(LoadingScreen& loadingScreen) {
  srand(time(NULL));

  loadingScreen.updateAndRender("Loading shaders", 0);

  _terrainShader.load("src/shaders/heightmap.vert", "src/shaders/heightmap.frag");
  _igEShader.load("src/shaders/igElement.vert", "src/shaders/igElement.frag");
  _skyboxShader.load("src/shaders/skybox.vert", "src/shaders/skybox.frag");
  _depthInColorBufferShader.load("src/shaders/2D_shaders/2D.vert", "src/shaders/2D_shaders/depthToColor.frag");

  _igEShader.bind();
  glUniform1f(_igEShader.getUniformLocation("elementNearPlane"), ELEMENT_NEAR_PLANE);
  Shader::unbind();

  loadingScreen.updateAndRender("Loading terrain data", 1);

  _terrainTexManager.loadFolder((size_t) Biome::BIOME_NB_ITEMS, "res/terrain/");
  _map.load("res/map/");
  _map.feedGeometryData(_terrainGeometry);

  GeneratedImage relief;

  if (relief.loadFromFile("res/map/relief.png")) {
    loadingScreen.updateAndRender("Loading relief", 25);
    _terrainGeometry.setReliefGenerator(relief);
  }

  else {
    loadingScreen.updateAndRender("Generating relief", 25);

    _mapInfoExtractor.convertMapData(512);
    _mapInfoExtractor.generateBiomesTransitions(7);

    _reliefGenerator.generateRelief();
    _reliefGenerator.saveToFile("res/map/relief.png");
    _terrainGeometry.setReliefGenerator(_reliefGenerator.getRelief());
  }

  loadingScreen.updateAndRender("Generate terrain geometry", 30);

  // The base subdivision level is 1, it will take into account the generated relief contrary to level 0
  _terrainGeometry.generateNewSubdivisionLevel();

  loadingScreen.updateAndRender("Launching chunks subdivisions", 33);

  std::vector<Chunk*> newChunks;
  for (int i = 0; i < NB_CHUNKS; i++) {
    for (int j = 0; j < NB_CHUNKS; j++) {
      newChunks.push_back(new Chunk(i, j, _terrainTexManager, _terrainGeometry, _chunkSubdivider));
    }
  }

  for (int i = 0; i < NB_CHUNKS; i++) {
    for (int j = 0; j < NB_CHUNKS; j++) {
      _chunkSubdivider.addTask(newChunks[i*NB_CHUNKS + j], 1);
      _terrain[i*NB_CHUNKS + j] = std::unique_ptr<Chunk>(newChunks[i*NB_CHUNKS + j]);
    }
  }

  loadingScreen.updateAndRender("Loading ocean and skybox", 34);

  _ocean.setTexture(_terrainTexManager.getTexture((size_t) Biome::OCEAN));
  _skybox.load("res/skybox/");

  Camera& cam = Camera::getInstance();
  _globalFBO.init(cam.getGameW(), cam.getGameH(), GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE);
  _depthInColorBufferFBO.init(cam.getGameW(), cam.getGameH(), GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE);
  _depthTexturedRectangle.reset(new TexturedRectangle(_globalFBO.getDepthTexture(), cam.getWindowRect()));

  loadingScreen.updateAndRender("Initializing content generator", 38);

  _contentGenerator.init();

  loadingScreen.updateAndRender("Generating herds", 60);

  appendNewElements(_contentGenerator.genHerd(cam.getPointedPos(), 20, Animals::DEER));
  appendNewElements(_contentGenerator.genHerds());

  _chunkSubdivider.waitForTasksToFinish();

  loadingScreen.updateAndRender("Generating forests", 60);

  //#pragma omp parallel for
  for (int i = 0; i < NB_CHUNKS*NB_CHUNKS; i++) {
    size_t x = i / NB_CHUNKS;
    size_t y = i - x * NB_CHUNKS;
    std::vector<igElement*> newTrees = _contentGenerator.genForestsInChunk(x,y);
    newChunks[x*NB_CHUNKS + y]->setTrees(newTrees);
  }
}

void Engine::appendNewElements(std::vector<igMovingElement*> elems) {

  for (int i = 0; i < elems.size(); i++)
    _igMovingElements.push_back(std::unique_ptr<igMovingElement>(elems[i]));

  for (int i = 0; i < elems.size(); i++) {
    Controllable* ctrl = dynamic_cast<Controllable*>(elems[i]);
    if (ctrl)
      _controllableElements.insert(ctrl);
  }
}

void Engine::updateMovingElementsStates() {
  struct squareHashFunc {
    size_t operator()(const glm::ivec2 &k) const {
    size_t h1 = std::hash<int>()(k.x);
    size_t h2 = std::hash<int>()(k.y);
    return (h1 << 1) + h1 + h2;
    }
  };

  float squareSize = Antilope::getStandardLineOfSight();
  int nbSquares = MAX_COORD / squareSize;

  std::unordered_map<glm::ivec2, std::list<igMovingElement*>, squareHashFunc> sortedElements;

  for (auto it = _igMovingElements.begin(); it != _igMovingElements.end(); it++) {
    if (!(*it)->isDead()) {
      glm::uvec2 chunkPos = ut::convertToChunkCoords((*it)->getPos());
      if (_terrain[chunkPos.x * NB_CHUNKS + chunkPos.y]->getDisplayMovingElements()) {
        glm::ivec2 square = glm::ivec2((*it)->getPos().x / squareSize, (*it)->getPos().y / squareSize);

        sortedElements[square].push_back(it->get());
      }
    }
  }

  for (auto square = sortedElements.begin(); square != sortedElements.end(); square++) {
    std::list<igMovingElement*> elmtsInSurroundingSquares;

    for (int k = std::max(0, square->first.x-1); k <= std::min(nbSquares-1, square->first.x+1); k++) {
    for (int l = std::max(0, square->first.y-1); l <= std::min(nbSquares-1, square->first.y+1); l++) {
      glm::ivec2 squareCoord(k,l);

      if (sortedElements.find(squareCoord) != sortedElements.end())
        elmtsInSurroundingSquares.insert(elmtsInSurroundingSquares.end(),
          sortedElements.at(squareCoord).begin(), sortedElements.at(squareCoord).end());
    }
    }

    // #pragma omp parallel
    // #pragma omp single
    {
      for (auto elem = square->second.begin(); elem != square->second.end(); elem++)
        // #pragma omp task firstprivate(elem)
        (*elem)->updateState(elmtsInSurroundingSquares);
      // #pragma omp taskwait
    }
  }
}

void Engine::compute2DCorners() {
  Camera& cam = Camera::getInstance();
  glm::mat4 rotateElements = glm::rotate(glm::mat4(1.f),
                                         (float) M_PI / 180.f * cam.getTheta(),
                                         glm::vec3(0, 0, 1));

  rotateElements =           glm::rotate(rotateElements,
                                         ((float) M_PI / 180.f * cam.getPhi() - 90.f) / 2.f,
                                         glm::vec3(0, 1, 0));

  for (auto it = _controllableElements.begin(); it != _controllableElements.end(); it++) {
    glm::vec3 pos;
    pos.x = (*it)->getPos().x;
    pos.y = (*it)->getPos().y;
    pos.z = (*it)->getHeight();

    std::array<float,12> vertices;

    if (!(*it)->isDead() && glm::length(pos-cam.getPos()) > ELEMENT_NEAR_PLANE) {
      // Calculate new corners
      glm::vec3 corners3[4];
      float width = (*it)->getSize().x;
      float height = (*it)->getSize().y;

      corners3[0] = glm::vec3( 0,  width/2, height);
      corners3[1] = glm::vec3( 0, -width/2, height);
      corners3[2] = glm::vec3( 0, -width/2, 0);
      corners3[3] = glm::vec3( 0,  width/2, 0);

      glm::vec3 translatePos((*it)->getPos().x,
                             (*it)->getPos().y,
                             (*it)->getHeight());

      glm::mat4 model = glm::translate(glm::mat4(1.f), translatePos) * rotateElements;

      // Compute their projections
      for (int i = 0; i < 4; i++) {
        glm::vec4 tmp(corners3[i], 1.f);
        tmp = model * tmp;
        tmp = cam.getViewProjectionMatrix() * tmp;
        corners3[i] = glm::vec3(tmp) / tmp.w;
      }

      for (int i = 0; i < 4; i++) {
        vertices[3*i]     = corners3[i].x;
        vertices[3*i + 1] = corners3[i].y;
        vertices[3*i + 2] = corners3[i].z;
      }
    }

    else {
      for (int i = 0; i < 12; i++) {
        vertices[i] = 0;
      }
    }

    (*it)->setProjectedVertices(vertices);
  }
}

void Engine::updateCulling() {
  Camera& cam = Camera::getInstance();

  // Compute frustum planes

  float theta = cam.getTheta();
  float phi   = cam.getPhi();
  float alpha = cam.getFov() * cam.getRatio() / 2.f;

  std::vector<glm::vec3> camFrustumPlaneNormals;
  // Bottom of the view
  camFrustumPlaneNormals.push_back(ut::carthesian(1.f, theta, phi + 90.f - cam.getFov() / 2.f));
  // Top
  camFrustumPlaneNormals.push_back(ut::carthesian(1.f, theta, phi + 90.f + cam.getFov() / 2.f) * -1.f);
  // Right
  camFrustumPlaneNormals.push_back(glm::rotate(
    ut::carthesian(1.f, theta + 90.f, 90.f),
    (float) (- alpha*RAD), ut::carthesian(1.f, theta + 180.f, 90.f - phi)));
  // Left
  camFrustumPlaneNormals.push_back(glm::rotate(
    ut::carthesian(1.f, theta - 90.f, 90.f),
    (float) (alpha*RAD), ut::carthesian(1.f, theta + 180.f, 90.f - phi)));

  // Update terrains
  for (int i = 0; i < NB_CHUNKS; i++) {
    for (int j = 0; j < NB_CHUNKS; j++) {
      _terrain[i*NB_CHUNKS + j]->computeCulling(camFrustumPlaneNormals);

      if (_terrain[i*NB_CHUNKS + j]->isVisible())
        _terrain[i*NB_CHUNKS + j]->computeDistanceOptimizations();
    }
  }
}

void Engine::update(int msElapsed) {
  updateCulling();

  // Update positions of igMovingElement regardless of them being visible
  for (auto it = _igMovingElements.begin(); it != _igMovingElements.end(); it++) {
    (*it)->update(msElapsed);
  }

  updateMovingElementsStates();

  // Fill the visible elements
  std::vector<igElement*> visibleElmts;

  // Update graphics of visible elements
  Camera& cam = Camera::getInstance();

  for (auto it = _igMovingElements.begin(); it != _igMovingElements.end(); it++) {
    glm::uvec2 chunkPos = ut::convertToChunkCoords((*it)->getPos());

    if (_terrain[chunkPos.x*NB_CHUNKS + chunkPos.y]->isVisible() &&
        _terrain[chunkPos.x*NB_CHUNKS + chunkPos.y]->getDisplayMovingElements()) {

      float height = _terrain[chunkPos.x*NB_CHUNKS + chunkPos.y]->getHeight((*it)->getPos());

      (*it)->setHeight(height);
      (*it)->updateDisplay(msElapsed, cam.getTheta());

      visibleElmts.push_back(it->get());
    }
  }

  _igElementDisplay.loadElements(visibleElmts);

  // Remove the dead elements from the controllable elements
  std::vector<Controllable*> toDelete;
  for (auto it = _controllableElements.begin(); it != _controllableElements.end(); it++) {
   if ((*it)->isDead())
     toDelete.push_back(*it);
  }
  for (int i = 0; i < toDelete.size(); i++) {
    _controllableElements.erase(toDelete[i]);
    _deadControllableElements.insert(toDelete[i]);
  }

  compute2DCorners();

  Log& logText = Log::getInstance();
  std::ostringstream renderStats;
  renderStats << "Moving elements: " << visibleElmts.size() << std::endl;
  logText.addLine(renderStats.str());
}

void Engine::renderToFBO() const {
  size_t nbTriangles = 0;
  size_t nbElements = 0;

  _globalFBO.bind();

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  Camera& cam = Camera::getInstance();
  glm::mat4 MVP = cam.getSkyboxViewProjectionMatrix();

  // Skybox

  _skyboxShader.bind();
	glUniformMatrix4fv(_skyboxShader.getUniformLocation("MVP"),
    1, GL_FALSE, &MVP[0][0]);

  glDisable(GL_DEPTH_TEST);
  _skybox.draw();
  nbTriangles += 12;
  glEnable(GL_DEPTH_TEST);

  Shader::unbind();

  // Terrain draws

  MVP = cam.getViewProjectionMatrix();

  _terrainShader.bind();
	glUniformMatrix4fv(_terrainShader.getUniformLocation("MVP"),
    1, GL_FALSE, &MVP[0][0]);

  // Background Ocean

  glDisable(GL_DEPTH_TEST);
  _ocean.draw();
  nbTriangles += 2;
  glEnable(GL_DEPTH_TEST);

  // Chunk

#ifndef GL_ES_VERSION_3_0
  if (_wireframe)
    glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
#endif

  for (int i = 0; i < NB_CHUNKS; i++) {
    for (int j = 0; j < NB_CHUNKS; j++) {
      if (_terrain[i*NB_CHUNKS + j]->isVisible())
        nbTriangles += _terrain[i*NB_CHUNKS + j]->draw();
    }
  }

#ifndef GL_ES_VERSION_3_0
  if (_wireframe)
    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
#endif

  Shader::unbind();

  // igElements

  glm::mat4 rotateElements = glm::rotate(glm::mat4(1.f),
                                         (float) M_PI / 180.f * cam.getTheta(),
                                         glm::vec3(0, 0, 1));

  rotateElements =           glm::rotate(rotateElements,
                                         ((float) M_PI / 180.f * cam.getPhi() - 90.f) / 2.f,
                                         glm::vec3(0, 1, 0));

  _igEShader.bind();
  glUniformMatrix4fv(_igEShader.getUniformLocation("VP"),
    1, GL_FALSE, &MVP[0][0]);
  glUniformMatrix4fv(_igEShader.getUniformLocation("MODEL"),
    1, GL_FALSE, &rotateElements[0][0]);
  glUniform3fv(_igEShader.getUniformLocation("camPos"),
    1, &cam.getPos()[0]);

  // Two passes to avoid artifacts due to alpha blending

  glUniform1i(_igEShader.getUniformLocation("onlyOpaqueParts"), true);
  _igElementDisplay.drawElements();

  for (int i = 0; i < NB_CHUNKS; i++) {
    for (int j = 0; j < NB_CHUNKS; j++) {
      if (_terrain[i*NB_CHUNKS + j]->isVisible() &&
          _terrain[i*NB_CHUNKS + j]->getTreesNeedTwoPasses())
        _terrain[i*NB_CHUNKS + j]->drawTrees();
    }
  }

  glUniform1i(_igEShader.getUniformLocation("onlyOpaqueParts"), false);

  glEnable (GL_BLEND);
  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  _igElementDisplay.drawElements();

  for (int i = 0; i < NB_CHUNKS; i++) {
    for (int j = 0; j < NB_CHUNKS; j++) {
      if (_terrain[i*NB_CHUNKS + j]->isVisible())
        nbElements += _terrain[i*NB_CHUNKS + j]->drawTrees();
    }
  }

  glDisable(GL_BLEND);

  Shader::unbind();
  FrameBufferObject::unbind();

  Log& logText = Log::getInstance();

  std::ostringstream renderStats;
  renderStats << "Triangles: " << nbTriangles << std::endl
              << "Trees:  " << nbElements << std::endl
              << "Chunks waiting for subdivision: " << _chunkSubdivider.getNbTasksInQueue() << std::endl;

  logText.addLine(renderStats.str());
}

void Engine::addLion(glm::ivec2 screenTarget, float minDistToAntilopes) {
  glm::vec2 lionPos = get2DCoord(screenTarget);
  for (auto it = _igMovingElements.begin(); it != _igMovingElements.end(); it++) {
    Antilope* atlp = dynamic_cast<Antilope*>(it->get());
    if (atlp && !atlp->isDead() && glm::length(atlp->getPos() - lionPos) < minDistToAntilopes)
      throw std::runtime_error("Can't spawn a predator here, too close to its preys");
  }
  std::vector<igMovingElement*> newLion = _contentGenerator.genLion(lionPos);

  if (newLion.size() == 0)
    throw std::runtime_error("Can't spawn a predator on this biome");

  appendNewElements(newLion);
}

std::vector<Controllable*> Engine::genTribe(glm::ivec2 screenTarget) {
  glm::vec2 pos = get2DCoord(screenTarget);
  std::vector<igMovingElement*> tribe = _contentGenerator.genTribe(pos);
  appendNewElements(tribe);

  std::vector<Controllable*> res;
  for (int i = 0; i < tribe.size(); i++) {
    res.push_back(dynamic_cast<Controllable*>(tribe[i]));
  }

  return res;
}

void Engine::deleteElements(const std::vector<igMovingElement*>& elementsToDelete) {
  for (auto elmt = _igMovingElements.begin(); elmt != _igMovingElements.end(); ) {
    bool hasDeleted = false;
    for (auto toDel = elementsToDelete.begin(); toDel != elementsToDelete.end(); toDel++) {
      if ((*toDel) == elmt->get()) {
        elmt = _igMovingElements.erase(elmt);
        hasDeleted = true;
      }
    }

    if (!hasDeleted)
      elmt++;
  }

  for (auto elmt = _controllableElements.begin(); elmt != _controllableElements.end(); ) {
    bool hasDeleted = false;
    for (auto toDel = elementsToDelete.begin(); toDel != elementsToDelete.end(); toDel++) {
      if (*toDel == *elmt) {
        elmt = _controllableElements.erase(elmt);
        hasDeleted = true;
      }
    }

    if (!hasDeleted)
      elmt++;
  }

  for (auto elmt = _deadControllableElements.begin(); elmt != _deadControllableElements.end(); ) {
    bool hasDeleted = false;
    for (auto toDel = elementsToDelete.begin(); toDel != elementsToDelete.end(); toDel++) {
      if (*toDel == *elmt) {
        elmt = _deadControllableElements.erase(elmt);
        hasDeleted = true;
      }
    }

    if (!hasDeleted)
      elmt++;
  }
}

glm::vec2 Engine::get2DCoord(glm::ivec2 screenTarget) {
  Camera& cam = Camera::getInstance();

  screenTarget = glm::ivec2(screenTarget.x * cam.getGameW() / cam.getWindowW(),
                            screenTarget.y * cam.getGameH() / cam.getWindowH());

  screenTarget.y = cam.getGameH() - screenTarget.y; // Inverted coordinates

  _depthInColorBufferFBO.bind();
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  _depthInColorBufferShader.bind();
  _depthTexturedRectangle->draw();
  Shader::unbind();

  GLubyte depthBytes[4];
  glReadPixels(screenTarget.x, screenTarget.y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &depthBytes[0]);

  FrameBufferObject::unbind();

  const glm::vec4 bit_shift = glm::vec4(1.0/(256.0*256.0*256.0), 1.0/(256.0*256.0), 1.0/256.0, 1.0);
  float depth = glm::dot(glm::vec4(depthBytes[0], depthBytes[1], depthBytes[2], depthBytes[3]) / 255.f, bit_shift);

  glm::vec3 modelCoord = glm::unProject(glm::vec3(screenTarget.x, screenTarget.y,depth),
    glm::mat4(1.f), cam.getViewProjectionMatrix(),
    glm::vec4(0,0, cam.getGameW(), cam.getGameH()));

  return glm::vec2( modelCoord.x, modelCoord.y);
}

glm::vec3 Engine::getNormalOnCameraPointedPos() const {
  Camera& cam = Camera::getInstance();
  glm::uvec2 chunkPos = ut::convertToChunkCoords(cam.getPointedPos());

  return _terrain[chunkPos.x*NB_CHUNKS + chunkPos.y]->getNorm(cam.getPointedPos());
}

float Engine::getHeight(glm::vec2 pos) const {
  if (pos.x < 0 || pos.y < 0 || pos.x > MAX_COORD || pos.y > MAX_COORD)
    return 0.f;

  glm::uvec2 chunkPos = ut::convertToChunkCoords(pos);

  return _terrain[chunkPos.x * NB_CHUNKS + chunkPos.y]->getHeight(pos);
}
