#pragma once

#define XWB_BGM 0
#define XWB_SOUNDS  1
#define XWB_GAME  2

//BGMのID定義
enum BGM_ID
{
	BGM_TITLE,//0
	BGM_GAME,//1
};
//SEのID定義
enum SOUNDS_ID
{
	SE_STATE,
	SE_KIRI,
	SE_A,


};

enum GAME_SE {
	CHIPS ,   //チップ+-
	handOutCards,//カード配布
	cardOpen,    //カードめくる
	SAFE,        //イカサマ判定(セーフ)
	CAUGHT,      //(アウト)
	THRY,        //(冤罪)
	RESULT,		 //リザルト
};

class AudioManager
{
public:
	static void Init();
	static void Update();
	static void PlayBGM(int id);
	static void StopBGM();
	static void PlaySE(int id);
	static void PlayGameSE(int id);
private:
	//現在再生されているBGM
	static int currentBGM;
};