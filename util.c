#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

/*
* @author Daniel Pavenko
* @date 02/23/24
* This file houses all of the utility methods that the rest of the project uses
*/

//Instance variables
int stdoutCopy; //saves the fd of our stdout

//function prototypes
void err(const char *errorMessage);

//sets up and opens redirection
void openRedirection(char *outputFile) {
        int fd = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
                err("error opening file in redirection");
        }

	//save the current file descriptor (fd)
	stdoutCopy = dup(STDOUT_FILENO);
	if (stdoutCopy < 0) {
		err("error duplicating stdout");
	}

	//redirect to output file
        if (dup2(fd, STDOUT_FILENO) < 0) {
                err("error in dup2");
        }
        close(fd); //close fd as its no longer needed
}

//closes redirection and restores stdout
void closeRedirection() {
	//restores original stdout
	if (dup2(stdoutCopy, STDOUT_FILENO) < 0) {
		err("error restoring stdout");
	}
}

//Custom generic error message
void err(const char *errorMessage) {
	char error_message[30] = "An error has occurred\n";
	write(STDERR_FILENO, error_message, strlen(error_message)); 
	//fprintf(stderr, "%s\n", errorMessage);
	exit(0);
}

//Custom batchMode error message
void errBatchMode(const char *errorMessage) {
        char error_message[30] = "An error has occurred\n";
        write(STDERR_FILENO, error_message, strlen(error_message));
        //fprintf(stderr, "%s\n", errorMessage);
        exit(1);
}
