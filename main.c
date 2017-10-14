/*
*  The Trevor Shell (tsh)
*  Copyright © 2017 Trevor D. Brown. All rights reserved.
*
*  This program was written in partial fulfillment of requirements for Project 1, in CS 425-001 (Fall 2017).
*   CS 425-001 (Fall 2017) - Dr. Qi Li
*/

// Libraries included in program:
#include <stdio.h>          // Standard I/O library
#include <signal.h>         // Signal Handling library
#include <unistd.h>         // Standard symbolic constants and types
#include <stdlib.h>         // C standard Library
#include <string.h>         // String library
#include <sys/types.h>      // System types library.
#include <sys/wait.h>       // Wait library.

// Constant Integers
#define BUFFER_SIZE  1024     // Buffer size constant
#define COMMAND_SIZE 1024    // Command size constant
#define DIRECTORY_PATH_SIZE 1024    // Directory path size constant (for current directory.)
#define HISTORY_SIZE 1024       // History size constant
#define HISTORY_FILE_PATH_SIZE 1024 // History file path size constant

// Buffer and Command spaces (Character array (aka String) and Character pointer array.)
char buffer[BUFFER_SIZE];  // Buffer space
char *command[COMMAND_SIZE];  // Command space
char currentDirectory[DIRECTORY_PATH_SIZE]; // Current Working Directory space.
char *history[HISTORY_SIZE];    // History space.
char historyfile[HISTORY_FILE_PATH_SIZE];      // History file space.

// Runtime Variables
int concurrent = 0;         // Concurrent process check.
int parse_count = 0;        // Parse counter.
int history_count = 0;      // History counter.

// vvvvvvvvvvvvvvvvvvvv FUNCTIONS vvvvvvvvvvvvvvvvvvvvvvvvvvvv //

// Diplay Prompt Function
void displayPrompt(){
    getcwd(currentDirectory, sizeof(currentDirectory));     // Retrieve the current working directory from the operating system.
    printf("[%s] tsh>", currentDirectory);                  // Display the current directory in brackets, and the prompt indicator.
}

// Get Command function.
void getCommand(){
    scanf(" %[^\n]s", buffer);          // Accept user input
}

// Buffer Parser function
void parseBuffer(){
    
    // Parser Variables
    char *temp = NULL;      // Temporary character pointer
    parse_count = 0;        // Set the parse counter to 0.
    
    // Break the buffer into multiple tokens, using the return, or new, line character "\n"
    temp = strtok (buffer, " \n");
    
    // Duplicate each token into the final command character pointer array.
    while (temp != NULL){
        command[parse_count++] = strdup(temp);
        temp = strtok(NULL, " \n");
    }
}

// Concurrency check function
void checkConcurrent(char *lastcharacter){
    
    // If the last character in the command is an ampersand (&), set the concurrent flag to 1.
    // Else, set the concurrent flag to 0.
    if (strcmp(lastcharacter, "&") == 0){
        concurrent = 1;
    }
    else{
        concurrent = 0;
    }
}

// Execute function
void  executeCommand(char *command, char *arguments[]){
    
    pid_t pid;      // Process identifier
    int status;     // Process execution status
    
    if ((pid = fork()) == 0) {
        // Check if command is to run concurrently (i.e. "&" at end of command)
        checkConcurrent(arguments[parse_count-1]);
        
        // If the process is concurrent, remove the ampersand (&) from the arguments.
        if (concurrent == 1){
            strcpy(arguments[parse_count - 1], NULL);
        }
        
        // Return an error if the execution fails (program does not exists, general errors, etc.)
        if (execvp(command, arguments) < 0) {
            fprintf(stderr, "Error - Command execution failed. (Did you type in a valid command?)\n");
            exit(-1);
        }
    }
    // Return an error if the process fork fails to complete.
    else if (pid < 0) {
        fprintf(stderr, "Error - Fork created PID less than 0.\n");
        exit(-1);
    }
    else {
        // If the process is not concurrent, wait for the child to finish.
        if (concurrent == 0){
            while (wait(&status) != pid);
        }
    }
    
    // Reset concurrency flag.
    concurrent = 0;
}

// vvvvvvvvvvvvvvvvvvvvvvvv Built-in Commands vvvvvvvvvvvvvvvvvvvvv//

// Change Directory (cd)
void bic_cd(char *command, char *arguments[]){
    if (chdir(arguments[1]) == -1) { /* change cwd to path */
        fprintf(stderr, "Error - Directory [%s] does not exist. No change. \n", arguments[1]);
    }
    getcwd(currentDirectory, DIRECTORY_PATH_SIZE);      // Retreive the current working directory.
}

// History (hst)
void bic_hst(int visible){
    
    // Setup history retrieval:
    char *line = NULL;  // Character pointer for holding current line of file.
    size_t len = 0;     // Variable for file length.
    ssize_t read = 0;   // Variable for length of read.
    
    history_count = 0;          // Set the history counter to 0.
    int history_count_top = 0;  // Variable to indicate the top of the history count.
    
    FILE *history_temp = NULL;   // History file runtime space.
    history_temp = fopen(historyfile, "r");  // Open the history file on disk, for reading. Associate with the history file runtime space.
    
    // vvvvvvvvvv Begin History vvvvvvvvvv //
    
    // Retrieve history from file
    while((read = getline(&line, &len, history_temp)) != -1){
        history[history_count++] = strdup(line);
    }
    
    fclose(history_temp); // Close the history file runtime space.
    
    // Visible or hidden check
    if (visible == 1){
        // If history is "visible", display the history content.
        printf("tsh History\n");
        history_count_top = history_count - 11; // Compute the top of the history list.
        
        // If the top of the list is less than 0, make the top 0.
        if (history_count_top < 0){
            history_count_top = 0;
        }
        
        // Print each line of the history.
        while(history_count_top < history_count){
            printf("%i: %s", history_count_top, history[history_count_top]);
            history_count_top++;
        }
    }
   
}

