#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <limits.h>
#include <termios.h>
#include <errno.h>


int rpe_cd(char** args);
int rpe_help(char** args);
int rpe_exit(char** args);
char* read_line();
char** separate_args_from_line();
void rpe_loop();
int execute_args();
void disable_raw_mode();
void enable_raw_mode();
int read_key();


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
        fflush(stdout);
        line = read_line();
        args = separate_args_from_line(line);
        status = execute_args(args);
        //printf("\n");
    } while(status);

    free(line);
    free(args);
}


#define LINE_BUFFER 1024
#define ARROW_LEFT 1000
#define ARROW_RIGHT 1001
#define BACKSPACE 127
char* read_line(){
    enable_raw_mode();
    int buffer_size = LINE_BUFFER;
    int position = 0;
    char* buffer = malloc(sizeof(char)*buffer_size);
    int cursor_position = 0;
    int c;
    while(1){
        c = read_key();
        switch(c){
        case '\n':
        case EOF:
            buffer[position] = '\0';
            disable_raw_mode();
            printf("\n");
            return buffer;
        case ARROW_LEFT:
            if (cursor_position > 0){
                cursor_position--;
                printf("\033[1D");
                fflush(stdout);
            }
            break;
        case ARROW_RIGHT:
            if (cursor_position < position){
                cursor_position++;
                printf("\033[1C");
                fflush(stdout);
            }
            break;
        case BACKSPACE:
            if(cursor_position > 0){
                memmove(&buffer[cursor_position - 1], &buffer[cursor_position], position - cursor_position);
                position--;
                cursor_position--;
                buffer[position] = '\0'
                printf("\033[1D");
                printf("\033[s");
                printf("%s ", &buffer[cursor_position]);
                printf("\033[u");
                fflush(stdout);
            }
            break;
        default:
            if (cursor_position == position){
                buffer[position] = c;
                position++;
                cursor_position++;
                putchar(c);
                fflush(stdout);
            }
            else{
                memmove(&buffer[cursor_position + 1], &buffer[cursor_position], position-cursor_position);
                buffer[cursor_position] = c;
                position++;
                cursor_position++;
                printf("\033[s");
                printf("%s", &buffer[cursor_position-1]);
                printf("\033[u");
                printf("\033[1C");
                fflush(stdout);           
            }
            if (position >= buffer_size){
                buffer_size += LINE_BUFFER;
                buffer = realloc(buffer, buffer_size);
                if (!buffer){
                    fprintf(stderr, "Buffer allocation error.\n");
                    exit(EXIT_FAILURE);
                }
            }
            break;
        }

        // if(c == '\n' || c == EOF){
        //     buffer[position] = '\0';
        //     disable_raw_mode();
        //     printf("\n");
        //     return buffer;

        // }
        // else if (c == ARROW_LEFT){
        //     if (cursor_position > 0){
        //         cursor_position--;
        //         printf("\033[D");
        //     }
        // }
        // else if (c == ARROW_RIGHT){
        //     if (cursor_position < position){
        //         cursor_position++;
        //         printf("\033[C");
        //     }
        // }
        // else{
        //     if (cursor_position == position){
        //         buffer[position] = c;
        //         position++;
        //         cursor_position++;
        //         putchar(c);
        //         fflush(stdout);
        //     }
        //     else{
        //         memmove(&buffer[cursor_position + 1], &buffer[cursor_position], position-cursor_position);
        //         buffer[cursor_position] = c;
        //         position++;
        //         cursor_position++;
        //         printf("\033[s");
        //         printf("%s", &buffer[cursor_position-1]);
        //         printf("\033[u");
        //         printf("\033[C");    
        //         fflush(stdout);       
        //     }

        // }

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


struct termios orig_termios;

void disable_raw_mode(){
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enable_raw_mode(){
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disable_raw_mode);

    struct termios raw = orig_termios;

    raw.c_lflag &= ~(ECHO | ICANON);

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int read_key(){
    int nread;
    char c;
    while((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) {
            perror("read");
        }
    }
    if (c == '\x1b'){
        char seq[3];
        if(read(STDIN_FILENO, &seq[0], 1) != 1){
            return '\x1b';
        }
        if(read(STDIN_FILENO, &seq[1], 1) != 1){
            return '\x1b';
        }
        if(seq[0] == '['){
            switch(seq[1]){
                case 'D': return ARROW_LEFT;
                case 'C': return ARROW_RIGHT;
            }
        }
        return '\x1b';
    }
    else {
        if (c == 127){
            return BACKSPACE;
        }
        return c;
    }



}