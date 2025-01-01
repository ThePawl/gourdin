#include "popupMenu.h"

#include "camera.h"
#include "game.h"

#define INDEX_NONE_SELECTED -1
#define MENU_ENTRY_HEIGHT InterfaceParameters::getInstance().sizeTextMedium()
#define MARGINS_SIZE InterfaceParameters::getInstance().marginsSize(MENU_ENTRY_HEIGHT)

#ifdef MOBILE
  #define FINGER_POPUP_OFFSET (300 * InterfaceParameters::getInstance().getMobileInterfaceZoomFactor())
#endif

glm::ivec2 PopupMenu::MenuEntry::_clickPos;

PopupMenu::MenuEntry::MenuEntry(std::string name, TCallbackFunc callback):
  _callback(callback),
  _highlight(InterfaceParameters::getInstance().colorBackground()),
  _highlighted(false) {

  _text.setText(name, MENU_ENTRY_HEIGHT);
}

void PopupMenu::MenuEntry::bindShaderAndDraw() const {
  if (_highlighted)
    _highlight.bindShaderAndDraw();
  _text.bindShaderAndDraw();
}

void PopupMenu::MenuEntry::triggerAndDestroy(Game* game) {
  (game->*_callback)(_clickPos);
}

void PopupMenu::MenuEntry::setAllocatedRect(const glm::ivec4& allocatedRect) {
  _allocatedRect = allocatedRect;
  _highlight.setRectangles(allocatedRect);
  _text.setPosition(glm::ivec2(allocatedRect.x + MARGINS_SIZE, allocatedRect.y + MARGINS_SIZE));
}

PopupMenu::PopupMenu(Game* game):
  _game(game),
  _highlightedIndex(0),
  _isDisplaying(false),
  _frame(InterfaceParameters::getInstance().colorFrame(), false),
  _background(InterfaceParameters::getInstance().colorBackground(), true) {}

void PopupMenu::addEntry(const std::string& name, TCallbackFunc callback) {
  _menuEntries.push_back(PopupMenu::MenuEntry(name, callback));
}

void PopupMenu::bindShadersAndDraw() const {
  if (_isDisplaying) {
    _background.bindShaderAndDraw();

    for (int i = 0; i < _menuEntries.size(); i++) {
      _menuEntries[i].bindShaderAndDraw();
    }

    _frame.bindShaderAndDraw();
  }
}

glm::ivec2 PopupMenu::getMenuSize() {
  glm::ivec2 totalSize(0.f);

  for (int i = 0; i < _menuEntries.size(); i++) {
    glm::ivec2 currentSize = _menuEntries[i].getTextSize();

    totalSize.x = std::max(totalSize.x, currentSize.x);
    totalSize.y += currentSize.y;
  }

  totalSize.x += 2 * MARGINS_SIZE;
  totalSize.y += 2 * _menuEntries.size() * MARGINS_SIZE;

  return totalSize;
}

void PopupMenu::create(glm::ivec2 clickPos) {
  if (!_menuEntries.empty()) {
    MenuEntry::_clickPos = clickPos;

#ifdef GL_ES_VERSION_3_0
    if (clickPos.x > Camera::getInstance().getWindowW() / 2)
      clickPos.x -= FINGER_POPUP_OFFSET;
    else
      clickPos.x += FINGER_POPUP_OFFSET;
#endif

    glm::ivec4 menuRectangle(clickPos - (glm::ivec2(2 * MARGINS_SIZE) + _menuEntries.front().getTextSize()) / 2, getMenuSize());

    _frame.setRectangles(menuRectangle);
    _background.setRectangles(menuRectangle);

    glm::ivec4 currentAllocatedRect = menuRectangle;
    currentAllocatedRect.w = MENU_ENTRY_HEIGHT + 2 * MARGINS_SIZE;

    for (int i = 0; i < _menuEntries.size(); i++) {
      _menuEntries[i].setAllocatedRect(currentAllocatedRect);
      currentAllocatedRect.y += currentAllocatedRect.w;
    }

    _isDisplaying = true;

    updateHighlight(clickPos);
  }
}

void PopupMenu::updateHighlight(const glm::ivec2& mousePos) {
  if (!_menuEntries.empty()) {
    if (mousePos.y < _menuEntries.front().getAllocatedRect().y ||
        mousePos.y > _menuEntries.back().getAllocatedRect().y + _menuEntries.back().getAllocatedRect().w) {
      highlightIndex(INDEX_NONE_SELECTED);
      return;
    }

    for (int i = 0; i < _menuEntries.size(); i++) {
      glm::ivec4 allocatedRect = _menuEntries[i].getAllocatedRect();
      if (allocatedRect.y + allocatedRect.w > mousePos.y) {
        highlightIndex(i);
        return;
      }
    }
  }
}

void PopupMenu::highlightIndex(int index) {
  _highlightedIndex = index;

  for (int i = 0; i < _menuEntries.size(); i++) {
    if ((int) i == index)
      _menuEntries[i].setHighlighted(true);
    else
      _menuEntries[i].setHighlighted(false);
  }
}

void PopupMenu::triggerAndDestroy(const glm::ivec2& clickPos) {
  updateHighlight(clickPos);
  if (_highlightedIndex != INDEX_NONE_SELECTED)
    _menuEntries[_highlightedIndex].triggerAndDestroy(_game);
  _isDisplaying = false;
}
