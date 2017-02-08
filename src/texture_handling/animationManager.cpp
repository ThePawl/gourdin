#include "animationManager.h"

#include <cmath>
#include <iostream>

AnimationManager::AnimationManager(const AnimationManagerInitializer& init) :
	_currentAnim(WAIT),
  _currentSprite(0),
  _dead(false),
  _alreadyElapsed(sf::Time::Zero),
	_texManager(init) {

	_animInfo = _texManager.getAnimInfo();
}

size_t AnimationManager::launchAnimation(ANM_TYPE type) {
  if (!_dead) {
    if (_currentAnim != type) {
      _currentAnim = type;

      _currentSprite = 0;
    }
  }

	return _animInfo[type].texLayer;
}

void AnimationManager::update(sf::Time elapsed, float nOrientation) {
  _alreadyElapsed += elapsed;

  if (_currentSprite == _animInfo[_currentAnim].steps - 1 && _animInfo[_currentAnim].pause != sf::Time::Zero) {
    if (_animInfo[_currentAnim].loop) {
      if (_alreadyElapsed.asMilliseconds() / _animInfo[_currentAnim].pause.asMilliseconds() != 0) {
        _currentSprite = 0;
        _alreadyElapsed = sf::milliseconds(_alreadyElapsed.asMilliseconds() - _animInfo[_currentAnim].pause.asMilliseconds());
      }
    }
  }

  else {
    if (_alreadyElapsed.asMilliseconds() / _animInfo[_currentAnim].duration.asMilliseconds() != 0) {
      _currentSprite++;
      _currentSprite %= _animInfo[_currentAnim].steps;
      _alreadyElapsed = sf::milliseconds(_alreadyElapsed.asMilliseconds() - _animInfo[_currentAnim].duration.asMilliseconds());
    }
  }

  _currentOrient = getClosestOrient(nOrientation);
}

sf::FloatRect AnimationManager::getCurrentSprite() const {
	sf::FloatRect sprite = _animInfo.at(_currentAnim).sprite;

  sprite.left += _currentSprite * sprite.width;
  sprite.top += _currentOrient * sprite.height;

  return sprite;
}

sf::Time AnimationManager::getAnimationTime(ANM_TYPE type) {
  return sf::milliseconds((_animInfo[type].steps-1) * _animInfo[type].duration.asMilliseconds());
}

int AnimationManager::getClosestOrient(float orientation) {

  float oriStep = 360.f / (float) _animInfo[_currentAnim].orientations;

  return (_animInfo[_currentAnim].orientations - (int) std::round(orientation / oriStep)) % 8;
}
