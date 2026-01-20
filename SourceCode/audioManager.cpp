#include"audioManager.h"
#include "../GameLib/game_lib.h"
using namespace GameLib;

int AudioManager::currentBGM = -1;

void AudioManager::Init()
{
    music::load(BGM_TITLE, L"./Data/Sounds/titlebgm.wav", 1.0f);
    music::load(BGM_GAME, L"./Data/Sounds/gamebgm.wav", 1.0f);
    sound::load(XWB_SOUNDS, L"./Data/Sounds/SE.xwb", 1.0f);
}
void AudioManager::Update() {}
// BGMÄ¶
void AudioManager::PlayBGM(int id)
{
    if (currentBGM == id) return;

    StopBGM();//Šù‚É•Ê‚ÌBGM‚ª—¬‚ê‚Ä‚¢‚½‚ç~‚ß‚é
    music::play(id, true);//ƒ‹[ƒvÄ¶
    currentBGM = id;//¡—¬‚ê‚Ä‚¢‚éBGM‹L˜^
}

// BGM’â~
void AudioManager::StopBGM()
{
    if (currentBGM != -1)
    {
        music::stop(currentBGM);
        currentBGM = -1;
    }
}

// SEÄ¶
void AudioManager::PlaySE(int id)
{
    sound::play(XWB_SOUNDS, id);
}
