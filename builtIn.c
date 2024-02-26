#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "util.h"

/*
* @author Daniel Pavenko
* @date 02/23/24
* This file houses all of the code for the built in commands. This is used by
* shell.c
*/

//Instance variables
char *pathVar[10] = {"/bin", "/usr/bin", NULL};

//Function prototype
void err(const char *errorMessage);

//Implementation of cd
void cd(char *arg) {
	if (chdir(arg) != 0) {
		err("chdir failed");
	}
}

//Implementation of path
void path(char* args[]) {
	//Path always overwrites the previous pathVar when its called
	for (int i = 0; i < 10; i++) {
		pathVar[i] = NULL;
	}

	//Fills pathVar, args[i + 1] because we dont want the command token
	for (int i = 0; args[i + 1] != NULL && i < 9; i++) {
		pathVar[i] = malloc(strlen(args[i + 1]) + 1);
		strcpy(pathVar[i], args[i + 1]);
	}
}

//Tries to run scripts
void runScript(char *args[]) {
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
			for (int i = 0; i < 10; i++) {
				char scriptPath[1024];
				snprintf(scriptPath, sizeof(scriptPath), "%s/%s", pathVar[i], args[0]);
				if (access(scriptPath, F_OK | X_OK) == 0) {
					//it does exist in path
					execv(scriptPath, args);
					err("execv in runScript in path failed");
				}
			}
			//it doesnt exist in cwd or path
			err("File not found");	
		}
	} else {
		//Parent process
		wait(NULL); //makes sure parent finishes first so we print things properly
	}
}
