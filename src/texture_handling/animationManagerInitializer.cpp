#include "animationManagerInitializer.h"

#include <sstream>

#include "tinyxml.h"

void AnimationManagerInitializer::load(std::string folderPath) {
  std::string xmlFile = folderPath + "animInfo.xml";

  TiXmlDocument doc(xmlFile);

  if(!doc.LoadFile()) {
    std::cerr << "Error while loading file: " << xmlFile << std::endl;
    std::cerr << "Error #" << doc.ErrorId() << ": " << doc.ErrorDesc() << std::endl;
  }

  TiXmlHandle hdl(&doc);
  TiXmlElement *elem = hdl.FirstChildElement().FirstChildElement().Element();

  if (elem == nullptr)
    std::cerr << "Empty xml file: " << xmlFile << std::endl;

  _maxHeight = 0;

  for (int i = 0; elem; i++) {
    AnimInfo anm;
    elem->QueryIntAttribute("steps", &anm.steps);
    elem->QueryIntAttribute("nbOri", &anm.orientations);

    int durationms, pausems, type;
    elem->QueryIntAttribute("duration", &durationms);
    elem->QueryIntAttribute("pause", &pausems);
    elem->QueryIntAttribute("type", &type);

    anm.duration = sf::milliseconds(durationms);

    if (pausems == -1) {
      anm.loop = false;
      anm.pause = sf::Time::Zero;
    }

    else {
      anm.loop = true;
      anm.pause = sf::milliseconds(pausems);
    }

    anm.texLayer = i;

    _animInfo.insert(std::pair<ANM_TYPE, AnimInfo>((ANM_TYPE) type, anm));
    elem = elem->NextSiblingElement();
  }

  _texArray.loadTextures(_animInfo.size(), folderPath);

  int i = 0;
  for (auto it = _animInfo.begin(); it != _animInfo.end(); it++) {
    sf::FloatRect texRect = _texArray.getTexRectangle(i);
    it->second.sprite = sf::FloatRect(0, 0, 1.f / it->second.steps        * texRect.width,
                                            1.f / it->second.orientations * texRect.height);

    std::ostringstream convert;
    convert << folderPath << i << ".png";
    sf::Vector2f texSize = _texArray.texSizes[i];
    it->second.spriteSize = sf::Vector2f(texSize.x / it->second.steps,
                                         texSize.y / it->second.orientations);

    if (it->second.spriteSize.y > _maxHeight)
      _maxHeight = it->second.spriteSize.y;

    i++;
  }
}
