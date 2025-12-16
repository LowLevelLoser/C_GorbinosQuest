#include <curses.h>
#include <string.h>
#include "game.h"
#include "util.h"

static game_log_t game_log = {
	.head = -1,
	.length = 0
};

//TODO make this a do while loop thingy
//MACRO jank to make drawing ascii art less cumbersome
#define DRAW_ASCII_ART const int drawing_rows = sizeof(drawing) / sizeof(drawing[0]);  \
												for (int r = 0; r < drawing_rows; r++) { \
													move(startY + r, startX); \
													printw("%s", drawing[r]); \
												} \



void RenderScenes(Game_t* game) {
	const int rows = ROOM_SIZE;
	const int cols = ROOM_SIZE;
	const int cellWidth = 2;

	int termRows, termCols;
	getmaxyx(stdscr, termRows, termCols);

	int displayHeight = rows;
	int displayWidth = cols * cellWidth;

	// Centering logic
	int startY = (termRows - displayHeight) / 2;
	int startX = (termCols - displayWidth) / 2;

	// Shift slightly left if possible to make room for UI on the right
	startX -= 10; 

	if (startY < 0) startY = 0;
	if (startX < 0) startX = 0;

	attron(A_BOLD);
	if (game->sub_state == FIGHT){
		switch(game->active_enemy.enemy_type){
			case NO_ENEMY:
				return;
				break;
			case GOBLIN:
				attron(COLOR_PAIR(4));
				{
					const char* drawing[] = { 
#include "goblin.txt" //FIX For some reason makes everything green
					};
					DRAW_ASCII_ART

					attroff(COLOR_PAIR(4));
				}
				return;
				break;
			case SKELETON:
				{
					const char* drawing[] = { 
#include "skeleton.txt"
					};
					DRAW_ASCII_ART
				}
				return;
				break;
			case DRAGON:
				attron(COLOR_PAIR(3));
				{
					const char* drawing[] = { 
#include "dragon.txt"
					};
					DRAW_ASCII_ART
						attroff(COLOR_PAIR(3));
				}
				return;
				break;
		}
	}

	else if (game->sub_state == WELL_AREA) {
		const char* drawing[] = { 
#include "well.txt"
		};
		DRAW_ASCII_ART

	}

	else if (game->sub_state == DEAD) {
		const char* drawing[] = { 
#include "grave.txt"
		};
		DRAW_ASCII_ART
	}

	else if (game->sub_state == WIN) {
		attron(COLOR_PAIR(5));
		const char* drawing[] = { 
#include "win_screen.txt"
		};
		DRAW_ASCII_ART
			attroff(COLOR_PAIR(5));
	}
	attroff(A_BOLD);
}

void LogMessage(message_t message) {
	if(game_log.length < LOG_MAX_LINES) game_log.length++;
	game_log.head++;
	game_log.head = game_log.head % LOG_MAX_LINES;
	game_log.messages[game_log.head] = message; //When rendering start rendering from head and go backwards length amount of lines
}

void DrawLog(int mapStartY, int logX, int logHeight) {

	int currentY = mapStartY;

	/* ---------- Header ---------- */

	const char *header = "== **LOG** ==";
	int header_len = strlen(header);
	int padding = (LOG_WIDTH - header_len) / 2;
	int headerX = logX + padding;

	attron(A_BOLD);
	mvprintw(currentY, logX, "%*s", LOG_WIDTH, " ");
	mvprintw(currentY, headerX, "== ");
	attron(COLOR_PAIR(3));
	printw("LOG");
	attroff(COLOR_PAIR(3));
	printw(" ==");
	attroff(A_BOLD);

	currentY++;
	logHeight--;

	/* ---------- Log Messages ---------- */

	attron(A_DIM);

	/* We want to show at most logHeight messages */
	int messages_to_draw = game_log.length;
	if (messages_to_draw > logHeight)
		messages_to_draw = logHeight;

	for (int i = 0; i < messages_to_draw ; i++) {
		/* newest → oldest */
		int index = (game_log.head - i + LOG_MAX_LINES) % LOG_MAX_LINES;
		const char *message = game_log.messages[index].message;

		int msg_len = strlen(message);
		int current_pos = 0;

		const char *prefix = "> ";
		int prefix_len = 2;
		int available_width = LOG_WIDTH - prefix_len;

		while (current_pos < msg_len) {
			if (currentY >= mapStartY + LOG_MAX_LINES)
				goto end_log_drawing;

			int segment_len = msg_len - current_pos;
			if (segment_len > available_width)
				segment_len = available_width;

			char segment[LOG_WIDTH + 1];
			memcpy(segment, message + current_pos, segment_len);
			segment[segment_len] = '\0';

			mvprintw(
					currentY,
					logX,
					"%s%-*s",
					prefix,
					LOG_WIDTH - prefix_len,
					segment
					);

			current_pos += segment_len;
			currentY++;

			/* wrapped lines get indentation */
			prefix = "  ";
			prefix_len = 2;
			available_width = LOG_WIDTH - prefix_len;
		}
	}

end_log_drawing:
	attroff(A_DIM);

}

