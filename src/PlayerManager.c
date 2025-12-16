#include <stdbool.h>
#include <stdlib.h>
#include "game.h"
#include "logic.h"
#include "render.h"

//TODO move this to logic.c
events_t MovePlayer(player_t* player, direction_t direction, Game_t *game){ 
	InitMapManager();
	coordinate_t temp_position = player->position;

	switch (direction) {
		case NORTH:
			player->position.y--;
			break;

		case SOUTH:
			player->position.y++;
			break;

		case EAST:
			player->position.x++;
			break;

		case WEST:
			player->position.x--;
			break;
		case NONE:
			break;
	}

	if(player->position.x < 0 || player->position.y < 0 || player->position.x > ROOM_SIZE - 1|| player->position.y > ROOM_SIZE - 1){
		player->position = temp_position; 	
		return NOTHING;
	}

	switch (game->board.grid[player->position.y][player->position.x]) {
		case COIN:
			return GET_GOLD;
			break;
		case ENTRY:
			return NEXT_ROOM;
			break;
		case EXIT:
			return NOTHING;
			break;
		case EMPTY:
			if (rand()%7 == 0) {
				return ENEMY_ENCOUNTER;
			}
			return NOTHING;
			break;	
		case WALL:
			player->position = temp_position;
			return NOTHING;
			break;
		case TREASURE:
			return GET_TREASURE;
			break;
		case WELL:
			return ENTER_WELL;
			break;
		case TRAP:
			return TRAP_TRIGGERED;
			break;
	}

	return NOTHING;
}

void HandleStats(player_t* player, Game_t* game){

	if (player->stats.gold > game->quota){
		game->sub_state = WIN;
	}

	if (player->stats.hp <= 0){
		game->sub_state = DEAD;
	}

	if (player->stats.hp > player->stats.max_hp){
		player->stats.hp = player->stats.max_hp;
	}

	if (player->stats.xp > player->stats.xp_threshold){
		//level up
		player->stats.max_hp += 20.0;
		player->stats.hp = player->stats.max_hp;
		player->stats.attack += 5;
		player->stats.defense += 2;
		player->stats.xp = 0;
		player->stats.xp_threshold *= 2;
		player->stats.xp_threshold += 5;
		player->level++;
		message_t message = {"You Leveled Up"};
		LogMessage(message);
	}

	if (player->stats.defense > 50.0){
		player->stats.defense = 50;
	}
}
