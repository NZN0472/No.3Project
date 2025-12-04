#include "../GameLib/game_lib.h"
#include"all.h"


//ゲーム作りたくねー！！！！
int curScene = SCENE_NONE;
int nextScene = SCENE_TITLE;

int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
	GameLib::init(L"ゲームタイトル(完成時必ず変える)", SCREEN_W, SCREEN_H,true);
	
	

	while (GameLib::gameLoop())
	{
		// シーン切り替え処理 
		if (curScene != nextScene)
		{
			// 現在のシーンに応じた終了処理 
			switch (curScene)
			{
			case SCENE_TITLE:
				title_deinit();
				break;
			case SCENE_GAME:
				game_deinit();
				break;
			}
			// 次のシーンに応じた初期設定処理 
			switch (nextScene)
			{
			case SCENE_TITLE:
				title_init();
				break;
			case SCENE_GAME:
				game_init();
				break;
			}
			// nextScene がcurScene になる 
			curScene = nextScene;
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
		debug::display(1, 1, 1, 1, 1);
		GameLib::present(1, 0);

	}
	// 現在のシーンに応じた終了処理を行う 
	switch (curScene)
	{
	case SCENE_TITLE:
		title_deinit();
		break;
	case SCENE_GAME:
		game_deinit();
		break;
	}

	// ゲームライブラリの終了処理 
	GameLib::uninit();
	return 0;
}