void DrawBar(int y, int x, float current, float max, int width, char* lable) {
	float percent = current / max;
	int filled = (int)(percent * width);

	move(y, x);
	printw("%s ", lable);

	// Draw the brackets and bar
	printw("[");
	for (int i = 0; i < width; i++) {
		if (i < filled) {
			attron(COLOR_PAIR(2)); // Assuming Pair 2 is Green
			addch('=');
			attroff(COLOR_PAIR(2));
		} else {
			attron(COLOR_PAIR(1)); // Assuming Pair 1 is Red
			addch('-');
			attroff(COLOR_PAIR(1));
		}
	}
	printw("] %0.1f/%0.1f", current, max);
}

void RenderUI(Game_t* game, int startY, int startX, int mapHeight, int mapWidth) {
    // Calculate UI starting position (Right side of map + padding)
    int uiX = startX + mapWidth + 4; 
    int uiY = startY;

    // 1. Draw a Vertical Separator
    // We draw a line from the top of the map to the bottom
    for(int i = 0; i < mapHeight; i++) {
        mvaddch(startY + i, startX + mapWidth + 2, '|');
    }

    // 2. Dungeon Info
    attron(A_BOLD);
    mvprintw(uiY++, uiX, "DUNGEON FLOOR: %d", game->level);
    attroff(A_BOLD);
    
    uiY++; // Spacer

    // 3. Player Stats
    mvprintw(uiY++, uiX, "PLAYER LVL: %d", game->player.level);
    
    // Draw Health Bar (Length 10)
	char lable[5] = "HP: ";
    DrawBar(uiY++, uiX, game->player.stats.hp, game->player.stats.max_hp, 10, lable);
    
    // Draw XP Bar (Length 10)
	lable[0] = 'X';
    DrawBar(uiY++, uiX, game->player.stats.xp, game->player.stats.xp_threshold, 10, lable);
    
    uiY++; // Spacer

    // 4. Combat Stats
    mvprintw(uiY++, uiX, "Attack:  %0.1f", game->player.stats.attack);
    mvprintw(uiY++, uiX, "Defense: %0.1f", game->player.stats.defense);

    uiY++; // Spacer
    
    // 5. Economy / Goal
    attron(COLOR_PAIR(CP_YELLOW)); 
    mvprintw(uiY++, uiX, "GOLD:    %d", game->player.stats.gold);
    attroff(COLOR_PAIR(CP_YELLOW));

    mvprintw(uiY++, uiX, "QUOTA:   %d", game->quota);

	if(game->sub_state == FIGHT){ //Stats for the monster
		
		uiY++;
		mvprintw(uiY++, uiX, "==============");
		mvprintw(uiY++, uiX, game->active_enemy.name);
		lable[0] = 'H';
		DrawBar(uiY++, uiX, game->active_enemy.stats.hp, game->active_enemy.stats.max_hp, 10, lable);
		uiY++;
		mvprintw(uiY++, uiX, "Attack:  %0.1f", game->active_enemy.stats.atk*(1.0+(.5)*(game->level-1.0)));
		mvprintw(uiY++, uiX, "Defense: %0.1f", game->active_enemy.stats.defense);

	}
}


