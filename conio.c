/* conio.c - A functional replacement for an IBM model 029 
 * card reader/punch
 *
 * (C) 2022 k theis <theis.kurt@gmail.com>
 * This software is licensed under the MIT license.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>

// used for submit()
#define SUBMITPORT 3505				// port of card reader
#define SUBMITADDR "192.168.0.133"	// address of card reader

#define LINESIZE 80					// cards hold 80 chars
#define MAXCARDS 100				// initial deck 100 cards (adds automatically when full)

/* pre-define subs */
void ruler(void);
void showcards(char *, int);
void replacecard(char *, char *, int);
void savefile(char *, char *, int);
void submit(char *, int);
void help(void);


void showcards(char *carddeck, int cardnumber) {
	int i, card=0;
	while (card < cardnumber) {
		printf("[%04d]",card+1);
		for (i=0; i<LINESIZE; i++) {
			printf("%c",carddeck[(card*LINESIZE)+i]);
			if (carddeck[(card*LINESIZE)+i] == '\n') i = LINESIZE+1;
		}
		card++;
		continue;
	}
	//printf("\n");
	return;
}

void replacecard(char *carddeck, char *line, int cardnumber) {
	char cmd[10], linenumber[10], newline[LINESIZE];
	int card = 0;
	sscanf(line,"%s %s",cmd, linenumber);
	card = atoi(linenumber);
	card -= 1;		// correct for offset
	if (card < 0 || card > cardnumber) {
		printf("Bad linenumber %s\n",linenumber);
		return;
	}
	memset(newline,0,LINESIZE);
	printf("Enter new line:");
	fgets(newline,LINESIZE,stdin);
	for (int i=0; i<LINESIZE; i++) carddeck[(card*LINESIZE)+i] = newline[i];
	printf("Card replaced\n");
	return;
}



void savefile(char *carddeck, char *line, int cardnumber) {
	char cmd[10], filename[40];
	FILE *fd;
	int i, card;
	sscanf(line,"%s %s",cmd,filename);
	fd = fopen(filename,"w");
	if (fd == NULL) {
		printf("Error creating file %s\n",filename);
		return;
	}
	card = 0;
	while (card < cardnumber) {
		for (i=0; i<LINESIZE; i++) {
			fprintf(fd,"%c",carddeck[(card*LINESIZE)+i]);
			if (carddeck[(card*LINESIZE)+i] == '\n') i = LINESIZE+1;
		}
		card++;
		continue;
	}
	fclose(fd);
	printf("File %s saved\n",filename);
	return;
}


void submit(char *carddeck, int cardnumber) {
	int sockfd = 0;
	struct sockaddr_in serv_addr;
	char sendbuf[LINESIZE];
	char line[LINESIZE];
	memset(line,0,LINESIZE);
	char address[LINESIZE];
	memset(address,0,LINESIZE);

	if (cardnumber-1 < 0) {
		printf("Card deck empty\n");
		return;
	}

	/* get address (<cr> uses default) */
	printf("IP ADDRESS? [%s]: ",SUBMITADDR);
	fgets(line,16,stdin);
	for (int i=1; i<sizeof(line); i++)
		if (line[i] == '\n') line[i] = '\0';
	if (line[0] == '\n') 
		strcpy(address,SUBMITADDR);
	else
		strcpy(address,line);

	/* set up network socket */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		printf("error setting up network\n");
		return;
	}
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(SUBMITPORT);
	serv_addr.sin_addr.s_addr = inet_addr(address);

	/* connect to server */
	if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		printf("error connecting to server\n");
		return;
	}

	/* send cards to server */
	int card = 0; int ntwkbufadr = 0; int bufadr = 0;
	while (card < cardnumber) {
		for (int i=0; i<LINESIZE; i++) sendbuf[i] = carddeck[(card*LINESIZE)+i];
		write(sockfd, sendbuf, strlen(sendbuf));
		card++;
		memset(sendbuf,0,LINESIZE);
		continue;
	}
	close(sockfd);
	printf("%d card submitted\n",card);
	return;
}




void ruler(void) {
	printf("0        1         2         3         4         5         6         7         8\n");
	printf("12345678901234567890123456789012345678901234567890123456789012345678901234567890\n");
	return;
}

