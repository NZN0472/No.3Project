#include"all.h"
#include "Blackjack.h"

int game_state;
int game_timer;
static BlackjackGame gBJ;



void game_init() {
	game_state = 0;
	game_timer = 0;

	gBJ.init();
}
void game_deinit() {
	gBJ.deinit();
}
void game_update() {
	switch (game_state)
	{
	case 0:
		//////// ‰Šúİ’è //////// 
		
		game_state++;
		/*fallthrough*/
	case 1:
		//////// ƒpƒ‰ƒ[ƒ^‚Ìİ’è //////// 
		game_state++;
		/*fallthrough*/
	case 2:
		//////// ’Êí //////// 
		gBJ.update();
		break;
	}
	game_timer++;
}
void game_render() {
	
	GameLib::clear(1, 1, 1);
	//sprite_render(title, 0, 0);
	gBJ.render();
}