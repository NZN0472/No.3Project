#include "BlackjackAssets.h"
#include "all.h"

void BlackjackAssets::load() {
    gameBg = sprite_load(L"./Data/Images/game.png");

    sprBJKA_B = sprite_load(L"Data/Images/Black_JQKA.png");
    sprBJKA_R = sprite_load(L"Data/Images/Red_JQKA.png");

    sprB36_B = sprite_load(L"Data/Images/Black3_4_5_6.png");
    sprB36_R = sprite_load(L"Data/Images/Red3_4_5_6.png");

    sprB710_B = sprite_load(L"Data/Images/Black7_8_9_10.png");
    sprB710_R = sprite_load(L"Data/Images/Red7_8_9_10.png");

    spr2Jo = sprite_load(L"Data/Images/2_jo.png");
    sprMark = sprite_load(L"Data/Images/mark.png");
    sprBack = sprite_load(L"Data/Images/backCard.png");
    sprBackBJ = sprite_load(L"Data/Images/D_back.png");

    titleBtn = sprite_load(L"./Data/Images/titleBtn2.png");

    sprCheatOn = sprite_load(L"./Data/Images/CheatOn.png");
    sprCheatOff = sprite_load(L"./Data/Images/CheatOff.png");

    sprM100 = sprite_load(L"./Data/Images/100-.png");
    sprM50 = sprite_load(L"./Data/Images/50-.png");
    sprM10 = sprite_load(L"./Data/Images/10-.png");
    sprP100 = sprite_load(L"./Data/Images/100+.png");
    sprP50 = sprite_load(L"./Data/Images/50+.png");
    sprP10 = sprite_load(L"./Data/Images/10+.png");

    sprCpu1 = sprite_load(L"./Data/Images/CPU1.png");
    sprCpu2 = sprite_load(L"./Data/Images/CPU2.png");
    sprCpu3 = sprite_load(L"./Data/Images/CPU3.png");
    sprSkip = sprite_load(L"./Data/Images/Skip.png");

    sprHit = sprite_load(L"./Data/Images/Hit.png");
    sprStand = sprite_load(L"./Data/Images/Stand.png");
    sprDoubl = sprite_load(L"./Data/Images/DoublDown.png");
    sprNext = sprite_load(L"./Data/Images/Next.png");
    sprNewGame = sprite_load(L"./Data/Images/NewGame.png");
    sprBet = sprite_load(L"./Data/Images/Bet.png");

    sprMinus = sprite_load(L"./Data/Images/-btn.png");
    sprPlus = sprite_load(L"./Data/Images/+btn.png");
}

static void del(Sprite*& p) { if (p) { safe_delete(p); p = nullptr; } }

void BlackjackAssets::unload() {
    del(sprBJKA_B); del(sprBJKA_R);
    del(sprB36_B);  del(sprB36_R);
    del(sprB710_B); del(sprB710_R);
    del(spr2Jo); del(sprMark); del(sprBack); del(sprBackBJ);
    del(titleBtn); del(gameBg);

    del(sprCheatOn); del(sprCheatOff);

    del(sprM100); del(sprM50); del(sprM10);
    del(sprP100); del(sprP50); del(sprP10);

    del(sprCpu1); del(sprCpu2); del(sprCpu3); del(sprSkip);

    del(sprHit); del(sprStand); del(sprDoubl);
    del(sprNext); del(sprNewGame); del(sprBet);

    del(sprMinus); del(sprPlus);
}
