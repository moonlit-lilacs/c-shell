#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <limits.h>


int rpe_cd(char** args);
int rpe_help(char** args);
int rpe_exit(char** args);
char* read_line();
char** separate_args_from_line();
void rpe_loop();
int execute_args();


int main(int argc, char **argv){


    rpe_loop();


    return 0;
}


void rpe_loop(){

    int status = 1;
    char* line;
    char** args;
    do {
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) != NULL){
            printf("[%s] ", cwd);
        }
        else{
            perror("getcwd() error");
        }
        printf("> ");
        line = read_line();
        args = separate_args_from_line(line);
        status = execute_args(args);
        //printf("\n");
    } while(status);

    free(line);
    free(args);
}
#define LINE_BUFFER 1024
char* read_line(){
    int buffer_size = LINE_BUFFER;
    int position = 0;
    char* buffer = malloc(sizeof(char)*buffer_size);
    int c;

    while(1){
        c = getchar();
        if(c == '\n' || c == EOF){
            buffer[position] = '\0';
            return buffer;
        }
        else{
            buffer[position] = c;

        }
        position++;
        if (position >= buffer_size){
            buffer_size += LINE_BUFFER;
            buffer = realloc(buffer, buffer_size);
            if (!buffer){
                fprintf(stderr, "Buffer allocation error.\n");
                exit(EXIT_FAILURE);
            }
        }
    }

}

#define TOKEN_BUFFER_SIZE 64
#define DELIMITERS " \t\n\n\a"
char** separate_args_from_line(char* line) {
    int buffer_size = TOKEN_BUFFER_SIZE;
    char** tokens = malloc(sizeof(char*)*buffer_size);
    char* token;
    int position = 0;

    if(!tokens){
        fprintf(stderr, "token allocation error");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, DELIMITERS);
    while(token != NULL){
        tokens[position] = token;
        position++;
        if(position >= buffer_size){
            buffer_size += TOKEN_BUFFER_SIZE;
            tokens = realloc(tokens, buffer_size);
            if(!tokens){
                fprintf(stderr, "token reallocation error");
                exit(EXIT_FAILURE);
            }
        }
        token = strtok(NULL, DELIMITERS);
    }
    tokens[position] = NULL;
    return tokens;
}

int rpe_launch(char** args){
    pid_t pid, wpid;
    int status;

    pid = fork();
    if(pid == 0){
        if (execvp(args[0], args) == -1) {
            perror("rpe");
        }
    }
    else if (pid < 0){
        perror("rpe");

    }
    else {
        do{
        wpid = waitpid(pid, &status, WUNTRACED);
        } while(!WIFEXITED(status) && !WIFSIGNALED(status));
    }
    return 1;

}



char* commands[] = {
        "cd",
        "help",
        "exit"
};

int (*built_in_commands[]) (char**) = {
        &rpe_cd,
        &rpe_help,
        &rpe_exit
};

int num_builtins() {
    return sizeof(commands) / sizeof(char*);
}

int rpe_cd(char** args){
    if(args[1] == NULL){
        fprintf(stderr, "expected argument to \"cd\"\n");
    }
    else{
        if (chdir(args[1]) != 0){
            perror("chdir err");
        }
    }
    return 1;
}

int rpe_help(char** args){
    int i;
    printf("Read-Process-Execute Shell Help:\n");
    printf("\tFollowing built-in functions:\n");
    for(int i = 0; i < num_builtins(); i++){
        printf("\t\t%s\n", commands[i]);
    }
    return 1;

}

int rpe_exit(char** args){
    return 0;
}

int execute_args(char** args){
    int i;
    if (args[0] == NULL){
        return 1;
    }
    for(int i = 0; i < num_builtins(); i++){
        if (strcmp(args[0], commands[i]) == 0){
            return(*built_in_commands[i])(args);
        }
    }

    return rpe_launch(args);
}