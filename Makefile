all	:	conio.o
		$(CC) $(FLAGS) -o conio conio.c $(CLIBS)
		rm conio.o

install	:	
		mv conio /usr/local/bin

CC = gcc
CFLAGS =
CLIBS = 

conio.o	:	conio.c
	$(CC) $(FLAGS) -c conio.c

		
