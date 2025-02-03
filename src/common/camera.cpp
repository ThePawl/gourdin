#include "camera.h"

#include "utils.h"

#define CHUNK_BEGIN_X 4
#define CHUNK_BEGIN_Y 6

Camera::Camera() :
  _fovAngle(45),
	_aspectRatio(1.f),
	_nearPlane(1.f),
	_farPlane(100000.f),
  _additionalHeight(0.f),
	_x(0.f), _y(0.f), _height(0.f),
	_r(INIT_R),
	_phi(INIT_PHI),
  _theta(INIT_THETA),
  _projection(1.f),
  _view(1.f) {}

void Camera::reset() {
  setPointedPos(glm::vec2(CHUNK_BEGIN_X * CHUNK_SIZE + CHUNK_SIZE / 2,
                          CHUNK_BEGIN_Y * CHUNK_SIZE + CHUNK_SIZE / 2));

  setValues(INIT_R, INIT_THETA, INIT_PHI);
}

void Camera::resizeGameViewport(unsigned int W, unsigned int H) {
	_gameW = W;
	_gameH = H;

	glViewport (0, 0, (GLint)W, (GLint)H);
  _aspectRatio = static_cast<float>(W)/static_cast<float>(H);
  float fovAngleRadians = 45 * M_PI / 180.f;
  _projection = glm::perspective(fovAngleRadians, _aspectRatio, _nearPlane, _farPlane);

  _viewProjection = _projection * _view;
  _skyboxViewProjection = _projection * _skyboxView;
}

void Camera::apply() {
  _pos = glm::vec3(_x,_y,_height + _additionalHeight) + ut::carthesian(_r, _theta, _phi);

  _view = glm::lookAt (
	  _pos,
    glm::vec3(_x, _y, _height + _additionalHeight),
    ut::carthesian(1.f, _theta, _phi + _fovAngle/2.f - 90.f)
  );

  _viewProjection = _projection * _view;

  glm::vec3 camDirection = ut::carthesian(1.f, _theta, _phi);

  _skyboxView = glm::lookAt (
	  camDirection,
    glm::vec3(0, 0, 0),
    ut::carthesian(1.f, _theta, _phi + _fovAngle/2.f - 90.f)
  );

  _skyboxViewProjection = _projection * _skyboxView;
}

void Camera::translate(float dWinX, float dWinY) {
  _x += dWinX*cos((_theta+90.f)*RAD)+dWinY*sin((_theta+90.f)*RAD);
  _y +=	dWinX*sin((_theta+90.f)*RAD)-dWinY*cos((_theta+90.f)*RAD);

  if (_x >= MAX_COORD)
    _x = MAX_COORD-1;
  if (_y >= MAX_COORD)
    _y = MAX_COORD-1;
  if (_x < 0)
    _x = 0;
  if (_y < 0)
    _y = 0;
}

void Camera::rotate (float dtheta, float dphi) {
  _theta += dtheta;
  _phi += dphi;

  if (_phi < 0)
    _phi = 0;
  else if (_phi > 180.f)
    _phi = 180.f;
}
