#include <stdlib.h>
#include <curses.h>
#include "game.h"
#include "logic.h"
#include "util.h"

// Group of messages

void EventHandler(events_t event, Game_t* game){
	InitMapManager();
	int roll = rand() % 100;
	int value = (rand()%5 + 1)*game->level;

	switch (event) {
	case NOTHING:
		return;
		break;
	case GET_GOLD:
		game->board.grid[game->player.position.y][game->player.position.x] = EMPTY;
		game->player.stats.gold += 5*game->level;
		//LOG_MESSAGE("You pick up " + std::to_string(5*game->level) + " coins.");
		LOG_MESSAGE(You picked up gold);
		return;
		break;

	case ENEMY_ENCOUNTER:
		if (roll > 50) {
			game->active_enemy = (Enemy_t)GOBLIN_INIT;
			game->sub_state = FIGHT;
		}
		if (roll > 75) game->active_enemy = (Enemy_t)SKELETON_INIT;
		if (roll > 90) game->active_enemy = (Enemy_t)DRAGON_INIT;
		return;
		break;

	case GET_TREASURE:
			game->board.grid[game->player.position.y][game->player.position.x] = EMPTY;
			game->player.stats.gold += value;
			//LOG_MESSAGE("You find treasure worth " + std::to_string(value) + " gold.");
			LOG_MESSAGE(You picked up treasure);
		break;

	case ENTER_WELL:
			LOG_MESSAGE(You approach a well);
			game->sub_state = WELL_AREA;
		break;

	case TRAP_TRIGGERED:
			game->board.grid[game->player.position.y][game->player.position.x] = EMPTY;
			game->player.stats.hp--;
			LOG_MESSAGE(You fell into a trap);
		break;

	case NEXT_ROOM:
			{
				GenerateRoom(ROOM_SIZE, 0, 0, (Dir){NONE,NONE}, (Dir){NONE,NONE});
				blocks_t blocks = GetBlocks();
				int last_block = blocks.block_head + 1;
				GenerateRoom(ROOM_SIZE, 0, 0, (Dir){NONE,NONE}, (Dir){NONE,NONE});
				blocks = GetBlocks();
				SetEntry(blocks.blocks[last_block].y, blocks.blocks[last_block].x);
				LitterMap();
				PlaceExits();
				game->board = GetRoom();
			}
			game->level++;
			game->player.stats.xp += game->player.stats.xp_threshold* .3;
			LOG_MESSAGE(You go down a floor);
			game->player.position = GetExit();
		break;
	};

}

//FIX logging for enemies
//NOTE might be infested
void HandleFight(Game_t* game, Enemy_t *active_enemy, int key_pressed) {
    // 1. STATE PERSISTENCE
    // We use a static variable to keep the enemy alive across function calls
    // without modifying your Game_t struct.

    // 2. INITIALIZATION (First frame of combat)
    if (active_enemy->enemy_type == NO_ENEMY) {
        // Create the enemy object based on what is stored in game struct
		//DisplayEnemyInfo()
        LOG_MESSAGE(Battle Start! Press 'a' to Attack, 'r' to Run.);
        return; // Wait for next input
    }

    // 3. INPUT HANDLING (Non-blocking)
    if (key_pressed == 0 || key_pressed == -1) {
        return; // No input do nothing this frame
    }

    // 4. PLAYER TURN
    bool player_acted = false;

    switch (key_pressed) {
        case 'a':
        {
            // Calculate Damage
            // NOTE: Assuming defense is a percentage
            float dmg_mult = (1.0f - (active_enemy->stats.defense / 100.0f));
            int damage = game->player.stats.attack * dmg_mult;
            
            // Ensure minimum damage of 1
            if (damage < 1) damage = 1; 

            //LOG_MESSAGE("You attacked " + std::string(active_enemy->GetType() == GOBLIN ? "Goblin" : "Enemy") + "!");
            //active_enemy->TakeDamage(damage);
            active_enemy->stats.hp -= damage;
            player_acted = true;
            break;
        }
        case 'r': // Run
        {
            // Simple 50% chance to flee
            if (rand() % 2 == 0) {
				active_enemy->enemy_type = NO_ENEMY;
				game->sub_state = OVERWORLD;
                LOG_MESSAGE(You ran away safely!);
                return;
            } else {
                LOG_MESSAGE(You failed to run!);
                player_acted = true; // Enemy gets a free hit
            }
            break;
        }
        default:
            return; // Don't let enemy attack if player mistyped
    }

    // 5. CHECK VICTORY
    if (active_enemy->stats.hp <= 0) {
        LOG_MESSAGE(Victory!);
        // Loot logic
        int gold = active_enemy->stats.gold_reward;
        float xp = active_enemy->stats.gold_reward / 1.5; // Simple XP formula
        
        game->player.stats.gold += gold;
        game->player.stats.xp += xp;
        
        LOG_MESSAGE(Gained  XP.);
		active_enemy->enemy_type = NO_ENEMY;
		game->sub_state = OVERWORLD;
        return;
    }

    // 6. ENEMY TURN (Only if player acted and enemy is still alive)
    if (player_acted) {
        //active_enemy->PerformAttack(game->player, game->level);
		game->player.stats.hp -= active_enemy->stats.atk*(1.0 - game->player.stats.defense/100)*(1.0+(.5)*(game->level-1.0));
        // 7. CHECK DEFEAT
        if (game->player.stats.hp <= 0) {
            LOG_MESSAGE(You have been defeated...);
            game->sub_state = DEAD;
        }  
	}
}

