#ifndef UTIL_H
#define UTIL_H

#include "render.h"
#include "logic.h"
#include "game.h"
#include <dlfcn.h>
#include <stdbool.h>

// Function pointer signatures of what we want to reload
typedef void (*RunGame_ptr)(void *game_ptr, bool *quit ,int key_pressed);
typedef void (*RenderGame_ptr)(void *game_ptr);
typedef void (*InitGameData_ptr)(void *game_ptr);
typedef size_t (*GameSize_ptr)();

typedef  struct { //Store all that's necessary for dynamic reloading declarations
	const char* path;
	void* handle;
	RenderGame_ptr RenderGame_fn;
	RunGame_ptr RunGame_fn;
	InitGameData_ptr InitGameData_fn ;
	GameSize_ptr GameSize_fn;
}HotReloadAPI;

enum ColorPairs {
	CP_BLUE = 1,
	CP_EMPTY,
	CP_RED,
	CP_GREEN,
	CP_YELLOW,
	CP_PLAYER
};

bool file_changed(const char* path);
void LoadFunctions(HotReloadAPI* api);
void UnloadFunctions(HotReloadAPI* api);


void InitCurses();
void SaveFile(const Game_t* game, const char* path);
void LoadFile(Game_t* game, const char* path);
bool file_changed(const char* path);

#endif
