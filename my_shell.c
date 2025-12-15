#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/wait.h>

#define ANSI_TEXT_YELLOW  "\x1b[33m" // цветной шелл
#define ANSI_TEXT_GREEN  "\x1B[32m"
#define ANSI_TEXT_RED     "\x1b[95m"
#define ANSI_TEXT_WHITE   "\x1b[37m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#define BACK_YELLOW  "\x1b[43m" // зыдний фон
#define BACK_GREEN  "\x1B[42m"
#define BACK_RED     "\x1b[45m"

char* const ops[] = {"&&", "&", "||", "|", "(", ")", ">", "<", ">>", ";"};

static bool parsing = true;

int cnt_args(char* str) {
    int cnt = 0;
    int i = 0;
    int len = strlen(str);
    
    while (i < len) {
        while (i < len && str[i] == ' ') i++;
        if (i >= len) break;
        
        cnt++;
        
        while (i < len && str[i] != ' ') i++;
    }
    return cnt;
}

char** parse_input(char* str) 
{
    int num_of_cmd = cnt_args(str);
    char** line = malloc(num_of_cmd * sizeof(char*));
    int step;
    char token[100];

    for (int i = 0; i < num_of_cmd; i++)
    {
        sscanf(str, "%99s%n", token, &step);
        line[i] = malloc((strlen(token) + 1) * sizeof(char));
        strcpy(line[i], token);
        str += step;
    }
    return line;
}

bool is_logic_op(char* cmd) {
    return (strcmp(cmd, "&&") == 0 || strcmp(cmd, "||") == 0 || strcmp(cmd, "&") == 0 || strcmp(cmd, ";") == 0);
}

bool is_quiet(char* cmd) {
    return strcmp(cmd, "&") == 0 ? true : false ;
}

int cnt_char(char* str, char c)
{
    int ans = 0;
    while(*str)
    {
        if (*str == c)
            ++ans;
        ++str;
    }
    return ans;
}

int cnt_words(char** str, char* word, int n)
{
    int ans = 0;
    for (int i = 0; i < n ; i++)
    {
        if (strcmp((str[i]), word) == 0)
            ++ans;
    }
    return ans;
}

// ##################
// CMD EXECUTION ####
// ##################

