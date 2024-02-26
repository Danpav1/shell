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
#define MAX_ARGUMENTS 10

//Instance Variables
char command[MAX_COMMAND_LENGTH];
char *tokens[MAX_ARGUMENTS];

//Function prototypes
void batchMode(char* fileName);
void interactiveMode();
void tokenize(char *command);
void useTokens();

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
		while (*cursor == ' ' || *cursor == '>') {
			if (*cursor == '>') {
				// Add '>' as its own token
				tokens[argCount++] = ">";
				if (argCount >= MAX_ARGUMENTS - 1) break;
			}
			cursor++; // Move past the delimiter
		}

		if (*cursor == '\0') break; // Break if end of string

		// Mark the start of a token
		char *start = cursor;

		// Move cursor forward until the next delimiter or end of string
		while (*cursor != '\0' && *cursor != ' ' && *cursor != '>') {
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
	/*
	for (int i = 0; tokens[i] != NULL && i < MAX_ARGUMENTS - 1; i++) {
		printf("tokens[%d]: %s\n", i, tokens[i]);
	}
	*/

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
			exit(0);
		}
	} else if (strcmp(tokens[0], "path") == 0) { //builtin "path"
		path(tokens);
	} else { //run scripts (everything other than builtin's)
		//checks for redirection
		int redirCount = 0;
		for (int i = 0; tokens[i] != NULL && i < MAX_ARGUMENTS - 1; i++) {
			if (redirCount == 0) {
				if (strcmp(tokens[i], ">") == 0) {
					tokens[i] = NULL;
					redirCount++;
					if (tokens[i + 1] == NULL) {
						err("No redir destination");
					} else if (tokens[i + 2] != NULL) {
						err("Too many redir destinations");
					} else {
						openRedirection(tokens[i + 1]);
					}		
				}
			} else {
				tokens[i] = NULL;
			}
		}
		runScript(tokens);
		closeRedirection();
	}

	//Clears the tokens array after they are used
	for (int i = 0; i < MAX_ARGUMENTS - 1; i++) {
		tokens[i] = NULL;
	}
}
