#ifndef FILE_LOADER_H
#define FILE_LOADER_H

#include "board.h"

// Structure to keep track of available level files
typedef struct {
    char directory[MAX_FILENAME];
    char level_files[MAX_LEVELS][MAX_FILENAME];
    int n_levels;
    int current_level;
} level_manager_t;

/*
 * Initializes the level manager by scanning the directory for .lvl files
 * Returns 0 on success, -1 on error
 */
int init_level_manager(level_manager_t* manager, const char* directory);

/*
 * Loads the current level into the board structure
 * Returns 0 on success, -1 on error
 */
int load_level_from_file(board_t* board, level_manager_t* manager, int accumulated_points);

/*
 * Advances to the next level
 * Returns 1 if there are more levels, 0 if this was the last level
 */
int next_level(level_manager_t* manager);

/*
 * Reads a behavior file (.p or .m) and populates the moves array
 * Returns the number of moves read, -1 on error
 */
int read_behavior_file(const char* filepath, command_t* moves, int* passo, int* pos_x, int* pos_y);

#endif