void execute(int argc, char** argv)
{
    //printf("%d\n", argc);
    for (int i = 0; i < argc; i++) {
        if ( (strcmp(argv[i], ">") == 0 || strcmp(argv[i], "<") == 0) || strcmp(argv[i], ">>") == 0) {
            if (i != argc - 2)
            {
                printf("error in using <>\n");
                exit(1);   
            }
            else {
                if (strcmp(argv[i], ">") == 0) {
                    int fd = open(argv[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    dup2(fd, STDOUT_FILENO);
                    close(fd);
                }
                else if (strcmp(argv[i], ">>") == 0) {
                    int fd = open(argv[i + 1], O_WRONLY | O_CREAT| O_APPEND, 0644);
                    dup2(fd, STDOUT_FILENO);
                    close(fd);
                }
                else if (strcmp(argv[i], "<") == 0 ) {
                    int fd = open(argv[i + 1], O_RDONLY, 0644);
                    dup2(fd, STDIN_FILENO);
                    close(fd);
                }
                argv[argc - 2] = NULL; argv[argc - 1] = NULL; // >< не флаги и не аргументы
                break;
            }
        }
    }
    // for (int i = 0; i < argc - 2; i++) printf("%s ", argv[i]); printf("\n");

    execvp(argv[0], argv);
    printf("error executing %s\n", argv[0]);
    exit(1);
}

// ##################
// PIPELINE PARSER###
// ##################
int parse_asm(char** line, int n, bool in_background) 
{
    if (n == 0) return 0;
    pid_t p;

    char** argv = malloc(sizeof(char*) * (n + 1)); 
    int argc = 0;
    int asm_length = cnt_words(line, "|", n) + 1; printf("asm length = %d\n", asm_length);

    int pipes[asm_length - 1][2]; // i процесс пишет в i , i + 1 читает из i - 1
    for (int i = 0;  i < asm_length - 1; i++) pipe(pipes[i]);

    printf("---Pipeline Start---\n");

    int cmd_num = 0;
    pid_t pids[asm_length];
    for (int i = 0; i < n; i++)
    {
        if (strcmp(line[i], "|") == 0)
        {
            argv[argc] = NULL; // конец аргументов для execvp
            
            printf("[Process]: ");
            for(int k = 0; k < argc; k++) printf("%s ", argv[k]);
            printf("\n");

            if (!(pids[cmd_num] = fork()) ) {
                if (cmd_num != 0){
                    if (dup2(pipes[cmd_num - 1][0], STDIN_FILENO) == -1)
                        exit(1);
                    }
                // close(pipes[cmd_num - 1][0]);
                if (cmd_num != asm_length - 1){
                    if (dup2(pipes[cmd_num][1], STDOUT_FILENO) == -1)
                        exit(1);
                    }
                for (int j = 0; j < asm_length - 1; j++) { // закрыть чужие пайпы
                    if (j != cmd_num && j != cmd_num - 1)
                        {close(pipes[j][0]); close(pipes[j][1]);}
                }

                if (cmd_num != asm_length - 1)
                    close(pipes[cmd_num][0]); 
                
                if (cmd_num != 0)
                    close(pipes[cmd_num - 1][1]);

                // close(pipes[cmd_num][1]);
                // for (int j = 0; j < asm_length; j++) {
                //     close(pipes[j][0]); close(pipes[j][1]);
                // }
                execute(argc, argv);
            }
            
            printf("   |\n");
            
            argc = 0;
            ++cmd_num;
        }
        else 
        {
            argv[argc++] = line[i];
        }
    }

    

    if (argc > 0) {
        argv[argc] = NULL;
        printf("[Process]: ");
        for(int k=0; k<argc; k++) printf("%s ", argv[k]);
        printf("\n");

        if (!(pids[cmd_num] = fork())) {
            if (asm_length != 1){
                dup2(pipes[asm_length - 2][0], STDIN_FILENO);
            }
            for (int i = 0; i < asm_length - 1; i++) {
                close(pipes[i][0]); close(pipes[i][1]);
            }

            execute(argc, argv);
        }
    }

    int err;
    for (int i = 0; i < asm_length - 1; i++) {close(pipes[i][0]); close(pipes[i][1]);}
    for (int i = 0; i < asm_length; i++) waitpid(pids[i], &err, 0);

    printf("---Pipeline End with status %d---\n", err);

    free(argv);

    return err;
}

int start_asm(char** pipeline_buf, int buf_len, bool background, int status, int regime) {
    if ( regime == 0) {
        status = parse_asm(pipeline_buf, buf_len, background);
    }
    else if ( regime == 1 && !status) {
        status = parse_asm(pipeline_buf, buf_len, background);   
    }
    else if ( regime == 2 && status) {
        status = parse_asm(pipeline_buf, buf_len, background);
    }
    else if ( regime == 3) {
        status = parse_asm(pipeline_buf, buf_len, background);
    }
    return status;
}

// ################
// GROUP PARSER ###
// ################
int parse_group(char** input, int n, int recursion_depth)
{

    char** pipeline_buf = malloc(sizeof(char*) * n);
    int buf_len = 0;
    int status = 0;
    int future_regime = 0;

    bool background = false;
    int regime = 0; // 0 - ; , 1 - && , 2 - || , 3 - &
    pid_t p;


    int i = 0;
    while (i < n)
    {
        if (strcmp(input[i], "(") == 0)
        {
            if (buf_len > 0) {
                status = start_asm(pipeline_buf, buf_len, false, status, regime);
                buf_len = 0;
                regime = 0;
            }

            int par_cnt = 1;
            int j = i + 1;
            while (j < n) {
                if (strcmp(input[j], "(") == 0) par_cnt++;
                if (strcmp(input[j], ")") == 0) par_cnt--;
                if (par_cnt == 0) break;
                j++;
            }

            if ((regime == 1 && status) || (regime == 2 && !status)) {
                i = j + 1;
                continue;
            }

            printf(ANSI_TEXT_RED "\n Subshell Recursion\n" ANSI_COLOR_RESET);
            // начало: i + 1 , j - (i + 1) - длинна

            bool is_subshell_bg = (j + 1 < n && strcmp(input[j + 1], "&") == 0);
            if (is_subshell_bg) printf("subshell is in bg...\n");
            if ( (p = fork()) == 0) {
                if (is_subshell_bg) {
                    pid_t p1 = fork();
                    if (p1) {
                        exit(0);
                    }
                    setpgid(0,0);
                    int fd_null = open("/dev/null", O_WRONLY);
                    dup2(fd_null, STDOUT_FILENO);
                    dup2(fd_null, STDIN_FILENO);
                    dup2(fd_null, STDERR_FILENO);
                    close(fd_null);
                }
                status = parse_group(input + i + 1, j - (i + 1), ++recursion_depth);
                // printf("actualy %d\n", status);
                exit(13);

            }
            waitpid(p, &status, 0);
            printf("subshell ended with: %d\n", status);


            printf( ANSI_TEXT_RED " End of the recursion\n\n" ANSI_COLOR_RESET);

            i = j + 1; // пропускаем скобки
            continue;
        }
        else if ( strcmp(input[i], ")") == 0) {
            printf("Error !!! mismatch parentesis\n");
            break;
        }

        if (is_logic_op(input[i]))
        {
            background = false;
            if ( strcmp(input[i], "&&") == 0) {
                // if (!status) {
                    future_regime = 1;
                //}
            }
            else if ( strcmp(input[i], "||") == 0) {
                // if (status) {
                    future_regime = 2;
                // }
            }
            else if ( strcmp(input[i], "&") == 0) {
                background = true;
                future_regime = 3;
            }
            else if ( strcmp(input[i], ";") == 0) {
                future_regime = 0;
            }

            printf("regime = %d\n", regime);
            
            if (background) {
                pid_t p2;
                if ( (p2 = fork()) == 0) {
                    if (fork() == 0) {
                        setpgid(0,0);
                        int fd_null = open("/dev/null", O_WRONLY);
                        dup2(fd_null, STDOUT_FILENO);
                        dup2(fd_null, STDIN_FILENO);
                        dup2(fd_null, STDERR_FILENO);
                        close(fd_null);
                        status = start_asm(pipeline_buf, buf_len, background, status, regime);
                        exit(0);
                    }
                    else {
                        exit(0);
                    }
                }
                status = 0;
            }
            else {
                status = start_asm(pipeline_buf, buf_len, background, status, regime);
            }

            regime = future_regime;            

            buf_len = 0;

            printf("future regime = %d\n", future_regime);
            printf("\n[Op]: %s \n\n", input[i]);
            
            i++;
            continue;
        }

        // ecли | или команда
        pipeline_buf[buf_len++] = input[i];
        i++;
    }

    // последняя команда
    if (buf_len > 0) {
        status = start_asm(pipeline_buf, buf_len, false, status, regime);
    }
    
    free(pipeline_buf);

    return status;
    // if (recursion_depth != 0) {
    //     exit(status);
    // }
}

bool is_op(char* str)
{
    for (int i = 0; i < 10; i++)
    {
        if (strcmp(ops[i], str) == 0)
        {
            return true;
        }
    }
    return false;
}

char* adjust_str(char* s)
{
    char* newline = malloc(sizeof(char) * 512);

    int j = 0;
    for (int i = 0; i < strlen(s); i++)
    {

        if ( (s[i] == ';' || s[i] == '|' || s[i] == '&' || s[i] == '>' || s[i] == '<'
        || s[i] == '(' || s[i] == ')')
            /*&& (s[i] != s[i + 1]) && (i != 0 && (s[i] != s[i - 1] ) )*/ )
        {
            newline[j++] = ' ';
            newline[j++] = s[i];
            if (s[i] == s[i + 1] && s[i] != '(' && s[i] != ')' )
            {
                newline[j++] = s[i+1];
                i++;
            }
            newline[j++] = ' ';
        }
        else {
            newline[j++] = s[i];
        }
    }
    newline[j] = '\0';
    free(s);
    return newline;
}

int main(int argc, char** argv)
{
    char* str;
    char** line;
    int n;
    

    while (parsing)
    {
        str = malloc(512);
        printf(ANSI_TEXT_WHITE BACK_YELLOW  "shell> " ANSI_COLOR_RESET);
        if (!fgets(str, 511, stdin)) return 0;
        
        str[strcspn(str, "\n")] = 0;

        str = adjust_str(str);
        printf("adj_str: %s\n", str);

        n = cnt_args(str);
        line = parse_input(str);


        for (int i = 0; i < n; i ++) // если есть команда exit сразу выходим
        {
            if ( strcmp(line[i], "exit") == 0) {
                parsing = false;

                free(str);
                for (int i = 0; i < n; i++) free(line[i]);
                free(line);
                exit(0);
                }
        }

        parse_group(line, n, 0);

        free(str);
        for (int i = 0; i < n; i++) free(line[i]);
        free(line);
    }

    return 0;
}