#include "game_backup.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>

// variáveis globais
board_t backup;
bool backup_exists = false;
pid_t backup_pid = -1;

void save_game(board_t *game_board) {
    if (backup_exists) return;

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return;
    }

    if (pid == 0) {
        // processo filho copia o estado
        copy_board_state(&backup, game_board);
        backup_exists = true;
        exit(0);
    } else {
        // processo pai espera o filho terminar
        backup_pid = pid;
        int status;
        waitpid(pid, &status, 0);
    }
}
void restore_game(board_t *game_board) {
    if (!backup_exists) return;
    restore_board_state(game_board, &backup);
    free_backup_memory();
}

void free_backup_memory(void) {
    if (backup_exists) {
        free_board_backup(&backup);
        backup_exists = false;
        backup_pid = -1;
    }
}