// History check function
void checkHstFile(char *home){
    
    // Construct the file name:
    strcat(historyfile, home);                  // Add home directory
    strcat(historyfile, "/.tsh_history");       // Add .tsh_history file
    
    // History file check
    if(access(historyfile, F_OK ) != -1 ) {
        // Ready, populate array.
        bic_hst(0);
    } else {
        // Create History File
        fprintf(stderr, "Error - History file does not exist. Creating a new one.\n");
        FILE *newhistoryfile = NULL;                // Create file.
        newhistoryfile = fopen(historyfile,"a");    // Write file to disk.
        fclose(newhistoryfile);                     // Close the file.
    }
}

// Record History Entry function.
void recordHistoryEntry(){
    FILE *historyfiletoappend = NULL;                // History file runtime space.
    char *entrytorecord = NULL;                      // String to write to file.
    
    entrytorecord = strdup(buffer);                // Store the buffer contents.
    strcat(entrytorecord, "\n");                     // Append a new line character.
    historyfiletoappend = fopen(historyfile,"a");    // Open file for appending.
    fprintf(historyfiletoappend, "%s", entrytorecord);      // Append the buffer contents to the file.
    fclose(historyfiletoappend);                     // Close the file
    bic_hst(0);                                      // Run the history command quietly.
    
}

// Clear History function
void clearHistory(){
    FILE *historyfiletoclear = NULL;                // History file runtime space.
    historyfiletoclear = fopen(historyfile, "w");   // Open file, with write flag.
    fclose(historyfiletoclear);                     // Immediately close the file.
    
    // Clear the history array.
    int i = 0;
    for (i = 0; i < history_count; i++){
        strcpy(history[i], "");
    }
    
    // Reset the history counter.
    history_count = 0;
}

// Clear "things" - buffer, command space, ect. function
void clearThings(){
    // Clear the buffer
    strcpy(buffer, "");
    
    // Clear the command space.
    if (parse_count != 0){
        int i = 0;
        for(i = 0; i < parse_count; i++){
            //printf("Removing: %i [%s]\n", i, command[i]);
            strcpy(command[i], "");
        }
    }
}

// Execute a historic command function.
void executeHistoricCommand(int history_index){
    // Prepare the Historic Command Execution.
    clearThings();  // Clear the buffer and command spaces.
    bic_hst(0);     // Run the history command quietly.
    
    if (history_index < history_count){
        strcpy(buffer, history[history_index]); // Copy the desired command into the buffer.
        parseBuffer();     // Parse the command.
        recordHistoryEntry();
        executeCommand(command[0], command);    //
    }
    else{
        // Print an error stating the history entry does not exist.
        fprintf(stderr, "Error - History entry [%i] does not exist.\n", history_index);
    }
}

// Signal Handling Function
void handle_SIGINT(){
    // If Ctrl-C is invoked:
    printf("\n");           // Print a new line
    strcpy(buffer, "hst");  // Store the command "hst" in the buffer.
    recordHistoryEntry();   // Record the buffer as a history entry.
    parseBuffer();          // Parse the buffer.
    bic_hst(1);             // Run the "hst" command "loudly".
    clearThings();          // Clear the buffer and command spaces.
    displayPrompt();        // Display the command prompt.
}

// vvvvvvvvvvvvvvvvvvvv Main Function vvvvvvvvvvvvvvvvvvv //

int main(int argc, char *argv[]){
    // Environmental Variables
    char *home = getenv("HOME");  // User's home directory path.
    
    // Prepares the signal handler.
    struct sigaction handler;
    handler.sa_handler = handle_SIGINT;
    
    // History file check
    checkHstFile(home);
    
    // Print welcome message
    printf("Welcome to the Trevor Shell (tsh)!\n");
   
    // Semi-infinite loop (ends upon "exit" call)
    while (1){
        // Establish signal handler.
        sigaction(SIGINT, &handler, NULL);  // Signal handler
        
        // Primary looping functions
        displayPrompt();                    // Displays the prompt.
        getCommand();                       // Gets the user input.
        recordHistoryEntry();               // Record the user input in the history.
        parseBuffer();                      // Parse the user input.
        
        // Check Special Commands
        if (strcmp(command[0], "exit") == 0){
            break;                                  // if "exit", break loop.
        }
        else if (strcmp(command[0], "cd") == 0){
            bic_cd(command[0], command);            // If "cd", run the built-in "change directory" command.
        }
        else if (strcmp(command[0], "hst") == 0){
            bic_hst(1);                              // If "hst", run the built-in "history" command.
        }
        else if (strcmp(command[0], "!") == 0){
            int history_index = atoi(command[1]);    // If "!", translate the history index into a valid integer.
            executeHistoricCommand(history_index);   // Execute the historic command specified.
        }
        else if (strcmp(command[0], "r") == 0){
            executeHistoricCommand(history_count - 2);    // If "r", execute the most recent command.
        }
        else if (strcmp(command[0], "hstclear") == 0){
            clearHistory();
        }
        else{
            executeCommand(command[0], command);    // If no special command, execute the provided command.
        }
        
        clearThings();  // Clear the buffer and command spaces.
    }
    
    // Goodbye!
    printf("\nY'all come back now! - Trevor D. Brown\n"); // Print Goodbye Message
    return 0;    // End program.
}

