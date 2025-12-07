#include <stdio.h>
#include <sys/sem.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <unistd.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/stat.h>

#define STR_SIZE 255

struct message {
    long type;
    char str[STR_SIZE];
};

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

    int msgqid = msgget(key, IPC_CREAT | 0666);
    struct message msg, msgin;

    // semctl(semid, 0, SETVAL, 0);
    // semctl(semid, 1, SETVAL, 0);
    // semctl(semid, 2, SETVAL, 0);

    FILE* file = fopen(filename, "r");
    pid_t p = getpid();
    char buffer[STR_SIZE];
    char buffer2[STR_SIZE];
    int i = 0;

    while(1) {
        msgrcv(msgqid, &msgin, sizeof(msgin.str), 3, 0);
        printf("message received %d %s\n", msgin.type, msgin.str);
        if (strcmp(msgin.str, "write") == 0) {
            strcpy(buffer, strchr(data, '\n') + 1);
            buffer[strlen(buffer) - 1] = '\0';
            ++i;
            sprintf(buffer2, "%s pid: %d\n",buffer, p);
            strcat(data, buffer2);

            msg.type = 1;
            strcpy(msg.str, "write");
            msgsnd(msgqid, &msg, sizeof(msg.str), 0);
            printf("message sent %d %s\n", msg.type, msg.str);
        }

        if (strcmp(msgin.str, "end") == 0) {
            break;
        }

    }

    shmdt(shmid);
    shmctl(shmid, IPC_RMID, NULL);

    msgctl(msgqid, IPC_RMID, NULL);

    fclose(file);
    return 0;
}