void help(void) {
	printf("\nCommands:\n");
	printf(".quit             Exit.\n");
	printf(".help             Show this list.\n");
	printf(".save [filename]  Save the card deck to a named file.\n");
	printf(".load [filename]  Load an external file to the card deck.\n");
	printf(".submit           Send cards to tcp-based server/device.\n");
	printf(".repl [card #]    Replace an existing card. You will be prompted for new.\n");
	printf(".new              Clear existing card deck.\n");
	printf(".list             Show numbered list of cards.\n");
	printf(".del              Delete the last card in the deck.\n");
	printf(".ruler            Display column numbers.\n");

	printf("\n");
	return;
}



int main(int argc, char **argv[]) {

	char *carddeck;
	carddeck = malloc(MAXCARDS*LINESIZE);		// will add to later if needed
	if (carddeck == NULL) {
		printf("Memory error\n");
		exit(1);
	}
	int totcards = MAXCARDS;	// used to realloc if carddeck fills up. 
	char line[LINESIZE];
	memset(line,0,LINESIZE);

	char card[LINESIZE];
	int cardnumber = 0;

	int n, x;

	printf("Sim 029 Card Punch. Type .help for commands\n\n");
	ruler();
	while (1) {
		/* get a line from console */
		memset(line,0,LINESIZE);
		fgets(line,LINESIZE,stdin);
		
		/* exit app */
		if (strncmp(line,".quit",5)==0) {
			free(carddeck);
			exit(0);
		}

		/* clear the buffer */
		if (strncmp(line,".new",4)==0) {
			memset(carddeck,0,totcards*LINESIZE);
			cardnumber = 0;
			printf("Card deck cleared\n");
			continue;
		}

		/* show column numbers */
		if (strncmp(line,".ruler",4)==0) {
			ruler();
			continue;
		}

		/* show command list */
		if (strncmp(line,".help",5)==0) {
			help();
			continue;
		}

		/* delete the last line */
		if (strncmp(line,".del",4)==0) {
			if (cardnumber == 0) continue;
			for (int i=0; i<LINESIZE; i++) carddeck[((cardnumber-1)*LINESIZE)+i] = '\0';
			cardnumber--;
			continue;
		}

		/* show all the cards */
		if (strncmp(line,".list",5)==0) {
			showcards(carddeck,cardnumber);
			continue;
		}

		/* replace a line: .repl [card number] */
		if (strncmp(line,".repl",5)==0) {
			replacecard(carddeck,line,cardnumber);
			continue;
		}

		/* save carddeck to a named file */
		if (strncmp(line,".save",5)==0) {
			savefile(carddeck,line,cardnumber);
			continue;
		}

		/* submit the card deck to a tcp server */
		if (strncmp(line,".submit",3)==0) {
			submit(carddeck,cardnumber);
			continue;
		}

		/* delete existing carddeck, load a file */
		/* NOTE: if cards exceeds buffer, use realloc, so this must be in main() */
		if (strncmp(line,".load",5)==0) {
			//cardnumber = loadfile(carddeck,line,cardnumber);
			char cmd[10], filename[40], ch;
			FILE *fd;
			int i, card = 0;
			sscanf(line,"%s %s",cmd,filename);
			fd = fopen(filename,"r");
			if (fd == NULL) {
				printf("Error opening file %s\n",filename);
				continue;
			}	
			// clear buffer first
			memset(carddeck,0,totcards*LINESIZE); cardnumber = 0;
			i=0;
			while (1) {
				ch = fgetc(fd);
				if (feof(fd)) break;
				carddeck[(card*LINESIZE)+i] = ch;
				i++;
				if (ch != '\n') continue;
				while (i < LINESIZE) carddeck[(card*LINESIZE)+i++] = '\0';
				i=0;
				card++;
				if (card == totcards) {		// at end of buffer
					totcards += 10;			// add 10 cards to the deck
					char *newbuf = realloc(carddeck,totcards*LINESIZE);
					if (newbuf == NULL) {
						printf("Memory error\n");
						exit(1);			// fatal when loading a file
					}
					else
						carddeck = newbuf;
				}
				continue;
			}
			fclose(fd);
			printf("File %s loaded, %d cards read\n",filename,card);
			cardnumber = card;
			continue;
		}

		/* no commands encountered - save line to card */
		for (n=0; n<LINESIZE; n++) {
			carddeck[(cardnumber*LINESIZE)+n] = line[n];
		}
		cardnumber++;
		if (cardnumber == totcards) {	// at end of buffer
			totcards += 10;				// add 10 cards to deck
			char *newbuf = realloc(carddeck,totcards*LINESIZE);
			if (newbuf == NULL) 
				printf("Memory full\n");
			else
				carddeck = newbuf;
			continue;
		}

		continue;
	}

}



