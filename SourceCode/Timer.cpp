#include "Timer.h"

Timer* Timer::getInstance() {
    static Timer instance;
    return &instance;
}

void Timer::initialize() {
    startTime = std::chrono::steady_clock::now();
    prevTime = startTime;
    deltaTime = 0.0f;
    totalTime = 0.0f;
}

void Timer::update() {
    auto now = std::chrono::steady_clock::now();

    // åoâﬂïbêî
    deltaTime = std::chrono::duration<float>(now - prevTime).count();
    totalTime = std::chrono::duration<float>(now - startTime).count();

    // FPSåvéZ
    if (deltaTime > 0.0f)
        fps = 1.0f / deltaTime;
    else
        fps = 0.0f;

    prevTime = now;
}

float Timer::getDeltaTime() const {
    return deltaTime;
}

float Timer::getFPS() const {
    return fps;
}

float Timer::getTotalTime() const {
    return totalTime;
}
