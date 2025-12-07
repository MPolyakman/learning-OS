#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <string.h>

int main(int argc, char ** argv)
{
    if (argc != 6) {
        printf("argc error here\n");
        return 0;
    }
    
    char* prog1 = argv[1];
    char* param1 = argv[2];
    char* prog2 = argv[3];
    char* param2 = argv[4];
    char* output_file = argv[5];
    
    int pipefd[2];
    pipe(pipefd);

    pid_t pid1 = fork();
    if (pid1 == 0) {
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);
        execlp(prog1, prog1, param1, NULL);
    }

    pid_t pid2 = fork();
    if (pid2 == 0) {
        dup2(pipefd[0], STDIN_FILENO);
        
        int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);

        dup2(fd, STDOUT_FILENO);
        close(fd);
        
        close(pipefd[0]);
        close(pipefd[1]);
        
        execlp(prog2, prog2, param2, NULL);
    }

    close(pipefd[0]);
    close(pipefd[1]);
    
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
    return 0;
}
