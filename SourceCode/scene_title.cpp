#include "all.h"

int title_state;
int title_timer;

Sprite* sprCar;

void title_init()  {
	title_state = 0;
	title_timer = 0;
}
void title_deinit(){
	safe_delete(sprCar);
}
void title_update(){
	switch (title_state)
	{
	case 0:
		//////// ‰Šúİ’è //////// 
		sprCar = sprite_load(L"./Data/Images/right1.png");
		title_state++;
		/*fallthrough*/
	case 1:
		//////// ƒpƒ‰ƒ[ƒ^‚Ìİ’è //////// 
		GameLib::setBlendMode(Blender::BS_ALPHA);
		title_state++;
		/*fallthrough*/
	case 2:
		//////// ’Êí //////// 
		if (TRG(0) & PAD_START)
		{
			nextScene = SCENE_GAME; break;
		}
		break;
	}
	title_timer++;

}
void title_render(){
	GameLib::clear(1, 0, 0);
	sprite_render(sprCar, 200, 200);
}