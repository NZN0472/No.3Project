#pragma once

#define XWB_BGM 0
#define XWB_SE  1
//BGM‚ÌID’è‹`
enum BGM_ID
{
	BGM_TITLE,//0
	BGM_GAME,//1
};
//SE‚ÌID’è‹`
enum SE_ID
{
	SE_STATE,//0
	SE_FADE,//1
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
	//Œ»İÄ¶‚³‚ê‚Ä‚¢‚éBGM
	static int currentBGM;
};