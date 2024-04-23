#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <stdbool.h>
#include <fcntl.h>

typedef struct {
    sem_t forks[5];
} Forks;

bool running = true;

void end_program(int signal) {
    if (signal == SIGINT) {
        printf("Ending program\n");
        running = false;
    }
}

void philosoph_activity(int id, sem_t *forks) {
    while (running) {
        printf("Philosopher %d is thinking.\n", id + 1);
        sleep(rand() % 3 + 1);

        printf("Philosopher %d wants to eat.\n", id + 1);
        sem_wait(&forks[id]);
        sem_wait(&forks[(id + 1) % 5]);

        printf("Philosopher %d is eating.\n", id + 1);
        sleep(rand() % 3 + 1);

        sem_post(&forks[id]);
        sem_post(&forks[(id + 1) % 5]);

        printf("Philosopher %d ends eating.\n", id + 1);
    }
}

int main() {
    srand(NULL);
    signal(SIGINT, end_program);

    Forks *forks = mmap(NULL, sizeof(Forks), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (forks == MAP_FAILED) {
        perror("failure nmap");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < 5; ++i) {
        if (sem_init(&forks->forks[i], 1, 1) == -1) {
            perror("failure sem_init");
            exit(EXIT_FAILURE);
        }
    }

    pid_t pid;
    for (int i = 0; i < 5; ++i) {
        pid = fork();
        if (pid < 0) {
            perror("failure fork");
            exit(EXIT_FAILURE);
        }
        if (pid == 0) {
            philosoph_activity(i, forks->forks);
            exit(EXIT_SUCCESS);
        }
    }

    while (running);

    for (int i = 0; i < 5; ++i) {
        sem_destroy(&forks->forks[i]);
    }
    munmap(forks, sizeof(Forks));
}
