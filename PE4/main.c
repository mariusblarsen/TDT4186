#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// Delimiter
#define DELIM " \t"


char* replaceWord(const char* s, const char* oldW, 
                  const char* newW) 
{  
    // Author: Henrik Fjellheim
    char* result; 
    int i, cnt = 0; 
    int newWlen = strlen(newW); 
    int oldWlen = strlen(oldW); 
  
    // Counting the number of times old word 
    // occur in the string 
    for (i = 0; s[i] != '\0'; i++) { 
        if (strstr(&s[i], oldW) == &s[i]) { 
            cnt++; 
  
            // Jumping to index after the old word. 
            i += oldWlen - 1; 
        } 
    } 
  
    // Making new string of enough length 
    result = (char*)malloc(i + cnt * (newWlen - oldWlen) + 1); 
  
    i = 0; 
    while (*s) { 
        // compare the substring with the result 
        if (strstr(s, oldW) == s) { 
            strcpy(&result[i], newW); 
            i += newWlen; 
            s += oldWlen; 
        } 
        else
            result[i++] = *s++; 
    } 
  
    result[i] = '\0'; 
    return result; 
} 

char* scan_input(){
    char input[256];
    printf("> ");
    scanf("%[^\n\r]", input);

    return strtok(input, DELIM);
}

// Get the command part of given input
char* get_command(char* input){
    // Make a copy of the input to prevent side effects
    char * input_copy = malloc(strlen(input) + 1); 
    strcpy(input_copy, input);

    char* command_part = strtok(input_copy, DELIM);
    
    // Command part now has allocated entire length of input in memory. 
    // But we only need the length of the actual command
    char* command = malloc(strlen(command_part) + 1);
    strcpy(command, command_part);

    free(input_copy);

    return command;
}

// Get the parameters part of the given input
char** get_parameters(char** input, int pos){
    char** result = malloc(sizeof(char) * 64);
    int counter = 0;

    while ( input[counter] != NULL && 
            *input[counter] != '\0' && 
            *input[counter] != '<' && 
            *input[counter] != '>')
    {
        result[counter] = input[counter];
        counter++;
    }

    return result;
}

// Get the IO redirection part of the given input. 
// Will return "<", ">" or NULL. 
char get_io_redirection_type_v2(char** input){

    int counter = 0;
    while ( input[counter] != NULL && 
            *input[counter] != '\0')
    {
        if (*input[counter] == '>') return '>';
        if (*input[counter] == '<') return '<';
        counter++;
    }
    return '\0';
}

// Get the IO Redirection path of the given input. 
// This is the path where the redirection should point. 
char *get_io_redirection_path_v2(char** input){
    int counter = 0;
    while ( input[counter] != NULL && 
            *input[counter] != '\0')
    {
        if (*input[counter] == '>') return input[++counter];
        if (*input[counter] == '<') return input[++counter];
        counter++;
    }
    return '\0';

}

void execute(char** args, int pos){
    // Used to keep track of zombies, to kill off. 
    int child_status;
    // Kills all zombies.
    do{
        child_status = waitpid(-1, NULL, WNOHANG);
        
        if (child_status > 0){
            // Print deceased zombie PID
            printf("Dead zombie: %d \n", child_status);
        }
            
    } while(child_status > 0);

    // Fork process
    int child_pid = fork();
    int fd = 0; // File descriptor

    if (child_pid > 0){
        // In parent process, start loop again
        child_pid = wait(&child_status);
        close(fd); // Closing any potential file descriptors
        printf("End of process %d: ", child_pid);
        if (WIFEXITED(child_status)) {
                printf("The process ended with exit(%d).\n", WEXITSTATUS(child_status));
        }
        if (WIFSIGNALED(child_status)) {
                printf("The process ended with kill -%d.\n", WTERMSIG(child_status));
        }

    }
    else if (child_pid == 0){
        // In child process
        
        // Run the command.
        // Execvp replaces the child (data and all) with the command to be executed.
        
       
        const char* filename = get_io_redirection_path_v2(args);
        if (get_io_redirection_type_v2(args) == '>'){
            // Instead of printing, writes to file.
            fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666); //eqv with creat(filename)
            if(fd != -1){
                dup2(fd, STDOUT_FILENO); 
                dup2(fd, STDERR_FILENO);
                //TODO: Check the return values of dup2 here
                close(fd);
            }else{
                printf("Unable to open file: %s\n", filename);
                return;
            }
            

        }
        else if (get_io_redirection_type_v2(args) == '<'){
            
            fd = open(filename, O_RDONLY, 0666);
            printf("FD: %d\n", fd);
            if(fd != -1){
                dup2(fd, STDIN_FILENO); 
                dup2(fd, STDERR_FILENO);
                //TODO: Check the return values of dup2 here
                close(fd);
            }else{
                printf("Unable to open file: %s\n", filename);
                return;
            }
        }

        int execvp_status = execvp(args[0], get_parameters(args, pos));
        if (execvp_status == -1){
            printf("No success!\n");
        }
    }
    else{
        // This will occur if fork() fails.(child_pid < 0) 
        perror("Error\n");
        // Kill the faulty process, with exit signal EXIT_FAILURE
        exit(EXIT_FAILURE);
    }

}

char **get_args(char* input, int pos){
    char **args = malloc(sizeof(char) * 1024);
    char *token;
    int i = 0;
    token = strtok(input, DELIM);
    while(token != (char*)NULL){
        args[i] = token;
        i++;
        token = strtok(NULL, DELIM);
    }
    args[i] = NULL;
    return args;
}


int main(int argc, char **argv) {
    
    while (1){
        
        printf("$ ");
        char *buffer = malloc(sizeof(char) * 1024);
        /*
            Read input from user
        */
        printf("TAKING THE INPUT:\n");
        
        int c;
        int pos = 0;
        while (1){
            printf("1\n");
            c = getchar();  // Read from stdin
            printf("C: %c\n", c);

            if (c == '\n'){  // on enter || c == EOF
                printf("3\n");
                buffer[pos] = '\0';
                printf("4\n");
                break;
            } else {
                printf("5\n");
                buffer[pos++] = c;
                printf("6\n");
            }
        }
        
        /**
         * Parse input to args
        **/
        printf("GETTING THE ARGS FROM INPUT\n");
        char **args = get_args(buffer, pos);
        printf("\n--args------\n");
        int i = 0;
        while (args[i] != NULL){
            printf("%s ", args[i++]);
        }
  
        printf("\n------------\n");
        printf("\n----params------\n");
        char **params = get_parameters(args, pos);
        while (params[i] != NULL){
            printf("%s ", params[i++]);
        }
        printf("\n------------\n");
        char direction = get_io_redirection_type_v2(args);
        printf("Direction:\t%c\n", direction);
        char *io_path = malloc(1000*64);

        io_path = get_io_redirection_path_v2(args);
        printf("IO Path:\t%s\n", io_path);
        // TODO: Handle direction of < or >
        // TODO: split io_redirection to from_path and to_path
        // TODO: Handle I/O
        /**
         * Execute args
        **/
        execute(args, pos);

        // Free all allocated data
        free(args);
        free(params);


    }

    // free(buffer);
    return 0;
}