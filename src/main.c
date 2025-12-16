#include <stdio.h>
#include <curses.h> 
#include <dlfcn.h>
#include <time.h>
#include <stdlib.h>
#include "game.h"
#include "util.h"

// Instances of extern "C" are just for linking like C

int main() {
	InitCurses();
    srand(time(0));

	HotReloadAPI hot_fn; //Boot strappinng Hot Reloading
	hot_fn.path = "build/gamehot.so";
	LoadFunctions(&hot_fn);
	bool quit = false;
	size_t game_size = hot_fn.GameSize_fn();
	void* game = malloc(game_size);
	//void* game = ::operator new(game_size);
	hot_fn.InitGameData_fn(game);

	while(!quit){ //Game loop
		doupdate();
		napms(TIME_PER_FRAME_IN_MS);

		if(file_changed(hot_fn.path)){
			UnloadFunctions(&hot_fn);
			napms(10);
			LoadFunctions(&hot_fn);
			//Check if Game_t changed and re initialize if so
			if(game_size != hot_fn.GameSize_fn()){
				game_size = hot_fn.GameSize_fn();
				free(game);
				void* game = malloc(game_size);
				hot_fn.InitGameData_fn(game);
			}
		}

		hot_fn.RunGame_fn(game, &quit, getch());
		hot_fn.RenderGame_fn(game);
	}
	
	endwin();
	UnloadFunctions(&hot_fn);
	free(game);
	return 0;
}

