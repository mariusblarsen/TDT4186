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

// Parse input string into list of arguments
char **parse_input_to_arguments(char* input){
    // Define a buffer of a fixed length, because we don't 
    // know how many arguments we are going to have. 
    char **buffer = malloc(sizeof(char) * 1024);

    // Split the input using strtok, and put the results into the buffer
    char* token = strtok(input, DELIM);
    int argument_count = 0;
    while(token != (char*)NULL){
        buffer[argument_count++] = token;
        token = strtok(NULL, DELIM);
    }

    // We now know the number of arguments, and can allocate precicely enough space
    char** arguments = malloc(sizeof(char*) * (argument_count + 1));

    // Copy over elements from buffer to arguments variable
    for (int i = 0; i < argument_count; i++)
    {
        arguments[i] = malloc(sizeof(char) * (strlen(buffer[i]) + 1));   // Allocate memory 
        strcpy(arguments[i], buffer[i]);    // Copy value from buffer
    }

    // Append one null char pointer to arguments that signal end of the array
    buffer[argument_count] = (char*)0;
    
    // Free the buffer
    free(buffer);

    return arguments;
}

// Scan input from terminal, and return pointer to a string (char array) containing entire input.
char* scan_input(){
    char *buffer = malloc(sizeof(char) * 1024);

    int current_char;   //TODO: Explain why this is int and not char? 
    int input_char_length = 0;
    while (1){
        current_char = getchar();  // Read from stdin
        if (current_char == '\n' || current_char == EOF){  // on enter
            buffer[input_char_length] = '\0';
            break;
        } else {
            buffer[input_char_length++] = current_char;
        }
    }
    
    // Now we know exactly how much space our input needs, and can allocate precicely that amount
    char* input = malloc(sizeof(char) * (input_char_length + 1));   // Plus one due to terminating '\0'

    // Copy the string from the buffer into input variable
    strcpy(input, buffer);

    // We no longer need the buffer, so we can free it from memory
    free(buffer);

    return input;
}

// Get index of redirection symbol (< or >) in argument array. 
// Returns -1 if arguments does not contain redirect symbol. 
int get_redirection_index(char** arguments){
    int counter = 0;
    while (arguments[counter] != NULL) {
        char argument = *arguments[counter];
        if (argument == '\0') break;    // Reached end of arguments
        if (argument == '>' || argument == '<') return counter; // Found redirection, return index
        
        counter++;
    }
    return -1;
}

// Get the command and parameters from arguments array. 
// This will discard everything before potential redirection symbols. 
// If redirection is present, a new array is returned, so arguments parameter is not changed. 
char** get_command_with_parameters(char** arguments){
    int redirection_index = get_redirection_index(arguments);
    if (redirection_index <= 0){
        return arguments;
    }
    
    char** result = malloc(sizeof(char*) * (redirection_index + 1));    // +1 becuase terminating array with NULL 

    for (int i = 0; i < redirection_index; i++)
    {
        result[i] = malloc(sizeof(char) * (strlen(arguments[i]) + 1));
        strcpy(result[i], arguments[i]);
    }
    result[redirection_index] = (char*)0;
    
    return result;
}

void execute_normal(char** arguments){
    int execute_status = execvp(arguments[0], get_command_with_parameters(arguments));
    if (execute_status < 0){
        printf("ERROR: execute_normal failed.\n");
    }
}

// Instead of printing, writes to file.
void execute_with_output_redirection(char** arguments, int redirection_index){

    // Save to restore normal stdin/out later
    int original_stdout = dup(STDOUT_FILENO); 
    int original_stdin = dup(STDIN_FILENO);

    const char* filename = arguments[redirection_index+1];
    int file_descriptor = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666); //eqv with creat(filename)
    if(file_descriptor != -1){
        // saved_stdout = dup(STDOUT_FILENO); // Save to return later
        dup2(file_descriptor, STDOUT_FILENO); 
        dup2(file_descriptor, STDERR_FILENO);
        //TODO: Check the return values of dup2 here
        close(file_descriptor);
    }else{
        printf("Unable to open file: %s\n", filename);
        return;
    }
    int execute_status = execvp(arguments[0], get_command_with_parameters(arguments));
    if (execute_status < 0){
        printf("No success!\n");
    }
    
    // Return io to original
    dup2(original_stdout, STDOUT_FILENO);
    dup2(original_stdin, STDIN_FILENO);

    // Make sure to close io
    close(original_stdout);
    close(original_stdin);

}

