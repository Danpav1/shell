#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "shell.h"
#include "util.h"

/*
* @author Daniel Pavenko
* @date 02/23/24
* This file houses the main method for the shell project
*/

int main(int argc, char* argv[]) {
	if (argc == 1) {
		interactiveMode();
	} else if (argc == 2) {
		batchMode(argv[1]);
	} else {
		errBatchMode("main error: Too many files given to batchmode");
	}
	return 0;
}
