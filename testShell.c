#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

/*
* @author Daniel Pavenko
* @date 02/23/24
* This file houses all the custom tests used for the project. It serves as my 
* custom testing harness.
*/

//function prototypes
void err(const char *errorMessage);

void runTests() {
	DIR *dir;
	struct dirent *entry;

	//Open the directory
	dir = opendir("/Users/danielpavenko/university/SPRING2024/OS/Projects/ostep-projects/processes-shell/customTests");
	if (dir == NULL) {
		err("Bad directory");
	}

	//Loop through each entry in the directory
	while ((entry = readdir(dir)) != NULL) {
		//Check if the entry is a regular file and if it ends with ".txt"
		if (entry->d_type == DT_REG && strstr(entry->d_name, ".txt") != NULL) {
			int pipefd[2]; //file descriptor for the pipe			
			pid_t pid;
	
			//creates a pipe
			if (pipe(pipefd) == -1) {
				err("pipe error");
			}

			//forks a new process
			pid = fork();

			if (pid < 0) {
				err("Fork failed");
			} else if (pid == 0) {
				//Child process
				close(pipefd[0]); //closes the read end of the pipe

				//Redirect the stdout to the write end of the pipe
				dup2(pipefd[1], STDOUT_FILENO);
				dup2(pipefd[1], STDERR_FILENO);
				close(pipefd[1]);				

				//Script execution
				char filePath[1024];
				snprintf(filePath, sizeof(filePath), "%s/%s", "/Users/danielpavenko/university/SPRING2024/OS/Projects/ostep-projects/processes-shell/customTests", entry->d_name);
				char *args[3];
				args[0] = "wish";
				args[1] = filePath;
				args[2] = NULL;
				execv("wish", args);
				err("fork failed");
			} else {
				//Parents process

				close(pipefd[1]); //closes the write end of the pipe

				//read from the pipe
				char buffer[1024];
				ssize_t bytes_read;

				int status;
				if (wait(&status) == -1) {
					err("wait");
				}

				bytes_read = read(pipefd[0], buffer, sizeof(buffer));

				if (bytes_read > 0) {
					printf("%s: %.*s", entry->d_name, (int)bytes_read, buffer);
				} else if (bytes_read == 0) {
					printf("%s: success\n", entry->d_name);
				} else {
					err("read error");
				}
			}			
		}
	}
	closedir(dir);
}

//Custom error message
void err(const char *errorMessage) {
	//char error_message[30] = "An error has occurred\n";
	//write(STDERR_FILENO, error_message, strlen(error_message)); 
	fprintf(stderr, "%s\n", errorMessage);
	exit(0);
}
