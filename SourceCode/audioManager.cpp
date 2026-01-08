#include"audioManager.h"
#include "../GameLib/game_lib.h"
using namespace GameLib;

int AudioManager::currentBGM = -1;

void AudioManager::Init()
{
	//sound::load(XWB_BGM, L"./Data/Sounds/bgm.xwb");
    //sound::load(XWB_SE, L"./Data/Sounds/se.xwb");
}
// BGMÄ¶
void AudioManager::PlayBGM(int id)
{
    if (currentBGM == id) return;

    StopBGM();//Šù‚É•Ê‚ÌBGM‚ª—¬‚ê‚Ä‚¢‚½‚ç~‚ß‚é
    sound::play(id, true);//ƒ‹[ƒvÄ¶
    currentBGM = id;//¡—¬‚ê‚Ä‚¢‚éBGM‹L˜^
}

// BGM’â~
void AudioManager::StopBGM()
{
    if (currentBGM != -1)
    {
        sound::stop(XWB_BGM, currentBGM);
        currentBGM = -1;
    }
}

// SEÄ¶
void AudioManager::PlaySE(int id)
{
    sound::play(id, false);
}