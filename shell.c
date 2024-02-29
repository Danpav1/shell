#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>

#include "builtIn.h"
#include "util.h"

/*
* @author Daniel Pavenko
* @date 02/23/24
* This file houses the code for the functionality of the shell. It currently
* has all the code needed for the project except for the built in commands.
*/

//Constants
#define MAX_COMMAND_LENGTH 100
#define MAX_ARGUMENTS 15

//Instance Variables
char command[MAX_COMMAND_LENGTH];
char *tokens[MAX_ARGUMENTS];

//Function prototypes
void batchMode(char* fileName);
void interactiveMode();
void tokenize(char *command);
void useTokens();
void handleRedirection(char *args[]);

//Function that implements the batch mode of the shell
void batchMode(char* fileName) {
	//Opening the file for reading
	FILE *file = fopen(fileName, "r");
		
	//check if we opened the file properly
	if (file == NULL) {
		errBatchMode("Error opening file");
	}

	//iterates through lines and gets commands
	while (fgets(command, sizeof(command), file) != NULL) {
		//removes newline character
		command[strcspn(command, "\n")] = '\0';
		
		// check if the line is empty or consists only of whitespace(s)
		char *temp = command;
		int isEmpty = 1; //flag to check if line is empty
		while (*temp) {
			if (!isspace((unsigned char)*temp)) {
				isEmpty = 0;
				break;
			}
			temp++;
		}

		//skip this iteration if the line is empty
		if (isEmpty) {
			continue;
		}

		//tokenizes command
		tokenize(command);

		//uses tokens
		useTokens();
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
		if (fgets(command, sizeof(command), stdin) == NULL) {
			printf("\n");
			break;
		}

		//Remove trailing newline character
		command[strcspn(command, "\n")] = '\0';

		//Tokenize the command
		tokenize(command);
		
		//Uses the tokens
		useTokens();
	}
}

//Helper function that implements custom tokenizations
void tokenize(char *command) {
	int argCount = 0;
	char *cursor = command; // Cursor to traverse the command string

	while (*cursor != '\0' && argCount < MAX_ARGUMENTS - 1) {
		// Skip any leading delimiters (spaces or '>')
		while (*cursor == ' ' || *cursor == '>' || *cursor == '&') {
			if (*cursor == '>') {
				// Add '>' as its own token
				tokens[argCount++] = ">";
				if (argCount >= MAX_ARGUMENTS - 1) break;
			} else if (*cursor == '&') {
				//Add '&' as its own token
				tokens[argCount++] = "&";
				if (argCount >= MAX_ARGUMENTS - 1) break;
			}
			cursor++; // Move past the delimiter
		}

		if (*cursor == '\0') break; // Break if end of string

		// Mark the start of a token
		char *start = cursor;

		// Move cursor forward until the next delimiter or end of string
		while (*cursor != '\0' && *cursor != ' ' && *cursor != '>' && *cursor != '&') {
			cursor++;
		}

		// Allocate memory for the token and copy it from the command string
		size_t tokenLength = cursor - start;
		if (tokenLength > 0) {
			char *token = malloc(tokenLength + 1); // +1 for null terminator
			if (token != NULL) {
				strncpy(token, start, tokenLength);
				token[tokenLength] = '\0'; // Null-terminate the token
				tokens[argCount++] = token; // Add the token to the tokens array
			} else {
				// Memory allocation failure
				err("tokenization malloc failure");
			}
		}
	// No need to increment cursor here, it's already at the next delimiter or end of string
	}
	tokens[argCount] = NULL; // Null-terminate the tokens array
}


