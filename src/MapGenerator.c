#include <stdbool.h>
#include <stdlib.h>
#include "game.h"
#include "logic.h"

static map_t map_G;

typedef struct{
	direction_t dirs[5];
	int dir_head;
} direction_vec;

typedef struct{
	Dir dirs[4];
	int dir_head;
} Dir_vec;

direction_vec InitDirectionVec(){
	direction_vec ret = {
		.dirs = {NONE, NONE, NONE, NONE},
		.dir_head = 0
	};
	return ret;
}

Dir_vec InitDirVec(){
	Dir_vec ret = {
		.dirs = {{NONE, NONE}, {NONE, NONE}, {NONE, NONE}, {NONE, NONE}},
		.dir_head = 0
	};
	return ret;
}

void InitMapManager(){
	for (int i = 0; i < ROOM_SIZE*ROOM_SIZE/16; i++) {
		map_G.blocks.blocks[i] = (coordinate_t){0,0};
	}
	for (int i = 0; i < ROOM_SIZE; i++) {
		for (int j = 0; j < ROOM_SIZE; j++) {
			map_G.room.grid[i][j] = WALL;
		}
	}

	map_G.entry.y = -1;
	map_G.entry.x = -1;
	map_G.exit.y = -1;
	map_G.exit.x = -1;
	map_G.blocks.block_head = 0;
}

void PlaceExits(){
	map_G.room.grid[map_G.entry.y][map_G.entry.x] = ENTRY;
	map_G.room.grid[map_G.exit.y][map_G.exit.x] = EXIT; 
}

void PlaceRoom(int col, int row){
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
			map_G.room.grid[row + j][col + i] = EMPTY;
	if (map_G.entry.y < 0 || map_G.entry.x < 0) {
		map_G.entry.x = col + 1;
		map_G.entry.y = row + 1;
	}
	map_G.exit.y = row + 2;
	map_G.exit.x = col + 2;

}

