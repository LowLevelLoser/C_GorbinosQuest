#include <stdio.h>
#include <stdlib.h>
#include <curses.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include "game.h"
#include "util.h"

void InitCurses() {
    // Initialize screen
    initscr();
    
    // Check for color support
    if (!has_colors()) {
        endwin();
		printf("This terminal doesn't support colors");
        exit(1);
    }

    // Initialize colors
    start_color();

    // Use default colors if supported; fallback to black background
    if (can_change_color() && use_default_colors() == OK) {
        init_pair(CP_BLUE, COLOR_BLUE, -1);
        init_pair(CP_EMPTY, COLOR_WHITE, -1);
        init_pair(CP_RED, COLOR_RED, -1);
        init_pair(CP_GREEN, COLOR_GREEN, -1);
        init_pair(CP_YELLOW, COLOR_YELLOW, -1);
        init_pair(CP_PLAYER, COLOR_MAGENTA, -1);
    } else {
        // Fallback if terminal can't use default background
        init_pair(CP_BLUE,COLOR_BLUE, COLOR_BLACK);
        init_pair(CP_EMPTY,COLOR_WHITE, COLOR_BLACK);
        init_pair(CP_RED, COLOR_RED, COLOR_BLACK);
        init_pair(CP_GREEN,COLOR_GREEN, COLOR_BLACK);
        init_pair(CP_YELLOW, COLOR_YELLOW, COLOR_BLACK);
        init_pair(CP_PLAYER, COLOR_MAGENTA, COLOR_BLACK);
    }

    // Standard ncurses settings
    cbreak();                // pass key presses to program, but not signals
    noecho();                // don't echo key presses to screen
    keypad(stdscr, TRUE);    // allow arrow keys
    timeout(0);              // non-blocking input
    curs_set(0);             // hide cursor
    nodelay(stdscr, TRUE);   // ensure getch() is non-blocking
}


void SaveFile(const Game_t* game, const char* path) {
    FILE *f = fopen(path, "wb"); //The b is important for windows comaptibility
    if (!f) {
        perror("SaveFile fopen");
        return;
    }

    size_t written = fwrite(game, sizeof(Game_t), 1, f);
    if (written != 1) {
        perror("SaveFile fwrite");
    }

    fclose(f);
}

void LoadFile(Game_t* game, const char* path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        perror("LoadFile fopen");
        return;
    }

    size_t read = fread(game, sizeof(Game_t), 1, f);
    if (read != 1) {
        perror("LoadFile fread");
    }

    fclose(f);
}

// # Hot Reloading Implementation
struct timespec mod_time;
// Checks if the file changed
// fopen
bool file_changed(const char* path) {
    struct stat st;
    stat(path, &st);

    if (st.st_mtim.tv_sec != mod_time.tv_sec ||
        st.st_mtim.tv_nsec != mod_time.tv_nsec)
    {
        mod_time = st.st_mtim;
        return true;
    }

    return false;
}


void LoadFunctions(HotReloadAPI* api){
	api->handle = dlopen(api->path, RTLD_LAZY);
	if(api->handle == NULL){
		printf("failure\n");
		exit(1);
	}
	
	// Searches for symbols in our .so file
	api->RunGame_fn = dlsym(api->handle, "RunGame");
	api->RenderGame_fn = dlsym(api->handle, "RenderGame");
	api->InitGameData_fn = dlsym(api->handle, "InitGameData");
	api->GameSize_fn = dlsym(api->handle, "GameSize");

	bool failed_to_load = false;

	if(api->RunGame_fn == NULL){
		printf("RunGame could not be found\n");
		failed_to_load = true;
	}
	if(api->RenderGame_fn == NULL){
		printf("RenderGame could not be found\n");
		failed_to_load = true;
	}
	if(api->InitGameData_fn == NULL){
		printf("InitGameData could not be found\n");
		failed_to_load = true;
	}
	if(api->GameSize_fn == NULL){
		printf("GameSize could not be found\n");
		failed_to_load = true;
	}

	if (failed_to_load){
		exit(1);
	}

}

void UnloadFunctions(HotReloadAPI* api){
	dlclose(api->handle);
}

