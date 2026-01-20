#pragma once
#include "all.h"

struct BlackjackAssets {
    Sprite* sprBJKA_B = nullptr;
    Sprite* sprBJKA_R = nullptr;
    Sprite* sprB36_B = nullptr;
    Sprite* sprB36_R = nullptr;
    Sprite* sprB710_B = nullptr;
    Sprite* sprB710_R = nullptr;
    Sprite* spr2Jo = nullptr;
    Sprite* sprMark = nullptr;
    Sprite* sprBack = nullptr;
    Sprite* sprBackBJ = nullptr;

    Sprite* titleBtn = nullptr;
    Sprite* gameBg = nullptr;

    Sprite* sprCheatOn = nullptr;
    Sprite* sprCheatOff = nullptr;

    Sprite* sprM100 = nullptr;
    Sprite* sprM50 = nullptr;
    Sprite* sprM10 = nullptr;
    Sprite* sprP100 = nullptr;
    Sprite* sprP50 = nullptr;
    Sprite* sprP10 = nullptr;

    Sprite* sprCpu1 = nullptr;
    Sprite* sprCpu2 = nullptr;
    Sprite* sprCpu3 = nullptr;
    Sprite* sprSkip = nullptr;

    Sprite* sprHit = nullptr;
    Sprite* sprStand = nullptr;
    Sprite* sprDoubl = nullptr;
    Sprite* sprNext = nullptr;
    Sprite* sprNewGame = nullptr;
    Sprite* sprBet = nullptr;

    Sprite* sprMinus = nullptr;
    Sprite* sprPlus = nullptr;

    Sprite* sprPauseBtn = nullptr;      // Pause.png（左上のボタン表示にも使う）
    Sprite* sprReturnGame = nullptr;    // Return_Game.png
    Sprite* sprWhatBJ = nullptr;        // whatBJ.png
    Sprite* sprCheat1 = nullptr;        // Lets_Cheat1.png
    Sprite* sprCheat2 = nullptr;        // Lets_Cheat2.png
    Sprite* sprMultBtn = nullptr;       // score.png（ボタン用）
    Sprite* sprMultInfo = nullptr;      // score.png（700x350 表）
    Sprite* sprBackBtn = nullptr;      // 


    void load();
    void unload();
};
