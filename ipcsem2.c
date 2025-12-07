#include <stdio.h>
#include <sys/sem.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <unistd.h>
#include <string.h>

#define STR_SIZE 255

int main(int argc, char** argv)
{
    if (argc != 2) {
        printf("argc error\n");
        return 1;
    }

    char* filename = argv[1];

    int proj_id = 0b10101010;

    key_t key = ftok("somefile", proj_id);
    int shmid = shmget(key, STR_SIZE * 3, IPC_CREAT | 0666);
    char *data = (char *)shmat(shmid, NULL, 0);

    int semid = semget(key, 3, IPC_CREAT | 0666);

    // semctl(semid, 0, SETVAL, 0);
    // semctl(semid, 1, SETVAL, 0);
    // semctl(semid, 2, SETVAL, 0);

    FILE* file = fopen(filename, "r");
    pid_t p = getpid();
    char buffer[STR_SIZE];
    char buffer2[STR_SIZE];
    int i = 0;

    while(1) {
        if (semctl(semid, 1, GETVAL) == 1) {
            strcpy(buffer, data);
            ++i;
            sprintf(buffer2, "Line -%d | pid -%d  :%s", i, p, buffer);
            strcat(data, buffer2);
            semctl(semid, 1, SETVAL, 0);
            semctl(semid, 2, SETVAL, 1);
        }

        if (semctl(semid, 1, GETVAL) == 2) {
            break;
        }

    }

    shmdt(shmid);
    shmctl(shmid, IPC_RMID, NULL);

    semctl(semid, IPC_RMID, NULL);

    fclose(file);
    return 0;
}
