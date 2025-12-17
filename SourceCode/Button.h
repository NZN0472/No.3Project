#pragma once
#include "../GameLib/game_lib.h"
#include "common.h"

class Button {
public:
    Button(float x, float y, float w, float h);

    void update();                 // 毎フレーム更新
    bool isClicked() const { return clicked; }   // 押した瞬間
    bool isHovered() const { return hovered; }   // 乗ってる

    void draw(float r = 0.2f, float g = 0.6f, float b = 1.0f, float a = 0.7f);
    void setRect(float x, float y, float w, float h);

private:
    float x, y, w, h;
    bool hovered = false;
    bool clicked = false;
};