void GenerateRoom(int size, int col, int row, Dir start, Dir end){
	// Base case
	if(size <= 4){
		PlaceRoom(col, row);
		map_G.blocks.blocks[map_G.blocks.block_head] = (coordinate_t){col, row};
		map_G.blocks.block_head++;
		return;
	}

	int half = size / 2;

	// --- 1. Randomize Undefined Start/Ends ---
	// We only randomize if the parent didn't FORCE a specific entry/exit.
	Dir temp_start = start;
	Dir temp_end = end;

	if (temp_start.horizontal == NONE) temp_start.horizontal = (direction_t)(EAST + (rand() % 2));
	if (temp_start.vertical == NONE)   temp_start.vertical   = (direction_t)(NORTH + (rand() % 2));

	if (temp_end.horizontal == NONE) temp_end.horizontal = (direction_t)(EAST + (rand() % 2));
	if (temp_end.vertical == NONE)   temp_end.vertical   = (direction_t)(NORTH + (rand() % 2));

	// --- 2. Self-Avoiding Walk ---
	Dir_vec visited = InitDirVec();
	visited.dirs[visited.dir_head]= temp_start;
	visited.dir_head++;
	Dir current = temp_start;

	// find path from temp_start to temp_end
	while (current.vertical != temp_end.vertical || current.horizontal != temp_end.horizontal) {
		direction_vec potential_moves = InitDirectionVec();
		direction_t v_move = (current.vertical == NORTH) ? SOUTH : NORTH;
		bool v_visited = false;

		for (int i = 0; i < visited.dir_head; i++) {
			Dir *node = &visited.dirs[i];

			if (node->vertical == v_move &&
					node->horizontal == current.horizontal) {
				v_visited = true;
				break;
			}
		}


		if (!v_visited){ 
			potential_moves.dirs[potential_moves.dir_head] = v_move;
			potential_moves.dir_head ++;
		}

		direction_t h_move = (current.horizontal == WEST) ? EAST : WEST;
		bool h_visited = false;

		for (int i = 0; i < visited.dir_head; i++) {
			Dir *node = &visited.dirs[i];

			if (node->vertical == current.vertical &&
					node->horizontal == h_move) {
				h_visited = true;
				break;
			}
		}

		if (!h_visited) {
			potential_moves.dirs[potential_moves.dir_head] = h_move;
			potential_moves.dir_head++;
		}

		if (potential_moves.dir_head == 0) break; 

		int random_index = rand() % potential_moves.dir_head;
		direction_t selected_move = potential_moves.dirs[random_index];

		if (selected_move == NORTH || selected_move == SOUTH) current.vertical = selected_move;
		else current.horizontal = selected_move;

		visited.dirs[visited.dir_head]= current;
		visited.dir_head++;
	}

	// --- 3. Recursion with SYNCHRONIZED Interfaces ---

	// We need to track the specific entry point for the *next* iteration.
	// For the very first map_G.room. the entry is the global 'start' passed in.
	Dir next_entry_constraints = start; 

	for (int i = 0; i < visited.dir_head; i++) {
		int x = col;
		int y = row;

		// Offset calculation
		if (visited.dirs[i].horizontal == EAST) x += half;
		if (visited.dirs[i].vertical == SOUTH)  y += half;

		// Define inputs for recursive call
		Dir local_start = next_entry_constraints;
		Dir local_end = {NONE, NONE}; // We will figure this out below

		// --- Determine the Exit (Interface with next map_G.room. ---
		if (i == visited.dir_head - 1) {
			// LAST ROOM: The exit is simply the global end
			local_end = end; 
		} else {
			// INTERMEDIATE: We must create a synchronized gate with the next map_G.room.
			Dir current_quad = visited.dirs[i];
			Dir next_quad = visited.dirs[i+1];

			// Check direction of travel between current and next quadrant
			if (current_quad.horizontal != next_quad.horizontal) {
				// Horizontal Move (East <-> West)
				// We must share the same VERTICAL coordinate (North or South).
				direction_t shared_vertical = (direction_t)(NORTH + (rand() % 2));

				// If moving East: Exit is East, Next Entry is West
				if (current_quad.horizontal == WEST) { // Moving West -> East
					local_end.horizontal = EAST; 
					local_end.vertical = shared_vertical; 

					// Prep next loop's entry
					next_entry_constraints.horizontal = WEST;
					next_entry_constraints.vertical = shared_vertical;
				} else { // Moving East -> West
					local_end.horizontal = WEST;
					local_end.vertical = shared_vertical;

					// Prep next loop's entry
					next_entry_constraints.horizontal = EAST;
					next_entry_constraints.vertical = shared_vertical;
				}
			} 
			else {
				// Vertical Move (North <-> South)
				// We must share the same HORIZONTAL coordinate (East or West).
				direction_t shared_horizontal = (direction_t)(EAST + (rand() % 2));

				if (current_quad.vertical == NORTH) { // Moving North -> South
					local_end.vertical = SOUTH;
					local_end.horizontal = shared_horizontal;

					next_entry_constraints.vertical = NORTH;
					next_entry_constraints.horizontal = shared_horizontal;
				} else { // Moving South -> North
					local_end.vertical = NORTH;
					local_end.horizontal = shared_horizontal;

					next_entry_constraints.vertical = SOUTH;
					next_entry_constraints.horizontal = shared_horizontal;
				}
			}
		}

		GenerateRoom(half, x, y, local_start, local_end);
	}
}

blocks_t GetBlocks(){
	return map_G.blocks;
}

void SetEntry(int row, int col){
	map_G.entry = (coordinate_t){col, row};
}

void LitterMap(){ //Potentially overlap with entry
	int r;
	for(int i = 0; i < map_G.blocks.block_head; i++){ 
		r = rand();
		int x = rand()%4 + map_G.blocks.blocks[i].x;
		int y = rand()%4 + map_G.blocks.blocks[i].y;
		if(r % 5 == 0 || r % 5 == 1){
			map_G.room.grid[y][x] = COIN;
		}
		else if(r%5 == 3){
			map_G.room.grid[y][x] = TREASURE;
		}
		else {
			map_G.room.grid[y][x] = TRAP;
			int x = rand()%4 + map_G.blocks.blocks[i].x;
			int y = rand()%4 + map_G.blocks.blocks[i].y;
			map_G.room.grid[y][x] = TRAP;
		}
	}
	int i = map_G.blocks.block_head/2;
	int x = rand()%4 + map_G.blocks.blocks[i].x;
	int y = rand()%4 + map_G.blocks.blocks[i].y;
	map_G.room.grid[y][x] = WELL;
}

coordinate_t GetExit(){return map_G.exit;}
Room_t GetRoom(){return map_G.room;}
