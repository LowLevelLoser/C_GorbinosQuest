#ifndef GAME_H
#define GAME_H

#define TIME_PER_FRAME_IN_MS 18
#define ROOM_SIZE 32
#define MAX_INVENTORY 10
#define LOG_MAX_LINES (ROOM_SIZE - 2)
#define LOG_WIDTH 26

//HACK actually implement logging one dai ;)
#define LOG_MESSAGE(...) do{ \
	message_t message = { #__VA_ARGS__ };\
	LogMessage(message); }\
while(0)

//TODO turn structs and enums into its own type

typedef enum {
	NONE,
	NORTH,
	SOUTH,
	EAST,
	WEST
}direction_t;

typedef enum { 
	NOTHING,
	GET_GOLD,
	ENEMY_ENCOUNTER,
	GET_TREASURE,
	TRAP_TRIGGERED,
	ENTER_WELL,
	NEXT_ROOM //Undo boosts and effects
}events_t;

typedef enum {
	QUIT_GAME,
	MENU,
	RUNNING
}state_t;

typedef enum {
	OVERWORLD,
	FIGHT,
	WELL_AREA,
	DEAD,
	WIN
}sub_state_t;

typedef enum { 
	EMPTY,
	WALL,
	COIN,
	ENTRY,
	TREASURE,
	TRAP,
	WELL,
	EXIT, 
}tile_t;


#define GOBLIN_INIT {"Goblin", GOBLIN, {15, 15, 5, 2, 10}}

#define SKELETON_INIT {"Skeleton", SKELETON, {50, 50, 15, 8, 50}}

#define DRAGON_INIT {"Dragon", DRAGON, {100, 100, 25, 15, 200}}


enum enemy_type_t{ // There is some macro magic to make this simpler
	NO_ENEMY,
	SKELETON,
	GOBLIN,
	DRAGON
};

typedef struct {
	float hp;
	float max_hp;
	float atk;
	float defense;
	int gold_reward;
}enemy_stats_t;

typedef struct {
    char name[16];
	enum enemy_type_t enemy_type;
	enemy_stats_t stats;
} Enemy_t;

typedef struct {
	int x;
	int y;
}coordinate_t;

typedef struct {
    float hp;
    float max_hp;
	float xp;
	float xp_threshold;
    float attack;
    float defense;
    int gold;
}stats_t ;


typedef struct {
	coordinate_t position;
	int level;
	stats_t stats;
}player_t ;

typedef  struct {
	tile_t grid[ROOM_SIZE][ROOM_SIZE];
}Room_t ;

typedef struct {
	direction_t horizontal;
	direction_t vertical;
}Dir;

typedef struct{
	char message [64];
} message_t;

typedef struct{
	message_t messages[LOG_MAX_LINES];
	int head;
	int length;
} game_log_t;

typedef struct { //This stores all the necessary game data DO NOT involve dynamic allocation or pointers
	state_t state;
	sub_state_t sub_state;
	Room_t board;
	int quota;
	int level;
	player_t player; 
	Enemy_t active_enemy;
}Game_t;

typedef struct{
	coordinate_t blocks[ROOM_SIZE*ROOM_SIZE/16];
	int block_head;
} blocks_t;


#endif
