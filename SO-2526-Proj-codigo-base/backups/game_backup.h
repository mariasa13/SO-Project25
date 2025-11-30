#ifndef GAME_BACKUP_H
#define GAME_BACKUP_H

#include "board.h"
#include <stdbool.h>
#include <sys/types.h>

// Funções para backup
void save_game(board_t *game_board);
void restore_game(void);
void free_backup_memory(void);

// Variáveis globais do backup
extern bool backup_exists;
extern board_t backup;
extern pid_t backup_pid;

#endif
