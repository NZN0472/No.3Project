#pragma once

#define XWB_BGM 0
#define XWB_SOUNDS  1

//BGMÇÃIDíËã`
enum BGM_ID
{
	BGM_TITLE,//0
	BGM_GAME,//1
};
//SEÇÃIDíËã`
enum SOUNDS_ID
{
	SE_STATE,
	SE_KIRI,
	SE_A,


};
class AudioManager
{
public:
	static void Init();
	static void Update();
	static void PlayBGM(int id);
	static void StopBGM();
	static void PlaySE(int id);
private:
	//åªç›çƒê∂Ç≥ÇÍÇƒÇ¢ÇÈBGM
	static int currentBGM;
};