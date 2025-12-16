#ifndef LOGIC_H
#define LOGIC_H

#include "game.h"
#include "render.h"
#include <stddef.h>

void RunGame(void *game, bool *quit , int key_pressed);
void InitGameData(void* game_ptr);
size_t GameSize();

void InitMapManager();
void PlaceRoom(int col, int row);
void GenerateRoom(int size, int col, int row, Dir start, Dir end);
Room_t GetRoom();
void PlaceExits();
blocks_t GetBlocks();
void SetEntry(int row, int col);
void LitterMap();
coordinate_t GetExit();


typedef struct {
	Room_t room;
	coordinate_t entry;
	coordinate_t exit;
	blocks_t blocks;
} map_t ;

events_t MovePlayer(player_t* player, direction_t direction, Game_t* game);
void HandleStats(player_t* player, Game_t* game);

//Literally no reason to make this inherited


#endif
