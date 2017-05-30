#include "controllable.h"

#include "camera.h"
#include "vecUtils.h"
#include <iostream>

Controllable::Controllable(sf::Vector2f position, AnimationManager graphics) :
 	igMovingElement(position, graphics),
  _target(position) {

}

void Controllable::update(sf::Time elapsed) {
  if (!_dead) {
    igMovingElement::update(elapsed);
    // The element has gone too far
    if (vu::dot((_target - _pos), getDirection()) < 0) {
      _pos = _target;
      stop();
    }
  }
}

void Controllable::setTarget(sf::Vector2f t) {
	if (!_dead) {
    launchAnimation(WALK);
		_target = t;

		setDirection(t-_pos);
	}
}

void Controllable::stop() {
  if (!_dead) {
    _target = _pos;
    setDirection(sf::Vector2f(0,0));
    launchAnimation(WAIT);
  }
}

sf::IntRect Controllable::getScreenCoord() const {
	Camera& cam = Camera::getInstance();

	sf::IntRect res;

	res.left = (_projectedVertices[3] + 1.f) / 2.f * cam.getW();
	res.top = -(_projectedVertices[1] + 1.f) / 2.f * cam.getH() + cam.getH();
	res.width =  (_projectedVertices[0] - _projectedVertices[3]) / 2.f * cam.getW();
	res.height = (_projectedVertices[1] - _projectedVertices[7]) / 2.f * cam.getH();

	return res;
}
