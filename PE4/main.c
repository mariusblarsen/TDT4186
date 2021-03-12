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

char* read_command(){
    char input[256];
    printf("> ");
    scanf("%[^\n\r]", input);

    return strtok(input, split);
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


char* concat(const char *s1, const char *s2)
{
    char *result = malloc(strlen(s1) + strlen(s2) + 1); // +1 for the null-terminator
    // in real code you would check for errors in malloc here
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

int main(int argc, char **argv) {

    //int execl_status = execl("hello_world", "", NULL);
    //printf("what?");
    //if (execl_status == -1){
    //    printf("success?");
    //}

    // First token = the command.
    char * token = read_command();
    char * parameters[256];
    int counter = 0;
    size_t total_string_len;

    // Command
    printf("Command:\n");
    printf("%s\n", token);
    char * command = token;
    
    // Parameters
    printf("Parameters:\n");
    token = strtok(NULL, split);

    while(token != NULL && strcmp(token, "<") != 0 && strcmp(token, ">") != 0) {
        printf("%s\n", token);
        parameters[counter++] = token;
        //total_string_len +=  sizeof(token) / sizeof(token[0]);
        total_string_len += strlen(token);
        token = strtok(NULL, " \t");
    } 
    
    if(token != NULL){
        // Redirection
        printf("The redirection:\n");
        printf("%s\n", token);
        token = strtok(NULL, split);
        if(token != NULL){
            // Filename
            printf("The filename:\n");
            printf("%s\n", token);
        }
    }

    // ADD ALL seperations " "
    total_string_len += (size_t)(counter-1);
    printf("Total string length: %zu\n", total_string_len);

    char* parameter_string = concat("", parameters[0]);
    for (int i = 1; i<counter; i++){
        parameter_string = concat(parameter_string, parameters[i]); 
        if (i != (counter-1)){
            parameter_string = concat(parameter_string, " ");
        }
    }
    parameter_string = concat(parameter_string,'\0');

    printf("STRING IN MAIN():\n");
    printf("%s\n", parameter_string);
    
    execute_command(command, parameter_string);
    
    free(parameter_string);
    
    return 0;
}