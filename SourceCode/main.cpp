#include "../GameLib/game_lib.h"
#include"all.h"
#include "Timer.h"
#include "Fade.h"

int curScene = SCENE_NONE;
int nextScene = SCENE_TITLE;

static Fade gFade;

//プロトタイプ宣言
static void SwitchScene(int from, int to);

int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
	GameLib::init(L"ゲームタイトル(完成時必ず変える)", SCREEN_W, SCREEN_H,true);

	//timerの初期設定
	auto* timer = Timer::getInstance();
	timer->initialize();

	while (GameLib::gameLoop())
	{
		//時間を更新
		timer->update();
		//デルタタイム取得
		float dt = timer->getDeltaTime();

		if (dt > 0.1)dt = 0.1f;//暴走防止（ウィンドウ操作などでdtが跳ねたとき）

		//フェードの更新
		gFade.Update(dt);
		
			// 初回だけシーン初期化
			if (curScene == SCENE_NONE)
			{
				SwitchScene(curScene, nextScene);
				curScene = nextScene;
				gFade.StartFadeIn(1.0f);
				
			}

			// フェードアウト開始
			if (curScene != nextScene && !gFade.IsFading())
			{
				gFade.StartFadeOut(1.0f);
			}

			//  フェードイン
			if (curScene != nextScene && gFade.IsFadeOutFinished())
			{
				SwitchScene(curScene, nextScene);
				curScene = nextScene;
				gFade.StartFadeIn(1.0f);
			}
			
			
		

		input::update();
		
		switch (curScene)
		{
		case SCENE_TITLE:
			title_update();
			title_render();
			break;
		case SCENE_GAME:
			game_update();
			game_render();
			break;
		}
		GameLib::setBlendMode(Blender::BS_ALPHA);
		gFade.Draw();
		debug::display(1, 1, 1, 1, 1);
		GameLib::present(1, 0);

	}

	// ゲームライブラリの終了処理 
	GameLib::uninit();
	return 0;
}


//シーンの破棄と新たなシーンのインプット
static void SwitchScene(int from, int to) {

	switch (from)
	{
	case SCENE_TITLE: title_deinit(); break;
	case SCENE_GAME:  game_deinit();  break;
	}

	switch (to)
	{
	case SCENE_TITLE: title_init(); break;
	case SCENE_GAME:  game_init();  break;
	}

}