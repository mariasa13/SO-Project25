#include "file_loader.h"
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdio.h>

// Helper function to check if a string ends with a given suffix
int ends_with(const char* str, const char* suffix) {
    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);
    if (str_len < suffix_len) return 0;
    return strcmp(str + str_len - suffix_len, suffix) == 0;
}

int init_level_manager(level_manager_t* manager, const char* directory) {
    DIR* dir = opendir(directory);
    if (!dir) {
        debug("Error: Could not open directory %s\n", directory);
        return -1;
    }

    strncpy(manager->directory, directory, MAX_FILENAME - 1);
    manager->n_levels = 0;
    manager->current_level = 0;

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL && manager->n_levels < MAX_LEVELS) {
        if (ends_with(entry->d_name, ".lvl")) {
            strncpy(manager->level_files[manager->n_levels], entry->d_name, MAX_FILENAME - 1);
            manager->n_levels++;
        }
    }
    closedir(dir);

    if (manager->n_levels == 0) {
        debug("Error: No .lvl files found in directory\n");
        return -1;
    }

    debug("Found %d level files:\n", manager->n_levels);
    for (int i = 0; i < manager->n_levels; i++) {
        debug("  - %s\n", manager->level_files[i]);
    }

    return 0;
}

int next_level(level_manager_t* manager) {
    manager->current_level++;
    if (manager->current_level >= manager->n_levels) {
        return 0; // No more levels
    }
    return 1; // More levels available
}

// Helper function to read a word from file descriptor
int read_word(int fd, char* buffer, int max_size) {
    int i = 0;
    char c;
    
    // Skip whitespace and comments
    while (read(fd, &c, 1) == 1) {
        if (c == '#') {
            // Skip comment line
            while (read(fd, &c, 1) == 1 && c != '\n');
            continue;
        }
        if (!isspace(c)) break;
    }
    
    // Read word
    do {
        buffer[i++] = c;
        if (i >= max_size - 1) break;
    } while (read(fd, &c, 1) == 1 && !isspace(c));
    
    buffer[i] = '\0';
    return i;
}

int read_behavior_file(const char* filepath, command_t* moves, int* passo, int* pos_x, int* pos_y) {
    int fd = open(filepath, O_RDONLY);
    if (fd < 0) {
        debug("Error: Could not open behavior file %s\n", filepath);
        return -1;
    }

    char word[256];
    int n_moves = 0;
    *passo = 0;

    while (read_word(fd, word, sizeof(word)) > 0) {
        if (strcmp(word, "PASSO") == 0) {
            read_word(fd, word, sizeof(word));
            *passo = atoi(word);
        } else if (strcmp(word, "POS") == 0) {
            read_word(fd, word, sizeof(word));
            *pos_y = atoi(word);
            read_word(fd, word, sizeof(word));
            *pos_x = atoi(word);
        } else if (strlen(word) == 1 && n_moves < MAX_MOVES) {
            // Single character command
            char cmd = word[0];
            if (cmd == 'A' || cmd == 'D' || cmd == 'W' || cmd == 'S' || cmd == 'R' || cmd == 'C') {
                moves[n_moves].command = cmd;
                moves[n_moves].turns = 1;
                moves[n_moves].turns_left = 1;
                n_moves++;
            } else if (cmd == 'T') {
                // T command needs a number
                read_word(fd, word, sizeof(word));
                int turns = atoi(word);
                moves[n_moves].command = 'T';
                moves[n_moves].turns = turns;
                moves[n_moves].turns_left = turns;
                n_moves++;
            }
        }
    }

    close(fd);
    return n_moves;
}

