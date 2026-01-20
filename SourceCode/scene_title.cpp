#include "all.h"
#include "Button.h"

static Button startBtn(420, 500, 440, 120);
static Button endBtn(0, 0, 220, 60);

static Button btnTut(440, 620, 300, 80);
static bool tutorialOnTitle = false;


int title_state;
int title_timer;


static Sprite* state=nullptr;
static Sprite* title=nullptr;
static Sprite* exitBtn=nullptr;

static Sprite* sprTutOn  = nullptr;
static Sprite* sprTutOff = nullptr;

//チュートリアルボタン
static constexpr float TUT_SRC_W = 300.0f;
static constexpr float TUT_SRC_H = 80.0f;

static void drawBtnImageFitLocal(Sprite* spr, const Button& b, float srcW, float srcH, bool enabled)
{
    if (!spr) return;
    float a = enabled ? 1.0f : 0.35f;
    float sx = b.getW() / srcW;
    float sy = b.getH() / srcH;

    sprite_render(spr,
        b.getX(), b.getY(), sx, sy,
        0, 0, srcW, srcH,
        0, 0, 0,
        1, 1, 1, a,
        false);
}

void title_init() {
    title_state = 2;
    title_timer = 0;
    tutorialOnTitle = true;
    if (!state)   state = sprite_load(L"./Data/Images/stateBtn2.png");
    if (!title)   title = sprite_load(L"./Data/Images/title.png");
    if (!exitBtn) exitBtn = sprite_load(L"./Data/Images/EXITBtn.png");

    // ON/OFF画像
    if (!sprTutOn)  sprTutOn = sprite_load(L"./Data/Images/TutOn.png");
    if (!sprTutOff) sprTutOff = sprite_load(L"./Data/Images/TutOff.png");

    GameLib::setBlendMode(Blender::BS_ALPHA);
    AudioManager::PlayBGM(BGM_TITLE);
}

void title_deinit() {
    safe_delete( state );
    safe_delete( title );
    safe_delete( exitBtn);
    safe_delete(sprTutOn);
    safe_delete(sprTutOff);
}

void title_update() {
    switch (title_state)
    {
    case 0:
        // 初期設定
        GameLib::setBlendMode(Blender::BS_ALPHA);
        
        title_init();
        
        break;

    case 2:
        // ボタン更新（毎フレーム）
        startBtn.update();
        endBtn.update();
        btnTut.update();

        if (btnTut.isClicked()) {
            tutorialOnTitle = !tutorialOnTitle;
        }

        // 左クリックで押されたら遷移
        if (startBtn.isClicked()) {
            gStartTutorial = tutorialOnTitle;  //（ゲーム開始時だけ有効）
            AudioManager::PlaySE(SE_KIRI);//state
            AudioManager::PlayBGM(BGM_GAME);
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
    //チュートリアルボタン

    Sprite* tutSpr = tutorialOnTitle ? sprTutOn : sprTutOff;
    if (tutSpr) {
        drawBtnImageFitLocal(tutSpr, btnTut, TUT_SRC_W, TUT_SRC_H, true);
    }
    else {
        // 画像がまだ無い時の保険：デバッグ表示
        btnTut.draw(0.2f, 0.2f, 0.2f, 1.0f);
        
    }
    
}
