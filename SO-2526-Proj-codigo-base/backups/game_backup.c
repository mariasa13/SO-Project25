#include "game_backup.h"
#include "board.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>

// Variáveis globais
board_t backup;
bool backup_exists = false;
pid_t backup_pid = -1;

void save_game(board_t *game_board) {
    if (backup_exists) return; // já existe backup

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return;
    }

    if (pid == 0) { // processo filho
        // cria backup
        copy_board_state(&backup, game_board);
        backup_exists = true;
        exit(0); // filho termina
    } else { // processo pai espera que o filho termine
        backup_pid = pid;
        int status;
        waitpid(pid, &status, 0);
    }
}

void restore_game(void) {
    if (!backup_exists) return;
    // restaura estado do backup
    restore_board_state(&backup, &backup); // usa backup como fonte
    free_backup_memory(); // limpa para permitir novo save
}

void free_backup_memory(void) {
    if (backup_exists) {
        free_board_backup(&backup);
        backup_exists = false;
        backup_pid = -1;
    }
}
