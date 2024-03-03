#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>

#include "shellCommands.h"
#include "util.h"

/*
* @author Daniel Pavenko
* @date 02/23/24
* This file houses the main functionality of the shell and is responsible for calling
* the appropriate functions in other files.
*/ 

//Constants
#define MAX_COMMAND_LENGTH 100
#define MAX_ARGUMENTS 15

//Instance Variables
char commandLine[MAX_COMMAND_LENGTH];
char *tokens[MAX_ARGUMENTS];

//Function prototype(s)
void batchMode(char* fileName);
void interactiveMode();
void tokenize(char *command);
void checkAndRouteTokens();
int handleBuiltIns(char *args[]);
int handleRedirection(char *args[]);
void handleParallel(char *args[]);

//Function that implements the batch mode of the shell
void batchMode(char* fileName) {
	//Opening the file for reading
	FILE *file = fopen(fileName, "r");
		
	//Check if we opened the file properly
	if (file == NULL) {
		errBatchMode("batchMode error: failure to open file properly");
	}

	//iterates through lines and gets commands
	while (fgets(commandLine, sizeof(commandLine), file) != NULL) {
		//Removes newline character
		commandLine[strcspn(commandLine, "\n")] = '\0';
		
		//Check if the line is empty or consists only of whitespace(s)
		char *temp = commandLine;
		int isEmpty = 1; //flag to check if line is empty
		while (*temp) {
			if (!isspace((unsigned char)*temp)) {
				isEmpty = 0;
				break;
			}
			temp++;
		}

		//Skip this iteration if the line is empty
		if (isEmpty) {
			continue;
		}

		//Tokenizes commandLine
		tokenize(commandLine);

		//Validates the tokens as args and routes them to their respective functions
		checkAndRouteTokens();
	}
	fclose(file);
	exit(0);
}

//Function that implements the interactive mode of the shell
void interactiveMode() {
	while(1) {
		printf("wish> ");
		fflush(stdout);

		//Read command from user
		if (fgets(commandLine, sizeof(commandLine), stdin) == NULL) {
			printf("\n");
			break;
		}

		//Remove trailing newline character
		commandLine[strcspn(commandLine, "\n")] = '\0';

		//Tokenize the commandLine
		tokenize(commandLine);
		
		//Validates the tokens as args and routes them to their respective functions
		checkAndRouteTokens();
	}
}

//Helper function that handles the tokenization of the commandLine. The process is manual
//as we need to use multiple delimiters (' ', '>', '&') in a fashion that strtok cannot
//handle 
void tokenize(char *commandLine) {
	int argCount = 0;
	char *cursor = commandLine; //Cursor to traverse the commandLine string

	while (*cursor != '\0' && argCount < MAX_ARGUMENTS - 1) {
		//Skip any leading delimiters (' ' or '>' or '&')
		while (*cursor == ' ' || *cursor == '>' || *cursor == '&') {
			if (*cursor == '>') {
				//Add '>' as its own token
				char *token = malloc(2);
				strcpy(token, ">\0");
				tokens[argCount++] = token;
				if (argCount >= MAX_ARGUMENTS - 1) break;
			} else if (*cursor == '&') {
				//Add '&' as its own token
				char *token = malloc(2);
				strcpy(token, "&\0");
				tokens[argCount++] = token;
				if (argCount >= MAX_ARGUMENTS - 1) break;
			}
			cursor++; //Move past the delimiter
		}

		if (*cursor == '\0') break; //Break if end of string

		//Mark the start of a token
		char *start = cursor;

		//Move cursor forward until the next delimiter or end of string
		while (*cursor != '\0' && *cursor != ' ' && *cursor != '>' && *cursor != '&') {
			cursor++;
		}

		//Allocate memory for the token and copy it from the commandLine string
		size_t tokenLength = cursor - start;
		if (tokenLength > 0) {
			char *token = malloc(tokenLength + 1); //+1 for null terminator
			if (token != NULL) {
				strncpy(token, start, tokenLength);
				token[tokenLength] = '\0'; //Null-terminate the token
				tokens[argCount++] = token; //Add the token to the tokens array
			} else {
				//Memory allocation failure
				err("tokenize error: failure to allocate memory for token");
			}
		}

	//No need to increment cursor here, it's already at the next delimiter or end of string
	}
	tokens[argCount] = NULL; //Null-terminate the tokens array
}

