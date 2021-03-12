#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

// Delimiter
const char split[3] = " \t";

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

    return strtok(input, split);
}

// Get the command part of given input
char* get_command(char* input){
    // Make a copy of the input to prevent side effects
    char * input_copy = malloc(strlen(input) + 1); 
    strcpy(input_copy, input);

    char* command_part = strtok(input_copy, split);
    
    // Command part now has allocated entire length of input in memory. 
    // But we only need the length of the actual command
    char* command = malloc(strlen(command_part) + 1);
    strcpy(command, command_part);

    free(input_copy);

    return command;
}

// Get the parameters part of the given input
char* get_parameters(char* input){
    // Make a copy of the input to prevent side effects
    char * input_copy = malloc(strlen(input) + 1); 
    strcpy(input_copy, input);

    // Create temporary result to store parameters
    // Allocate the entire size of the input (including termination char)
    // This is only a temporary result, since we don't need the entire allocated size, 
    // but at this point we do not know how much size is needed. 
    char temp_result[strlen(input_copy) + 1];
    
    // First token is the command part
    char* current_token = strtok(input_copy, split);

    // Next token should be the first parameter
    current_token = strtok(NULL, split);

    int counter = 0;
    int total_string_size = 0;
    // Loop through all parameters, and add them to temp result
    while(  current_token != NULL && 
            strcmp(current_token, "<") != 0 && 
            strcmp(current_token, ">") != 0)
    {
        // Add this token to the result
        for (size_t i = 0; i < strlen(current_token); i++)
        {
            temp_result[counter++] = current_token[i];
        }
        // Append a space as a separator
        temp_result[counter++] = ' ';

        // Go to next token
        current_token = strtok(NULL, split);
    } 

    // Replace the last space with string termination char
    temp_result[counter] = '\0';

    // Our result now contains the string that we want, 
    // but it has some unused space. Let's fix that. 
    // Now that we know the size, we can allocate the result 
    // array with the correct size, to avoid wasting memory. 
    // char* final_result = malloc(counter);
    char* final_result = malloc(counter);

    // Copy over the characters that we need
    for (size_t i = 0; i < counter + 1; i++)
    {
        final_result[i] = temp_result[i];
    }

    // We no longer need the copied input, remove it from memory
    free(input_copy);
    
    return final_result;
}

// Get the IO redirection part of the given input. 
// Will return "<", ">" or NULL. 
char* get_io_redirection_type(char* input){
    if (strchr(input, '<') != NULL){
        return (char*)"<";
    }
    if (strchr(input, '>') != NULL){
        return (char*)">";
    }
    return (char*)NULL;
}

// Get the IO Redirection path of the given input. 
// This is the path where the redirection should point. 
char* get_io_redirection_path(char* input){
    char* redirection_type = get_io_redirection_type(input);

    if (redirection_type == (char*)NULL){
        return (char*)NULL;
    }

    // Make a copy of the input to prevent side effects
    char * input_copy = malloc(strlen(input) + 1); 
    strcpy(input_copy, input);

    char* token = strtok(input_copy, redirection_type);
    token = strtok(NULL, redirection_type);

    if (token == (char*)NULL) return (char*) NULL;

    char* result = malloc(strlen(token) + 1);
    strcpy(result, token);

    free(input_copy);

    return result;
}


void execute_command(char* command, char* parameters){
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
    if (child_pid > 0){
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
    else if (child_pid == 0){
        // In child process
        
        // Run the command.
        // Execl replaces the child (data and all) with the command to be executed.
        int execl_status = execl(command, parameters, NULL);
        if (execl_status == -1){
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

int main(int argc, char **argv) {

    char input[256];
    printf("> ");
    scanf("%[^\n\r]", input);

    char* command = get_command(input);
    printf("Command: %s\n", command);

    char* parameters = get_parameters(input);
    printf("Parameters: %s\n", parameters);

    char* redirection_type = get_io_redirection_type(input);
    printf("Redirection type: %s\n", redirection_type);

    char* redirection_path = get_io_redirection_path(input);
    printf("Redirection path: %s\n", redirection_path);

    
    return 0;
}