size_t GameSize(){
	return sizeof(Game_t);
}

void InitGameData(void* game_ptr){
	Game_t temp;
	temp.state = MENU;
	temp.sub_state = OVERWORLD;
	
	for (int i = 0; i < ROOM_SIZE; i++) {
		for (int j = 0; j < ROOM_SIZE; j++) {
		temp.board.grid[i][j] = WALL;
		}
	}


	stats_t default_player_stats = {
		.hp = 20.0,
		.max_hp = 20.0,
		.xp = 0.0,
		.xp_threshold = 10.0,
		.attack = 10.0,
		.defense = 5.0,
		.gold = 0,
	};
	

	temp.level = 1;
	temp.player.level = 1;
	temp.player.stats = default_player_stats;
	temp.quota = 1000;

	*(Game_t *)game_ptr = temp;
}

void RunGame(void *game_ptr, bool* quit, int key_pressed){
	
	Game_t* game = (Game_t *)game_ptr;
	player_t* player = &game->player;
	char savefile[] = "savefile";

	if(game->state == MENU){
		switch (key_pressed) {
		case '1':
			InitGameData(game_ptr);
			game = (Game_t *)game_ptr;		
			{
				InitMapManager();
				GenerateRoom(ROOM_SIZE, 0, 0, (Dir){NONE,NONE}, (Dir){NONE,NONE});
				blocks_t blocks = GetBlocks();
				int last_block = blocks.block_head;
				GenerateRoom(ROOM_SIZE, 0, 0, (Dir){NONE,NONE}, (Dir){NONE,NONE});
				blocks = GetBlocks();
				SetEntry(blocks.blocks[last_block].y, blocks.blocks[last_block].x);
				LitterMap();
				PlaceExits();
				game->board = GetRoom();
				game->player.position = GetExit();
			 }

			 game->state = RUNNING;
			 break;
		case '2':
				 LoadFile(game, savefile);
				 break;
		case '3':
		case 'q':
				 *quit = true;
				 game->state = QUIT_GAME;
				 break;
		}
		return;
	} 

	if(game->sub_state == OVERWORLD) {
		events_t event = NOTHING;
		switch (key_pressed) {
			case KEY_UP:
				event = MovePlayer(player, NORTH, game);
				break;
			case KEY_DOWN:
				event = MovePlayer(player, SOUTH, game);
				break;
			case KEY_RIGHT:
				event = MovePlayer(player, EAST, game);
				break;
			case KEY_LEFT:
				event = MovePlayer(player, WEST, game);
				break;
			case 'q':
				game->state = MENU;
				break;
				break;
			case 's':
				SaveFile(game, savefile);
				LOG_MESSAGE("Game saved.");
				break;
			case 'l':
				LoadFile(game, savefile);
				LOG_MESSAGE("Game loaded.");
				break;
		}	
		EventHandler(event, game);
		event = NOTHING;
	}

	if (game->sub_state == FIGHT){
		HandleFight(game, &game->active_enemy, key_pressed);
	}

	else if (game->sub_state == WELL_AREA) {
		int roll = rand();
		switch(key_pressed){
			case 'd':
				if (game->player.stats.gold < 10){
					LOG_MESSAGE("Come back when you're a little richer");
					break;
				}
				game->player.stats.gold -= 10;
				if(roll % 5 == 0){
					game->player.stats.hp += 15;
					LOG_MESSAGE("It heals you");
				}
				else if (roll % 5 == 1) {
					game->player.stats.xp += 10;
					LOG_MESSAGE("You feel more experienced");
				}
				else if (roll % 5 == 2) {
					LOG_MESSAGE("You feel more lucky");
				}
				else{
					LOG_MESSAGE("Nothing Happened");
				}
				break;
			case 'l':
				game->sub_state = OVERWORLD;
				LOG_MESSAGE("You go back to the dungeon");
				break;

		}
	}

	if (key_pressed == 'q'){
			game->state = MENU;
	}

	if (key_pressed == '?'){ //Debug key Don't tell anyone
			game->player.stats.gold += 100;
	}

	HandleStats(player, game);
}
