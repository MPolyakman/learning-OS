#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <string.h>

int main(int argc, char ** argv)
{
    if (argc != 8) {
        printf("argc error");
        return 0;
    }
    
    char* prog1 = argv[1];
    char* param1 = argv[2];
    char* prog2 = argv[3];
    char* param2 = argv[4];
    char* prog3 = argv[5];
    char* param3 = argv[6];
    char* output_file = argv[7];
    
    int pipe1[2], pipe2[2];    
    
    pipe(pipe1); pipe(pipe2);

    pid_t pid1 = fork();
    if (pid1 == 0) {
        close(pipe1[0]); 
        close(pipe2[0]); 
        close(pipe2[1]);
        
        dup2(pipe1[1], STDOUT_FILENO);
        close(pipe1[1]);
        
        execlp(prog1, prog1, param1, NULL);
    }

    pid_t pid2 = fork();
    if (pid2 == 0) {
        close(pipe1[1]); 
        close(pipe2[0]);
        
        dup2(pipe1[0], STDIN_FILENO);
        close(pipe1[0]);
        dup2(pipe2[1], STDOUT_FILENO);
        close(pipe2[1]);
        
        execlp(prog2, prog2, param2, NULL);
    }

    pid_t pid3 = fork();
    if (pid3 == 0) {
        close(pipe1[0]);
        close(pipe1[1]);
        close(pipe2[1]);
        
        dup2(pipe2[0], STDIN_FILENO);
        close(pipe2[0]);
        
        int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) {
            printf("create file error");
            return 0;
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
        execlp(prog3, prog3, param3, NULL);
    }

    close(pipe1[0]);
    close(pipe1[1]);
    close(pipe2[0]);
    close(pipe2[1]);

    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
    waitpid(pid3, NULL, 0);
    
    return 0;
}