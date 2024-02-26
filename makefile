CC = gcc
CFLAGS = -Wall -Wextra -std=c99

all: wish testMain

wish: wish.o shell.o builtIn.o util.o
	$(CC) $(CFLAGS) -o wish wish.o shell.o builtIn.o util.o

testMain: testMain.o testUtil.o testShell.o
	$(CC) $(CFLAGS) -o testMain testMain.o testUtil.o testShell.o

wish.o: wish.c
	$(CC) $(CFLAGS) -c wish.c

shell.o: shell.c
	$(CC) $(CFLAGS) -c shell.c

builtIn.o: builtIn.c
	$(CC) $(CFLAGS) -c builtIn.c

util.o: util.c
	$(CC) $(CFLAGS) -c util.c

testMain.o: testMain.c
	$(CC) $(CFLAGS) -c testMain.c

testUtil.o: testUtil.c
	$(CC) $(CFLAGS) -c testUtil.c

testShell.o: testShell.c
	$(CC) $(CFLAGS) -c testShell.c

clean:
	rm -f wish testMain *.o