//Helper function that validates the tokens as arguments and routes them to their respective
//function(s), usually error processing or execution
void checkAndRouteTokens() {
	if (tokens[0] == NULL) { //No args given
		err("No args given");
	} else if (strcmp(tokens[0], "&") == 0) { //Early return if first token is "&"
		return;
	} else if (handleBuiltIns(tokens) == 0) { 
		int redirCount = 0;
		int amphCount = 0;

		//Count how many ">" and "&" are present
		for (int i = 0; tokens[i] != NULL && i < MAX_ARGUMENTS - 1; i++) {
			if (strcmp(tokens[i], ">") == 0) {
				redirCount++;
			} else if (strcmp(tokens[i], "&") == 0) {
				amphCount++;
			}
		}

		//If there are ">" but no "&", call handleRedirection
		if (redirCount > 0 && amphCount == 0) {
			handleRedirection(tokens);
		} else if (amphCount > 0) { //If "&" is present, handle parallel commands
			handleParallel(tokens);
		} else { //If there are no "&", ">" tokens present, run script normally
			enqueueCommandArgs(tokens);
		}
	}
	
	runCommandArgsQueue();

	//Free memory 
	for (int i = 0; tokens[i] != NULL; i++) {
		free(tokens[i]);
	}
}

//Helper function that handles parallel commands validation
void handleParallel(char *args[]) {
	char *tokenSubArray[MAX_ARGUMENTS] = {NULL};
	int cursor = 0;

	for (int i = 0; args[i] != NULL && i < MAX_ARGUMENTS - 1; i++) {
		if (strcmp(args[i], "&") == 0) {
			if (cursor != 0) {
				tokenSubArray[cursor] = NULL;
				
				if (handleRedirection(tokenSubArray) == 1) {
					for (int j = 0; j <= cursor; j++) {
						free(tokenSubArray[j]);
						tokenSubArray[j] = NULL;
					}
				} else {
					enqueueCommandArgs(tokenSubArray);
					for (int j = 0; j <= cursor; j++) {
						free(tokenSubArray[j]);
						tokenSubArray[j] = NULL;
					}
				}
				cursor = 0;
			}

			if (tokens[i + 1] != NULL && strcmp(tokens[i + 1], "&") == 0) {
				err("handleParallel error: two '&' cannot be adjacent");
			}
		} else {
			tokenSubArray[cursor] = malloc(strlen(args[i]) + 1);
			strcpy(tokenSubArray[cursor], args[i]);
			cursor++;
		}
	}
	
	//Check and run last command stored in tokenSubArray
	if (tokenSubArray[0] != NULL) {
		if (handleRedirection(tokenSubArray) == 0) {
			enqueueCommandArgs(tokenSubArray);
		}
	}

	//Free memory
	for (int i = 0; tokenSubArray[i] != NULL && i < MAX_ARGUMENTS; i++) {
		free(tokenSubArray[i]);
	}
}

//Helper function that handles the builtIn validation
int handleBuiltIns(char *args[]) {
	if (strcmp(args[0], "cd") == 0) {
		if (args[1] == NULL) {
			err("handleBuiltIns error: cd requires 1 argument");
		} else if (args[2] != NULL) {
			err("handleBuiltIns error: cd can only take 1 argument");
		} else {
			cd(args[1]);
			return 1;
		}
	} else if (strcmp(args[0], "path") == 0) {
		path(tokens);
		return 1;
	} else if (strcmp(args[0], "exit") == 0) {
		if (args[1] != NULL) {
			err("handleBuiltIns error: exit cannot take arguments");
		} else {
			//Make sure we've ran all queued commands before exiting
			runCommandArgsQueue();
			waitForChildren();
			exit(0);
		}
	} else {
		return 0;
	}
}

//Helper function that handles the redirection validation
int handleRedirection(char *args[]) {
	//Checks for redirection
	int redirCount = 0;
	for (int i = 0; args[i] != NULL && i < MAX_ARGUMENTS - 1; i++) {
		if (redirCount == 0) {
			if (strcmp(args[i], ">") == 0) {
				redirCount++;
				if (args[i + 1] == NULL) {
					err("handleRedirection error: no '>' argument");
				} else if (args[i + 2] != NULL) {
					err("handleRedirection error: too many '>' arguments");
				} else {
					enqueueCommandArgs(args);
					return 1; //Redirection handled and enqueued
				}		
			}
		} 
	}
	return 0; //No redirection found
}
