#include "all.h"
#include "Button.h"

static Button startBtn(420, 500, 440, 120);
static Button endBtn(0, 0, 220, 60);



int title_state;
int title_timer;

Sprite* sprCar;
Sprite* state;
Sprite* title;
Sprite* exitBtn;
void title_init() {
    title_state = 0;
    title_timer = 0;
    sprCar = nullptr;
}

void title_deinit() {
    safe_delete(sprCar);
}

void title_update() {
    switch (title_state)
    {
    case 0:
        // 初期設定
        state = sprite_load(L"./Data/Images/stateBtn2.png"); // スタートボタン
        title = sprite_load(L"./Data/Images/title.png");     // タイトル
        exitBtn = sprite_load(L"./Data/Images/EXITBtn.png"); // 終了ボタン
        GameLib::setBlendMode(Blender::BS_ALPHA);
        title_state = 2; // いきなり通常へ
        break;

    case 2:
        // ボタン更新（毎フレーム）
        startBtn.update();
        endBtn.update();

        // 左クリックで押されたら遷移
        if (startBtn.isClicked()) {
            nextScene = SCENE_GAME;
        }
       
        if (endBtn.isClicked()) {
            gQuit = true;
        }
        break;
    }

    title_timer++;
}

void title_render() {
    GameLib::clear(1, 0, 0);

    // タイトル画面
    sprite_render(title, 0, 0);

    //終了ボタン
    sprite_render(exitBtn, 0, 0);
    //スタートボタン
    sprite_render(state, 420, 500);
    if (sprCar) sprite_render(sprCar, 200, 200);

    // ボタン表示（画像が無い間の確認用）
    //startBtn.draw(0.10f, 0.35f, 0.95f, 1.0f);
    //endBtn.draw(0.0f, 1.0f, 1.0f, 1.0f);
}
