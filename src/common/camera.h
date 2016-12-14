#pragma once

#include <SFML/OpenGL.hpp>
#include <SFML/System.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "utils.h"

#include <cmath>

#define RAD M_PI / 180.f

#define INIT_R 150.f

class Controller;
class Game;

class Camera {
public:
	static Camera& getInstance() {
    static Camera instance;
    return instance;
  }

	Camera(Camera const&)          = delete;
	void operator=(Camera const&)  = delete;

	inline unsigned int getW() const {return _W;}
	inline unsigned int getH() const {return _H;}
  inline sf::Vector3f getPos() const {return _pos;}
  inline float getTheta() const {return _theta;}
  inline float getPhi() const {return _phi;}
  inline float getFov() const {return _fovAngle;}
  inline float getRatio() const {return _aspectRatio;}
  inline sf::Vector2f getPointedPos() const {return sf::Vector2f(_x, _y);}
  inline glm::mat4 getViewProjectionMatrix() const {return _viewProjection;}
	inline float getZoomFactor() const {return _r / INIT_R;}

	friend Controller;
	friend Game;

private:
	Camera();

	void resize(unsigned int W, unsigned int H);
	void apply ();

	void translate (float dWinX, float dWinY);
	inline void rotate (float dtheta, float dphi) {_theta += dtheta; _phi += dphi;}
	inline void zoom (float dr) {_r += dr;}

  inline void setPointedPos(sf::Vector2f newPos) {_x = newPos.x; _y = newPos.y;}
  inline void setHeight(float nHeight) {_height = nHeight;}

	unsigned int _W, _H;

  float _fovAngle;
  float _aspectRatio;
  float _nearPlane;
  float _farPlane;

  float _x, _y, _height;
  float _r, _phi, _theta; // The camera points towards the center of the sphere
  sf::Vector3f _pos;

  glm::mat4 _projection;
  glm::mat4 _view;
  glm::mat4 _viewProjection;
};
