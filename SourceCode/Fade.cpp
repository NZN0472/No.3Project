#include "Fade.h"
#include "common.h"
#include "all.h"
#include <numeric>  
#include <algorithm>

Fade::Fade()
    : fadeState(FadeState::None),
    fadeTimer(0.0f),
    fadeDuration(1.0f) {
}

void Fade::StartFadeOut(float dur) {
    fadeState = FadeState::FadeOut;
    fadeDuration = dur;
    fadeTimer = 0.0f;
}

void Fade::StartFadeIn(float dur) {
    fadeState = FadeState::FadeIn;
    fadeDuration = dur;
    fadeTimer = 0.0f;
}

void Fade::Update(float elapsedTime) {
    if (fadeState == FadeState::None) return;

    fadeTimer += elapsedTime;
    if (fadeTimer >= fadeDuration) {
        fadeTimer = fadeDuration;

        if (fadeState == FadeState::FadeOut) {
            fadeState = FadeState::Wait; // フェードアウト完了 → 次のシーン切り替え
        }
        else if (fadeState == FadeState::FadeIn) {
            fadeState = FadeState::None; // フェードイン完了
        }
    }
}

static float clamp01(float x) {
    if (x < 0.0f) return 0.0f;
    if (x > 1.0f) return 1.0f;
    return x;
}
static float easeOutCubic(float t) {
    t = clamp01(t);
    float u = 1.0f - t;
    return 1.0f - (u * u * u);
}
static float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

void Fade::Draw() {
    if (fadeState == FadeState::None) return;
    if (fadeDuration <= 0.0f) return;

    float t = clamp01(fadeTimer / fadeDuration);

    // p：覆い具合（0→1）
    // FadeIn は逆（1→0）
    float p = 0.0f;
    if (fadeState == FadeState::FadeOut)      p = t;
    else if (fadeState == FadeState::Wait)    p = 1.0f;
    else if (fadeState == FadeState::FadeIn)  p = 1.0f - t;

    p = easeOutCubic(p);
    GameLib::setBlendMode(Blender::BS_ALPHA);

    // うっすら暗くして演出の土台
    primitive::rect(
        0.0f, 0.0f,
        (float)SCREEN_W, (float)SCREEN_H,
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.20f * p
    );

    const float cardW = 90.0f;
    const float cardH = 130.0f;
    const float overlap = 10.0f; // 少し重ねて隙間を減らす

    const int cols = (int)std::ceil(SCREEN_W / (cardW - overlap));
    const int rows = (int)std::ceil(SCREEN_H / (cardH - overlap));
    const int total = cols * rows;

    // ディーラー位置（下中央から配る）
    const float sx = SCREEN_W * 0.5f;
    const float sy = SCREEN_H + 160.0f;

    // カード背面っぽい青（primitive::rect の青）
    const float r = 0.10f, g = 0.35f, b = 0.95f;

    // 配るテンポ
    const float stagger = 0.70f;

    // totalが変わったら順番作り直し（初回もここで作る）
    if ((int)dealOrder.size() != total) {
        PrepareDealOrder(total);
    }

    // s：出てくる順番（0→total-1）
    for (int s = 0; s < total; ++s) {
        // idx：塗りつぶす“行き先”がランダム
        int idx = dealOrder[s];

        int cx = idx % cols;
        int cy = idx / cols;

        float tx = cx * (cardW - overlap);
        float ty = cy * (cardH - overlap);

        // s番目の出現タイミング（順番はランダム、出るテンポは一定）
        float delay = (float)s / (float)total;
        float local = clamp01(p * (1.0f + stagger) - delay * stagger);
        local = easeOutCubic(local);

        if (local <= 0.0f) continue;

        // 少しバタつき
        float wob = std::sinf(fadeTimer * 18.0f + idx * 0.37f) * 6.0f * (1.0f - local);

        float x = lerp(sx, tx, local) + wob;
        float y = lerp(sy, ty, local);

        // “カード本体”
        primitive::rect(
            x, y, cardW, cardH,
            0.0f, 0.0f, 0.0f,
            r, g, b, 0.95f * local
        );

        // ハイライト
        primitive::rect(
            x + 3.0f, y + 3.0f, cardW - 6.0f, 2.0f,
            0.0f, 0.0f, 0.0f,
            1.0f, 1.0f, 1.0f, 0.25f * local
        );
        primitive::rect(
            x + cardW - 3.0f, y + 3.0f, 2.0f, cardH - 6.0f,
            0.0f, 0.0f, 0.0f,
            1.0f, 1.0f, 1.0f, 0.18f * local
        );
    }

    // 最後に薄く面で締める（隙間対策）
    primitive::rect(
        0.0f, 0.0f,
        (float)SCREEN_W, (float)SCREEN_H,
        0.0f, 0.0f, 0.0f,
        r, g, b, 0.08f * p
    );


}

void Fade::PrepareDealOrder(int total)
{
    dealOrder.resize(total);
    std::iota(dealOrder.begin(), dealOrder.end(), 0);
    std::shuffle(dealOrder.begin(), dealOrder.end(), rng);

}