void execute_with_input_redirection(char** arguments, int redirection_index){

    // Save to restore normal stdin/out later
    int original_stdout = dup(STDOUT_FILENO); 
    int original_stdin = dup(STDIN_FILENO);

    const char* filename = arguments[redirection_index+1];
    int file_descriptor = open(filename, O_RDONLY, 0666);
    if(file_descriptor != -1){
        // saved_stdin = dup(STDIN_FILENO); // Save to return later
        dup2(file_descriptor, STDIN_FILENO); 
        dup2(file_descriptor, STDERR_FILENO);
        //TODO: Check the return values of dup2 here
        close(file_descriptor);
    }else{
        printf("Unable to open file: %s\n", filename);
        return;
    }
    int execute_status = execvp(arguments[0], get_command_with_parameters(arguments));
    if (execute_status < 0){
        printf("No success!\n");
    }

    // Return io to original
    dup2(original_stdout, STDOUT_FILENO);
    dup2(original_stdin, STDIN_FILENO);

    // Make sure to close io
    close(original_stdout);
    close(original_stdin);
}

// Execute a command given as an array of arguments
void execute(char** arguments){
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

    if (child_pid < 0){
        // This will occur if fork() fails.(child_pid < 0) 
        perror("ERROR: Fork failed. \n");
        // Kill the faulty process, with exit signal EXIT_FAILURE
        exit(EXIT_FAILURE);
        return;
    }

    if (child_pid == 0){
        // In child process
        int redirection_index = get_redirection_index(arguments);
        if (redirection_index <= 0){
            execute_normal(arguments);
        }
        else if (*arguments[redirection_index] == '>'){
            execute_with_output_redirection(arguments, redirection_index);
        }
        else if (*arguments[redirection_index] == '<'){
            execute_with_input_redirection(arguments, redirection_index);
        }
        else{
            printf("ERROR: Redirection index returned unexpected result.\n");
        }
    }
    else{
        // In parent process, start loop again
        child_pid = wait(&child_status);
        printf("End of process %d: ", child_pid);
        if (WIFEXITED(child_status)) {
            printf("The process ended with exit(%d).\n", WEXITSTATUS(child_status));
        }
        if (WIFSIGNALED(child_status)) {
            printf("The process ended with kill -%d.\n", WTERMSIG(child_status));
        }
    }
    
}

// Print the whole array of arguments, where each argument is separated by a comma 
// Also prints the sub-array where any redirection part is removed
void print_arguments(char** arguments){
    printf("Arguments: [");    
    int i = 0;
    while (arguments[i] != NULL){
        printf("%s", arguments[i++]);
        if (arguments[i] != NULL) printf(", ");
    }
    printf("]\n");

    char** arguments_with_parameters = get_command_with_parameters(arguments);
    printf("Command with arguments: [");
    i = 0;
    while (arguments_with_parameters[i] != NULL){
        printf("%s", arguments_with_parameters[i++]);
        if (arguments_with_parameters[i] != NULL) printf(", ");
    }
    printf("]\n");

    printf("Redirection index: %d\n", get_redirection_index(arguments));
}

int main(int argc, char **argv) {
    while (1){
        printf("$ ");
        char* input = scan_input();
        char** arguments = parse_input_to_arguments(input);

        print_arguments(arguments);
        
        // Execute command 
        execute(arguments);
        
        // Free memory allocated to arguments
        free(arguments);
    }
    
    return 0;
}