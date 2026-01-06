#include "all.h"

void audio_init()
{
	// 効果音
	sound::load(XWB_SE1, L"./Data/Sounds/SE1.xwb");

}

void audio_deinit()
{
	// 音楽ファイルクリア
	music::clear();
}