int load_level_from_file(board_t* board, level_manager_t* manager, int accumulated_points) {
    //memset(board, 0, sizeof(board_t));
    if (manager->current_level >= manager->n_levels) {
        return -1;
    }

    char filepath[MAX_FILENAME * 2];
    snprintf(filepath, sizeof(filepath), "%s/%s", 
             manager->directory, manager->level_files[manager->current_level]);

    int fd = open(filepath, O_RDONLY);
    if (fd < 0) {
        debug("Error: Could not open level file %s\n", filepath);
        return -1;
    }

    strncpy(board->level_name, manager->level_files[manager->current_level], 255);
    
    char word[256];
    board->pacman_file[0] = '\0';
    board->n_ghosts = 0;
    
    // Read level parameters
    while (read_word(fd, word, sizeof(word)) > 0) {
        if (strcmp(word, "DIM") == 0) {
            read_word(fd, word, sizeof(word));
            board->height = atoi(word);
            read_word(fd, word, sizeof(word));
            board->width = atoi(word);
        } else if (strcmp(word, "TEMPO") == 0) {
            read_word(fd, word, sizeof(word));
            board->tempo = atoi(word);
        } else if (strcmp(word, "PAC") == 0) {
            read_word(fd, board->pacman_file, sizeof(board->pacman_file));
        } else if (strcmp(word, "MON") == 0) {
            // Read monster filenames until we hit a non-filename
            while (read_word(fd, word, sizeof(word)) > 0) {
                if (ends_with(word, ".m")) {
                    strncpy(board->ghosts_files[board->n_ghosts], word, 255);
                    board->n_ghosts++;
                } else {
                    // We've hit the board matrix, break and handle it
                    break;
                }
            }
        }
        
        // Check if we've started reading the board matrix
        if (strlen(word) > 0 && (word[0] == 'X' || word[0] == 'o' || word[0] == '@')) {
            // This is the first line of the board
            break;
        }
    }

    // Allocate board memory
    board->n_pacmans = 1;
    board->board = calloc(board->width * board->height, sizeof(board_pos_t));
    board->pacmans = calloc(board->n_pacmans, sizeof(pacman_t));
    board->ghosts = calloc(board->n_ghosts, sizeof(ghost_t));

    // Read board matrix
    int row = 0;
    char c;
    int col = 0;
    
    // Process the first word we already read
    for (int i = 0; word[i] != '\0' && col < board->width; i++) {
        if (word[i] == 'X') {
            board->board[row * board->width + col].content = 'W';
            board->board[row * board->width + col].has_dot = 0;
        } else if (word[i] == 'o') {
            board->board[row * board->width + col].content = ' ';
            board->board[row * board->width + col].has_dot = 1;
        } else if (word[i] == '@') {
            board->board[row * board->width + col].content = ' ';
            board->board[row * board->width + col].has_portal = 1;
            board->board[row * board->width + col].has_dot = 0;
        }
        col++;
    }
    if (col >= board->width) {
        row++;
        col = 0;
    }

    // Continue reading the rest of the board
    while (read(fd, &c, 1) == 1 && row < board->height) {
        if (c == 'X' || c == 'o' || c == '@') {
            if (c == 'X') {
                board->board[row * board->width + col].content = 'W';
                board->board[row * board->width + col].has_dot = 0;
            } else if (c == 'o') {
                board->board[row * board->width + col].content = ' ';
                board->board[row * board->width + col].has_dot = 1;
            } else if (c == '@') {
                board->board[row * board->width + col].content = ' ';
                board->board[row * board->width + col].has_portal = 1;
                board->board[row * board->width + col].has_dot = 0;
            }
            col++;
            if (col >= board->width) {
                row++;
                col = 0;
            }
        }
    }

    close(fd);

    // Load pacman behavior
    if (strlen(board->pacman_file) > 0) {
        char pacman_path[MAX_FILENAME * 2];
        snprintf(pacman_path, sizeof(pacman_path), "%s/%s", manager->directory, board->pacman_file);
        
        int pos_x, pos_y;
        board->pacmans[0].n_moves = read_behavior_file(pacman_path, board->pacmans[0].moves, 
                                                       &board->pacmans[0].passo, &pos_x, &pos_y);
        board->pacmans[0].pos_x = pos_x;
        board->pacmans[0].pos_y = pos_y;
        board->pacmans[0].current_move = 0;
        board->pacmans[0].waiting = board->pacmans[0].passo;
        board->pacmans[0].alive = 1;
        board->pacmans[0].points = accumulated_points;
        board->board[pos_y * board->width + pos_x].content = 'P';
    } else {
        // Manual control - place at (1,1) by default
        board->pacmans[0].n_moves = 0;
        board->pacmans[0].passo = 0;
        board->pacmans[0].pos_x = 1;
        board->pacmans[0].pos_y = 1;
        board->pacmans[0].current_move = 0;
        board->pacmans[0].waiting = 0;
        board->pacmans[0].alive = 1;
        board->pacmans[0].points = accumulated_points;
        board->board[1 * board->width + 1].content = 'P';
    }

    // Load ghost behaviors
    for (int i = 0; i < board->n_ghosts; i++) {
        char ghost_path[MAX_FILENAME * 2];
        snprintf(ghost_path, sizeof(ghost_path), "%s/%s", manager->directory, board->ghosts_files[i]);
        
        int pos_x, pos_y;
        board->ghosts[i].n_moves = read_behavior_file(ghost_path, board->ghosts[i].moves, 
                                                      &board->ghosts[i].passo, &pos_x, &pos_y);
        board->ghosts[i].pos_x = pos_x;
        board->ghosts[i].pos_y = pos_y;
        board->ghosts[i].current_move = 0;
        board->ghosts[i].waiting = board->ghosts[i].passo;
        board->ghosts[i].charged = 0;
        board->board[pos_y * board->width + pos_x].content = 'M';
    }

    debug("Loaded level: %s (dimensions: %dx%d, tempo: %d)\n", 
          board->level_name, board->width, board->height, board->tempo);

    return 0;
}