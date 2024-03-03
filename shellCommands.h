//Header file for shellCommands

void pathInit();

void cd(char *arg);

void path(char *arg[]);

void runScript(char *args[]);

void waitForChildren();

void enqueueCommandArgs(char *args[]);

void runCommandArgsQueue();