void RenderRunningGame(Game_t* game) {

    const int rows = ROOM_SIZE;
    const int cols = ROOM_SIZE;
    const int cellWidth = 2;

    int termRows, termCols;
    getmaxyx(stdscr, termRows, termCols);

    int displayHeight = rows;
    int displayWidth = cols * cellWidth;

    // Centering logic
    int startY = (termRows - displayHeight) / 2;
    int startX = (termCols - displayWidth) / 2;
    
    // Shift slightly left if possible to make room for UI on the right
    startX -= 10;

    if (startY < 0) startY = 0;
    if (startX < 0) startX = 0;


if (game->sub_state == OVERWORLD) {
	attroff(COLOR_PAIR(3) | A_BOLD);
	// --- DRAW MAP ---
    for (int r = 0; r < rows; r++) {
        move(startY + r, startX);

        for (int c = 0; c < cols; c++) {
            tile_t tile = game->board.grid[r][c];

            switch (tile) {
                case WALL:
                    attron(COLOR_PAIR(CP_BLUE));
                    printw("[]");
                    attroff(COLOR_PAIR(CP_BLUE));
                    break;

                case EMPTY:
                    attron(COLOR_PAIR(CP_EMPTY));
                    printw(". ");
                    attroff(COLOR_PAIR(CP_EMPTY));
                    break;

                case ENTRY:
					attron(A_BOLD);
                    attron(COLOR_PAIR(CP_GREEN));
                    printw("# ");
                    attroff(COLOR_PAIR(CP_GREEN));
					attroff(A_BOLD);
                    break;

                case EXIT:
                    attron(COLOR_PAIR(CP_RED));
                    printw("# ");
                    attroff(COLOR_PAIR(CP_RED));
                    break;

                case TRAP:
                    attron(COLOR_PAIR(CP_EMPTY));
                    printw(". ");
                    attroff(COLOR_PAIR(CP_EMPTY));
                    break;

                case COIN:
                    attron(COLOR_PAIR(CP_YELLOW));
                    printw("* ");
                    attroff(COLOR_PAIR(CP_YELLOW));
                    break;

                case TREASURE:
                    attron(COLOR_PAIR(CP_YELLOW));
                    printw("T ");
                    attroff(COLOR_PAIR(CP_YELLOW));
                    break;

                case WELL:
					attron(A_BOLD);
                    attron(COLOR_PAIR(4));
                    printw("W ");
                    attroff(COLOR_PAIR(4));
					attroff(A_BOLD);
                    break;
            }
        }
    }
	// --- DRAW PLAYER ---
	int playerScreenY = startY + game->player.position.y;
	int playerScreenX = startX + (game->player.position.x * cellWidth);

	move(playerScreenY, playerScreenX);
	attron(A_BOLD);
	printw("@ ");
	attroff(A_BOLD);

}
	
else {
	RenderScenes(game);
}

	// --- DRAW COMMAND INSTRUCTIONS (NEW) ---
    // Calculate the position: one row below the map's last row (startY + rows)
    int commandsY = startY + rows + 1; 
    int commandsX = startX; // Start at the same X position as the map
	switch (game->sub_state) {
		case OVERWORLD:{
		   const char* instructions = "s) save game | l) load save | q) go to menu | arrow keys to move ";
		   attron(A_BOLD); 
		   mvprintw(commandsY, commandsX, instructions);
		   attroff(A_BOLD);
		   }
		   break;
		case WELL_AREA:{
		   const char* instructions = "d) Drop a coin (10 GOLD) | l) leave | q) go to main menu";
		   attron(A_BOLD); 
		   mvprintw(commandsY, commandsX, instructions);
		   attroff(A_BOLD);
		   }
		   break;
		case FIGHT:{
		   const char* instructions = "a) Attack | r) run away | q) go to main menu";
		   attron(A_BOLD); 
		   mvprintw(commandsY, commandsX, instructions);
		   attroff(A_BOLD);
		   }
		   break;
		case WIN:{
		   const char* instructions = "q) go to menu ";
		   attron(A_BOLD); 
		   mvprintw(commandsY, commandsX, instructions);
		   attroff(A_BOLD);
		   }
		   break;
		case DEAD:{
		   const char* instructions = "q) go to menu";
		   attron(A_BOLD); 
		   mvprintw(commandsY, commandsX, instructions);
		   attroff(A_BOLD);
		   }
		   break;
	}
    
    // --- DRAW UI SIDEBAR ---
    RenderUI(game, startY, startX, displayHeight, displayWidth);

	// --- DRAW LOG (NEW SECTION) ---
    
    // Log starts 2 characters to the left of the map's calculated startX
    // and is constrained by the map's height.
    int logX = startX - LOG_WIDTH - 2; 
    
    // If we've shifted the map all the way to the left, we need to check bounds
    if (logX < 0) logX = 0; 
    
    // The log will use the map's startY and height
    DrawLog(startY, logX, displayHeight);

	// DRAW BANNER

	const int max_width_in_cells = ROOM_SIZE; 

	getmaxyx(stdscr, termRows, termCols); // Get current terminal size

	const char* banner_drawing[] = { 
#include "banner.txt"
	};
	const int banner_height = sizeof(banner_drawing) / sizeof(banner_drawing[0]);


	int banner_display_width = max_width_in_cells * cellWidth; 
	startY = 0;
	startX = (termCols - banner_display_width - 10) / 2;

	if (startX < 0) startX = 0;
	// --- 3. Draw Banner ---
	// Loop through each line (row) of the banner array
	attron(COLOR_PAIR(CP_RED) | A_BOLD);
	for (int r = 0; r < banner_height; r++) {
		move(startY + r, startX);
		printw("%s", banner_drawing[r]); 
	}
	attroff(COLOR_PAIR(CP_RED) | A_BOLD);


}

