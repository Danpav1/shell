#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

#include "util.h"

#define MAX_ARGUMENTS 15
#define MAX_CHILDREN 10

/*
* @author Daniel Pavenko
* @date 02/23/24
* This file houses all of the code for the built in commands. This is used by
* shell.c
*/

//Instance variables
char *pathVar[MAX_ARGUMENTS] = {"/bin", "/usr/bin", NULL};
char **parallelArgs[MAX_ARGUMENTS];
int numOfParallelArgs = 0;
pid_t childPids[MAX_CHILDREN];
int numChildren = 0;

//Function prototype
void err(const char *errorMessage);
void waitForChildren();
void addToParallelArray(char *args[]);
void runParallelArray();

//Implementation of cd
void cd(char *arg) {
	if (chdir(arg) != 0) {
		err("chdir failed");
	}
}

//Implementation of path
void path(char *args[]) {
	//Path always overwrites the previous pathVar when its called
	for (int i = 0; i < MAX_ARGUMENTS - 1; i++) {
		pathVar[i] = NULL;
	}

	//Fills pathVar, args[i + 1] because we dont want the command token
	for (int i = 0; args[i + 1] != NULL && i < MAX_ARGUMENTS - 2; i++) {
		pathVar[i] = malloc(strlen(args[i + 1]) + 1);
		strcpy(pathVar[i], args[i + 1]);
	}
}

//Tries to run scripts
void runScript(char *args[]) {
	/*	
	printf("\n");
	for (int i = 0; args[i] != NULL && i < MAX_ARGUMENTS; i++) {
		printf("args[%d]: %s\n", i, args[i]);
	}
	*/

	int redirFlag = 0;

	if (numChildren >= MAX_CHILDREN) {
		err("Maximum number of parallel scripts exceeded");
	}
	
	//redirection
	for (int i = 0; args[i] != NULL && i < MAX_ARGUMENTS - 1; i++) {
		if (strcmp(args[i], ">") == 0) {
			args[i] = NULL;
			openRedirection(args[i + 1]);
			args[i + 1] = NULL;
			redirFlag = 1;
			break;
		}
	}

	//fork a new process
	pid_t pid = fork();

	if (pid < 0) {
		err("Fork failed");
	} else if (pid == 0) {
		//Child process
		//Checks if script exists in cwd
		if (access(args[0], F_OK | X_OK) == 0) {
			//it does exist in cwd
			execv(args[0], args);
			err("execv in runScript in cwd failed");
		} else {
			//it doesn't exist
			//check if script exists within the path
			for (int i = 0; i < MAX_ARGUMENTS - 1; i++) {
				char scriptPath[1024];
				snprintf(scriptPath, sizeof(scriptPath), "%s/%s", pathVar[i], args[0]);
				if (access(scriptPath, F_OK | X_OK) == 0) {
					//it does exist in path
					//printf("scriptPath: %s\n", scriptPath);
					execv(scriptPath, args);
					//perror("error: ");
					err("execv in runScript in path failed");
				}
			}
			//it doesnt exist in cwd or path
			err("File not found");	
		}
	} else {
		//Parent process
		//wait(NULL);
		//	printf("\tpid started: %d\n", pid);
		childPids[numChildren++] = pid;
		//waitForChildren();
		//closeRedirection();
		if (redirFlag == 1) {
			closeRedirection();
		}
	}
	//closeRedirection();
}


//blocks main process (waits) until all children are complete

void waitForChildren() {
	
	int status;
	pid_t pid;

	while (numChildren > 0) {
		pid = waitpid(-1, &status, 0);
		if (pid < 0) {
			err("error waiting for child process");
		} else {
			//printf("\twaiting on pid: %d\n", pid);
			//	printf("\tpid finished: %d\n", pid);
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

//deep copies and adds copy to parallelArray
void addToParallelArray(char *args[]) {
	if (numOfParallelArgs >= MAX_ARGUMENTS) {
		err("no space in parallel args");
	}

	char **argsCopy = malloc((MAX_ARGUMENTS + 1) * sizeof(char *));
	if (argsCopy == NULL) {
		err("failure to allocate memory for parallelArgs array");
	}

	int i;
	for (i = 0; args[i] != NULL && i < MAX_ARGUMENTS; i++) {
		//Allocate memory for each string and copy it
		argsCopy[i] = strdup(args[i]);
		if (argsCopy[i] == NULL) {
			err("Failed to duplicate a string for parallel args");

			//Free any strings already copied and the array itself
			while (--i >= 0) free(argsCopy[i]);
			free(argsCopy);
			return;
		}
	}
	//Null-terminate the array of pointers
	argsCopy[i] = NULL;

	//Add the deep copied array of strings to parallelArgs
	parallelArgs[numOfParallelArgs++] = argsCopy;

}

//runs every arg array in parallel array
void runParallelArray() {
	for (int i = 0; i < numOfParallelArgs; i++) {
		if (parallelArgs[i] != NULL) {
			runScript(parallelArgs[i]);
			//waitForChildren();
		}

		//Free the deep copied argument strings
		for (int j = 0; parallelArgs[i][j] != NULL; j++) {
			free(parallelArgs[i][j]);
		}

		//Free the argument array itself
		free(parallelArgs[i]);
		parallelArgs[i] = NULL;
	}
	numOfParallelArgs = 0;
	waitForChildren();
}