//Function that uses the tokens and sees if they are valid arguments
void useTokens() {

	if (tokens[0] == NULL) { //No args given
		err("Tokens array is empty");
	} else if (strcmp(tokens[0], "cd") == 0) { //builtin "cd"
		if (tokens[1] == NULL) {
			err("No arguments");
		} else if (tokens[1] != NULL && tokens[2] != NULL) {
			err("Too many arguments");
		} else {
			cd(tokens[1]);
		}
	} else if (strcmp(tokens[0], "exit") == 0) { //builtin "exit"
		if (tokens[1] != NULL) {
			err("Cannot exit with arguments");
		} else {
			//	printf("exit called\n");
			runParallelArray();
			waitForChildren();
			exit(0);
		}
	} else if (strcmp(tokens[0], "path") == 0) { //builtin "path"
		path(tokens);
	} else { //run scripts (everything other than builtin's)

		char *parallelArgs[MAX_ARGUMENTS] = {NULL};
		int cursor = 0;
		int redirCount = 0;
		int amphCount = 0;

		//run scripts like normal if theres no '>' or '&'
		for (int i = 0; tokens[i] != NULL && i < MAX_ARGUMENTS - 1; i++) {
			if (strcmp(tokens[i], ">") == 0) {
				redirCount++;
			} else if (strcmp(tokens[i], "&") == 0) {
				amphCount++;
			}
		}

		if (redirCount > 0 && amphCount == 0) {
			handleRedirection(tokens);
			runParallelArray();
			return;
		}

		//if there are none, run like normal
		if (redirCount == 0 && amphCount == 0) {
			//runScript(tokens);
			addToParallelArray(tokens);
			//waitForChildren();
			//return;
		} else {
		
			//early return if first token is "&"
			if (strcmp(tokens[0], "&") == 0) {
				return;
			}
			int redirFlag = 0;
			//check and runs parallel commands
			for (int i = 0; tokens[i] != NULL && i < MAX_ARGUMENTS - 1; i++) {
				redirFlag = 0;
				if (strcmp(tokens[i], "&") == 0) {
					if (cursor != 0) {
						parallelArgs[cursor] = NULL;
						
						for (int j = 0; parallelArgs[j] != NULL && j < MAX_ARGUMENTS - 1; j++) {
							if (strcmp(parallelArgs[j], ">") == 0) {
								handleRedirection(parallelArgs);
								
								for (int j = 0; j <= cursor; j++) {
									free(parallelArgs[j]);
									parallelArgs[j] = NULL;
								}

								redirFlag = 1;
							}
						}
						if (redirFlag == 0) {
							//runScript(parallelArgs);
							addToParallelArray(parallelArgs);
							//waitForChildren();
						}
						for (int j = 0; j <= cursor; j++) {
							free(parallelArgs[j]);
							parallelArgs[j] = NULL;
						}
						cursor = 0;
					}
					if (tokens[i + 1] != NULL && strcmp(tokens[i + 1], "&") == 0) {
						err("two '&' cannot be adjacent");
					}
				} else {
					parallelArgs[cursor] = malloc(strlen(tokens[i]) + 1);
					strcpy(parallelArgs[cursor], tokens[i]);
					cursor++;
				}
			}

			redirCount = 0;

			for (int i = 0; parallelArgs[i] != NULL && i < MAX_ARGUMENTS - 1; i++) {
				if (strcmp(parallelArgs[i], ">") == 0) {
					redirCount++;
				}
			}

			//check and run last command
			if (parallelArgs[0] != NULL) {
				if (redirCount == 0) { 
					//runScript(parallelArgs);
					addToParallelArray(parallelArgs);
					//waitForChildren();
				} else {
					handleRedirection(parallelArgs);
				}
			}
		}
	}

	
	runParallelArray();
	//waitForChildren();

	//Clears the tokens array after they are used
	for (int i = 0; i < MAX_ARGUMENTS - 1; i++) {
		tokens[i] = NULL;
	}
}

void handleRedirection(char *args[]) {
	//checks for redirection
	int redirCount = 0;
	for (int i = 0; args[i] != NULL && i < MAX_ARGUMENTS - 1; i++) {
		if (redirCount == 0) {
			if (strcmp(args[i], ">") == 0) {
				//args[i] = NULL;
				redirCount++;
		 		if (args[i + 1] == NULL) {
					err("No redir destination");
				} else if (args[i + 2] != NULL) {
					err("Too many redir destinations");
				} else {
					//openRedirection(args[i + 1]);
					addToParallelArray(args);
				}		
			}
		} else {
			//args[i] = NULL;
		}
	}
	//runScript(args);
	//closeRedirection();
	//waitForChildren();
}