void RenderMenu() {
    int termRows, termCols;
    getmaxyx(stdscr, termRows, termCols);
    
    // --- Detailed ASCII ART Title ---
    const char* title[] = {
		#include "title.txt"
     };
    
    int title_lines = sizeof(title) / sizeof(title[0]);
    // The widest line is 99 characters long. Using this exact width is key for centering.
    const int max_title_width = 99; 

    // Calculate starting coordinates for centering
    int start_y = (termRows / 2) - (title_lines / 2) - 3; 
    int start_x = (termCols / 2) - (max_title_width / 2);

    // Apply color and draw the title
    attron(COLOR_PAIR(CP_RED) | A_BOLD);
    for (int i = 0; i < title_lines; ++i) {
        mvprintw(start_y + i, start_x, "%s", title[i]);
    }
    attroff(COLOR_PAIR(CP_RED) | A_BOLD);
    
    // --- Menu Options ---
    const char* options[] = {
        "1. New Game",
        "2. Load Game",
        "3. Quit"
    };
    int num_options = 3;
    
    // We want the menu options to be centered relative to the whole art block.
    // The menu text "New Game" is 9 characters long.
    // The formatting " > New Game < " is 15 characters long.
    const int menu_text_width = 15;
    
    // Recalculate menu_x: Center the menu text (width 15) within the total art width (99)
    int menu_y = start_y + title_lines + 1; 
    int menu_x = start_x + (max_title_width / 2) - (menu_text_width / 2); 

    mvprintw(menu_y++, menu_x, "-----------------");

    for (int i = 0; i < num_options; ++i) {
        int current_y = menu_y + i;
        mvprintw(current_y, menu_x, "  %s  ", options[i]);
    }
    
    menu_y = menu_y + num_options;
    mvprintw(menu_y++, menu_x, "-----------------");
}



void RenderGame(void* game_ptr) {
	erase();
	Game_t* game = (Game_t *)game_ptr;
	switch(game->state){
		case MENU:
			RenderMenu();
			break;
		case RUNNING:
			RenderRunningGame(game);
			break;
		case QUIT_GAME:
			break;
	}
	refresh();
}
