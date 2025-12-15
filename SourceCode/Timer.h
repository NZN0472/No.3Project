#pragma once
#include <chrono>

class Timer {
public:
    // 初期化（ゲーム起動時に一度呼ぶ）
    void initialize();

    // 毎フレーム呼ぶ（時間更新）
    void update();

    // 前フレームとの経過時間（秒）
    float getDeltaTime() const;

    // 現在のFPS
    float getFPS() const;

    // 経過時間（ゲーム開始からの合計）
    float getTotalTime() const;

    // シングルトンインスタンス取得
    static Timer* getInstance();

private:
    Timer() = default;  // シングルトン用
    ~Timer() = default;

    std::chrono::steady_clock::time_point prevTime;
    std::chrono::steady_clock::time_point startTime;

    float deltaTime = 0.0f;
    float totalTime = 0.0f;
    float fps = 0.0f;
};