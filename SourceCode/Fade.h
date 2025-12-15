#pragma once
#include "../GameLib/game_lib.h"
#include "common.h"
#include<vector>
#include<random>


// フェードの状態
enum class FadeState {
    None,
    FadeOut,
    Wait,
    FadeIn
};

// 汎用フェードクラス（どのマネージャーからでも使用可能）
class Fade {
public:
    Fade();
    void StartFadeOut(float duration = 1.0f);  // フェードアウト開始
    void StartFadeIn(float duration = 1.0f);   // フェードイン開始
    void Update(float elapsedTime);            // 経過時間を進める
    void Draw();                               // フェード矩形描画

    // 状態チェック
    bool IsFading() const { return fadeState != FadeState::None; }
    bool IsFadeOutFinished() const { return fadeState == FadeState::Wait; }
    bool IsFadeInFinished() const { return fadeState == FadeState::None && fadeTimer >= fadeDuration; }

private:
    FadeState fadeState = FadeState::None;
    float fadeTimer;
    float fadeDuration;

    std::vector<int> dealOrder;
    std::mt19937 rng{ std::random_device{}() };

    void PrepareDealOrder(int total);
};