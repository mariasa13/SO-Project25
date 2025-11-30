#ifndef GAME_BACKUP_H
#define GAME_BACKUP_H

#include "board.h"
#include <stdbool.h>
#include <sys/types.h>

void save_game(board_t *game_board);
void restore_game(board_t *game_board);
void free_backup_memory(void);

// Variáveis globais definidas no game_backup.c
extern bool backup_exists;
extern board_t backup;
extern pid_t backup_pid;

#endif
