#include "board.h"
#include "display.h"
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include "file_loader.h"

#define CONTINUE_PLAY 0
#define NEXT_LEVEL 1
#define QUIT_GAME 2
#define LOAD_BACKUP 3
#define CREATE_BACKUP 4

void screen_refresh(board_t * game_board, int mode) {
    debug("REFRESH\n");
    draw_board(game_board, mode);
    refresh_screen();
    if(game_board->tempo != 0)
        sleep_ms(game_board->tempo);       
}

int play_board(board_t * game_board) {
    pacman_t* pacman = &game_board->pacmans[0];
    command_t* play;
    if (pacman->n_moves == 0) { // if is user input
        command_t c; 
        c.command = get_input();

        if(c.command == '\0')
            return CONTINUE_PLAY;

        c.turns = 1;
        play = &c;
    }
    else { // else if the moves are pre-defined in the file
        // avoid buffer overflow wrapping around with modulo of n_moves
        // this ensures that we always access a valid move for the pacman
        play = &pacman->moves[pacman->current_move%pacman->n_moves];
    }

    debug("KEY %c\n", play->command);

    if (play->command == 'Q') {
        return QUIT_GAME;
    }

    int result = move_pacman(game_board, 0, play);
    if (result == REACHED_PORTAL) {
        // Next level
        return NEXT_LEVEL;
    }

    if(result == DEAD_PACMAN) {
        return QUIT_GAME;
    }
    
    for (int i = 0; i < game_board->n_ghosts; i++) {
        ghost_t* ghost = &game_board->ghosts[i];
        // avoid buffer overflow wrapping around with modulo of n_moves
        // this ensures that we always access a valid move for the ghost
        move_ghost(game_board, i, &ghost->moves[ghost->current_move%ghost->n_moves]);
    }

    if (!game_board->pacmans[0].alive) {
        return QUIT_GAME;
    }      

    return CONTINUE_PLAY;  
}

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Usage: %s <level_directory>\n", argv[0]);
        return 1;
        // TODO receive inputs
    }

    open_debug_file("debug.log");

    const char* level_directory = argv[1];
    level_manager_t level_manager;
    if (init_level_manager(&level_manager, level_directory) == -1) {
        printf("Error: Could not initialize level manager\n");
        close_debug_file();
        return 1;
    }

    // Random seed for any random movements
    srand((unsigned int)time(NULL));

    terminal_init();
    
    int accumulated_points = 0;
    bool end_game = false;
    board_t game_board;

    while (!end_game) {
        if (load_level_from_file(&game_board, &level_manager, accumulated_points) != 0) {
            printf("Error: Could not load level %d\n", level_manager.current_level);
            break;
        }

        bool level_completed = false;

        draw_board(&game_board, DRAW_MENU);
        refresh_screen();

        while(true) {
            int result = play_board(&game_board); 

            if(result == NEXT_LEVEL) {
                accumulated_points = game_board.pacmans[0].points;
                screen_refresh(&game_board, DRAW_WIN);
                sleep_ms(game_board.tempo);
                level_completed = true;
                break;
            }

            if(result == QUIT_GAME) {
                screen_refresh(&game_board, DRAW_GAME_OVER); 
                sleep_ms(game_board.tempo);
                end_game = true;
                break;
            }
    
            screen_refresh(&game_board, DRAW_MENU); 

            accumulated_points = game_board.pacmans[0].points;      
        }
        print_board(&game_board);
        unload_level(&game_board);

        if (!end_game && level_completed) {
            if (next_level(&level_manager) == 0) {
                end_game = true;
            }
        } else {
            end_game = true;
        }
    }    

    terminal_cleanup();

    close_debug_file();

    return 0;
}
