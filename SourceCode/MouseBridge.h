#pragma once

class MouseBridge {
public:
    // 毎フレーム1回だけ呼ぶ
    static void Update();

    // 左クリック押した瞬間
    static bool TrgL();

    // 左クリック押しっぱなし
    static bool DownL();

private:
    static bool prevL;
    static bool curL;
    static bool trgL;
};
