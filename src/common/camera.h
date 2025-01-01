#pragma once

#include "opengl.h"
#include "utils.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>

#define ELEMENT_NEAR_PLANE 20.f

// r, theta, phi follows the mathematical convention: theta is the azimuthal and phi the polar angle

#ifdef PHONE
	#define MIN_R 80.f
	#define INIT_R 100.f
#else
	#define MIN_R 120.f
	#define INIT_R 150.f
#endif

#define MAX_R 12500.f
#define INIT_PHI 60.f
#define INIT_THETA 180.f

class Controller;
class EventHandler;
class EventHandlerLockedView;
class EventHandlerGlobalView;
class Game;

class Camera {
public:
	static Camera& getInstance() {
    static Camera instance;
    return instance;
  }

	Camera(Camera const&)          = delete;
	void operator=(Camera const&)  = delete;

	void reset();

	inline glm::ivec4 getScreenRect() const {return glm::ivec4(0,0,_windowW,_windowH);}

	inline unsigned int getW() const {return _W;}
	inline unsigned int getH() const {return _H;}
	inline unsigned int getWindowW() const {return _windowW;}
	inline unsigned int getWindowH() const {return _windowH;}
  inline glm::vec3 getPos() const {return _pos;}
	inline glm::vec2 getProjectedPos() const {return glm::vec2(_pos.x, _pos.y);}
  inline float getTheta() const {return _theta;}
  inline float getPhi() const {return _phi;}
  inline float getFov() const {return _fovAngle;}
  inline float getRatio() const {return _aspectRatio;}
  inline glm::vec2 getPointedPos() const {return glm::vec2(_x, _y);}
	inline float getHeight() const {return _height;}
	inline float getAdditionalHeight() const {return _additionalHeight;}
  inline glm::mat4 getViewProjectionMatrix() const {return _viewProjection;}
	inline glm::mat4 getSkyboxViewProjectionMatrix() const {return _skyboxViewProjection;}
	inline float getZoom() const {return _r;}

	friend Controller;
	friend EventHandler;
	friend EventHandlerLockedView;
	friend EventHandlerGlobalView;
	friend Game;

private:
	Camera();

	void resize(unsigned int W, unsigned int H);
	inline void setWindowSize(unsigned int W, unsigned int H) {_windowW = W; _windowH = H;}
	void apply();

	void translate (float dWinX, float dWinY);
	void rotate (float dtheta, float dphi);
	inline void zoom (float dr) {_r += dr; _r = _r < MIN_R ? MIN_R : _r; _r = _r > MAX_R ? MAX_R : _r;}
	inline void setTheta(float theta) {_theta = ut::clampAngle(theta);}
	inline void setPhi  (float phi) {_phi = phi;}
	inline void setZoom (float r) {_r = r;}
	inline void setValues (float r, float theta, float phi) {setZoom(r); setTheta(theta); setPhi(phi);}

  inline void setPointedPos(glm::vec2 newPos) {_x = newPos.x; _y = newPos.y;}
  inline void setHeight(float nHeight) {_height = nHeight;}
	inline void setAdditionalHeight(float nAddHeight) {_additionalHeight = nAddHeight;}

	unsigned int _W, _H;
	unsigned int _windowW, _windowH;

  float _fovAngle;
  float _aspectRatio;
  float _nearPlane;
  float _farPlane;

	float _additionalHeight;
  float _x, _y, _height;
  float _r, _phi, _theta; // The camera points towards the center of the sphere
  glm::vec3 _pos;

  glm::mat4 _projection;
  glm::mat4 _view;
  glm::mat4 _viewProjection;
	glm::mat4 _skyboxView;
	glm::mat4 _skyboxViewProjection;
};
