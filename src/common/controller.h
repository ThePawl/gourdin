#pragma once

#include <GL/glew.h>
#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>

#include "event_handler_game.h"
#include "event_handler_sandbox.h"
#include "engine.h"
#include "gameGame.h"
#include "gameSandbox.h"

#ifdef __APPLE__
#define CORE_PROFILE
#endif

#ifndef NDEBUG
	class TestHandler;
#endif

/* Controls the main loop of the program
 */
class Controller {
public:
	Controller(sf::RenderWindow& window);

	void init();
	void run();

#ifndef NDEBUG
	friend TestHandler;
#endif

private:
	void benchmark();

	bool _running;
	int _msElapsed;

	Engine _engine;
	GameGame _gameGame;
	GameSandbox _gameSandbox;

	EventHandlerGame    _eHandlerGame;
	EventHandlerSandbox _eHandlerSandbox;
	EventHandlerType    _currentHandlerType;

	sf::RenderWindow& _window;
};
