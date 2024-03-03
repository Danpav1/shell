#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "util.h"

#define MAX_ARGUMENTS 15
#define MAX_CHILDREN 10

/*
* @author Daniel Pavenko
* @date 02/23/24
* This file houses the functionality of the builtIn commands and running external
* scripts.
*/

//Instance variables
char *pathVar[MAX_ARGUMENTS] = {NULL};
char **pendingCommandArgs[MAX_ARGUMENTS];
int numPendingCommandArgs = 0;
pid_t childPids[MAX_CHILDREN];
int numChildren = 0;

//Function prototype(s)
void err(const char *errorMessage);
void waitForChildren();
void enqueueCommandArgs(char *args[]);
void runCommandArgsQueue();
void pathInit();

//Implementation of builtIn command "cd"
void cd(char *arg) {
	if (chdir(arg) != 0) {
		err("cd error: chdir failed");
	}
}

//Initilizes the pathVar to have the correct malloced starting dir(s)
void pathInit() {
	char *defaultOne = malloc(4);
	strcpy(defaultOne, "/bin");
	pathVar[0] = defaultOne;

	char *defaultTwo = malloc(8);
	strcpy(defaultTwo, "/usr/bin");
	pathVar[1] = defaultTwo;
}

//Implementation of builtIn command "path"
void path(char *args[]) {	
	//Overwrite and free pathVar
	for (int i = 0; i < MAX_ARGUMENTS - 1; i++) {
		free(pathVar[i]);
		pathVar[i] = NULL;
	}

	//Fills pathVar, args[i + 1] because we dont want the command token
	for (int i = 0; args[i + 1] != NULL && i < MAX_ARGUMENTS - 2; i++) {

		//Check if all args in path are valid directories
		if (access(args[i + 1], F_OK | X_OK) != 0) {
			err("Invalid path arg found");
		}

		pathVar[i] = malloc(strlen(args[i + 1]) + 1);
		strcpy(pathVar[i], args[i + 1]);
	}
}

//Fuction that handles finding, checking and running of non builtIn commands. These
//are usually external scripts.
void runScript(char *args[]) {
	int redirFlag = 0;

	//Makes sure we dont have too many processes running at a given time
	if (numChildren >= MAX_CHILDREN) {
		err("runScript error: Maximum number of parallel scripts exceeded");
	}
	
	//Checks for and handles redirection
	for (int i = 0; args[i] != NULL && i < MAX_ARGUMENTS - 1; i++) {
		if (strcmp(args[i], ">") == 0) {
			args[i] = NULL;
			openRedirection(args[i + 1]);
			args[i + 1] = NULL;
			redirFlag = 1;
			break;
		}
	}

	//Fork a new process
	pid_t pid = fork();

	if (pid < 0) {
		err("runScript error: fork failed");
	} else if (pid == 0) {
		//Child process
		//Checks if script exists in cwd
		if (access(args[0], F_OK | X_OK) == 0) {
			//Script does exist in cwd
			execv(args[0], args);
			err("runScript error: execv in cwd section failed");
		} else {
			//Script doesn't exist
			//Check if script exists within the path
			for (int i = 0; i < MAX_ARGUMENTS - 1; i++) {
				char scriptPath[1024];
				snprintf(scriptPath, sizeof(scriptPath), "%s/%s", pathVar[i], args[0]);
				if (access(scriptPath, F_OK | X_OK) == 0) {
					//Script does exist in path
					execv(scriptPath, args);
					err("runScript error: execv in path section failed");
				}
			}
			//Script doesnt exist in cwd or path
			err("runScript error: file was not found in CWD or PATH");
		}
	} else {
		//Parent process
		childPids[numChildren++] = pid;

		//Close the redirection if we opened it
		if (redirFlag == 1) {
			closeRedirection();
		}
	}
}

//Blocks main process (waits) until all children are complete
void waitForChildren() {
	int status;
	pid_t pid;

	while (numChildren > 0) {
		//Set any terminated fork to pid
		pid = waitpid(-1, &status, 0);
		if (pid < 0) {
			err("waitForChildren error: error while waiting for child process");
		} else {;
			//printf("pid finished: %d", pid);
			//Tries to match our terminated childs pid to any of our "active" pids
			for (int i = 0; i < numChildren; i++) {
				if (childPids[i] == pid) {
					for (int j = i; j < numChildren - 1; j++) {
						childPids[j] = childPids[j + 1];
					}
					numChildren--;
					break;
				}
			}
		}		
	}
}

//Deep copies and adds copy to pendingCommandArgs
void enqueueCommandArgs(char *args[]) {
	if (numPendingCommandArgs >= MAX_ARGUMENTS) {
		err("enqueueCommandArgs error: pendingCommandArgs is full");
	}

	//Allocation of the copy
	char **argsCopy = malloc((MAX_ARGUMENTS + 1) * sizeof(char *));
	if (argsCopy == NULL) {
		err("enqueueCommandArgs error: failure to allocate memory for argsCopy");
	}

	int i;
	for (i = 0; args[i] != NULL && i < MAX_ARGUMENTS; i++) {
		//Allocate memory for each string and copy it
		argsCopy[i] = strdup(args[i]);
		if (argsCopy[i] == NULL) {
			err("enqueueCommandArgs error: failure to duplicate string for argsCopy");

			//Free any strings already copied and the array itself
			while (--i >= 0) free(argsCopy[i]);
			free(argsCopy);
			return;
		}
	}
	//Null-terminate the array of pointers
	argsCopy[i] = NULL;

	//Add the deep copied array of strings (argsCopy) to pendingCommandArgs
	pendingCommandArgs[numPendingCommandArgs++] = argsCopy;
}

//runs every arg array within pendingCommandArgs
void runCommandArgsQueue() {
	for (int i = 0; i < numPendingCommandArgs; i++) {
		//Run the commandArgs array if its valid
		if (pendingCommandArgs[i] != NULL) {
			runScript(pendingCommandArgs[i]);
		}

		//Free the deep copied argument strings in the commandArgs array
		for (int j = 0; pendingCommandArgs[i][j] != NULL; j++) {
			free(pendingCommandArgs[i][j]);
		}

		//Free commandArgs array itself
		free(pendingCommandArgs[i]);
		pendingCommandArgs[i] = NULL;
	}
	numPendingCommandArgs = 0;
	waitForChildren();
}
