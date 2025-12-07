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
    if (argc != 3) {
        printf("argc error\n");
        return 1;
    }

    char* filename = argv[1];
    char* resname = argv[2];

    int proj_id = 0b10101010;

    key_t key = ftok("somefile", proj_id);
    int shmid = shmget(key, STR_SIZE * 3, IPC_CREAT | 0666);
    char *data = (char *)shmat(shmid, NULL, 0);

    int msgqid = msgget(key, IPC_CREAT | 0666);
    struct message msg, msgin;
    msg.type = 1;
    strcpy(msg.str, "read");

    msgsnd(msgqid, &msg, sizeof(msg.str), IPC_NOWAIT);

    printf("first message sent\n");
    FILE* file = fopen(filename, "r");
    FILE* resfile = fopen(resname, "w");
    pid_t p = getpid();
    char buffer[STR_SIZE];
    char buffer2[STR_SIZE];
    int i = 0;

    while(1) {
        msgrcv(msgqid, &msgin, sizeof(msgin.str), 1, 0);
        printf("1 msg received %ld , %s\n", msgin.type, msgin.str);
        if (strcmp(msgin.str, "read") == 0){
            if (NULL == fgets(buffer, sizeof(buffer) - 1, file) ) {
                msg.type = 2;
                strcpy(msg.str, "end");
                msgsnd(msgqid, &msg, sizeof(msg.str), 0);
                msg.type = 3;
                msgsnd(msgqid, &msg, sizeof(msg.str), 0);
                printf("2 msg sent %ld , %s\n", msg.type, msg.str);
                break;
            }
            ++i;
            sprintf(buffer2, "%ld pid: %d - %s", i, p, buffer);
            strcpy(data, buffer2);

            msg.type = 2;
            strcpy(msg.str, "write");
            msgsnd(msgqid, &msg, sizeof(msg.str), 0);
            printf("3 msg sent %ld , %s\n", msg.type, msg.str);
        }

        if (strcmp(msgin.str, "write") == 0) {
            fprintf(resfile, "%s", data);
            msg.type = 1;
            strcpy(msg.str, "read");
            msgsnd(msgqid, &msg, sizeof(msg.str), 0);
            printf("4 msg snt %ld , %s\n", msg.type, msg.str);
            strcpy(data, "");
        }
    }

    shmdt(shmid);
    shmctl(shmid, IPC_RMID, NULL);

    msgctl(msgqid, IPC_RMID, NULL);

    fclose(file);
    fclose(resfile);
    return 0;
}
