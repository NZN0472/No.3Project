#include "Button.h"
#include "MouseBridge.h"
#include "../GameLib/game_lib.h"

using namespace GameLib;
using namespace input;

// Button.cpp 内だけで TRG を上書き：左クリックだけ PAD_START 扱い
#undef TRG
#define TRG(player) ( \
    (GameLib::input::TRG(player) & ~GameLib::input::PAD_START) | \
    (MouseBridge::TrgL() ? GameLib::input::PAD_START : 0) \
)

Button::Button(float x, float y, float w, float h)
    : x(x), y(y), w(w), h(h)
{
}

void Button::update() {
    float mx = (float)GameLib::input::getCursorPosX();
    float my = (float)GameLib::input::getCursorPosY();

    hovered = (mx >= x && mx <= x + w &&
        my >= y && my <= y + h);

    // クリックされた瞬間（左クリックのみ）
    clicked = hovered && (TRG(0) & PAD_START);
}

void Button::setRect(float x, float y, float w, float h) {
    this->x = x;
    this->y = y;
    this->w = w;
    this->h = h;
}

void Button::draw(float r, float g, float b, float a) {
    // hovered なら少し濃くする（見た目用）
    float aa = hovered ? (a * 0.9f) : (a * 0.6f);

    primitive::rect(
        x, y, w, h,
        0, 0, 0,
        r, g, b, aa
    );
}
