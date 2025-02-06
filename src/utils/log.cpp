#include "log.h"

#include "camera.h"

#define LOG_REFRESH_RATE_MS 1000

Log::Log() {
  _text.precision(2);
  _text.setf( std::ios::fixed, std:: ios::floatfield );
}

void Log::addFPSandCamInfo() {
  if (_lastFPSupdate.getElapsedTime() > LOG_REFRESH_RATE_MS) {
    _lastFPS.str("");

    size_t fps = 1000 * _framesSinceLastUpdate / _lastFPSupdate.getElapsedTime();

    _lastFPS << "FPS: " << fps << std::endl;
    _framesSinceLastUpdate = 1;
    _lastFPSupdate.restart();
  }

  else
    _framesSinceLastUpdate++;

  Camera& cam = Camera::getInstance();
  _text << "R: " << (int) cam.getZoom() << "\n";

  #ifndef MOBILE
    _text << "Theta: " << cam.getTheta() - 360 * (int) (cam.getTheta() / 360) +
                         (cam.getTheta() < 0 ? 360 : 0) << "\n"
          << "Phi: " << cam.getPhi() << std::endl;
    _text << "X: " << cam.getPointedPos().x << "\n"
          << "Y: " << cam.getPointedPos().y << std::endl;
  #endif

  _text << _lastFPS.str();
}
