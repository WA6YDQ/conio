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
#include <ctype.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// used for submit()
#define SUBMITPORT 3505				// port of card reader
#define SUBMITADDR "192.168.0.133"	// address of card reader

#define LINESIZE 80					// cards hold 80 chars
#define MAXCARDS 100				// initial deck of 100 cards (adds automatically when full)

// used for telnet
#define IAC     255
#define DONT    254
#define DO      253
#define WONT    252
#define WILL    251
#define SB      250
#define SE      240
#define NAWS    31
#define DEFAULT_PORT 23

/* pre-define subs */
void ruler(void);
void showcards(char *, int);
void replacecard(char *, char *, int);
void savefile(char *, char *, int);
void submit(char *, int);
void help(void);

/* global vars */
char ipaddress[20];

/* display card deck with card numbers */
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

/* replace a single (numbered) card. Will ask for a replacement */
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


/* save card deck to unix file */
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

/* send card deck over tcp/ip to reader */
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
	printf("IP ADDRESS? [%s]: ",ipaddress);
	fgets(line,16,stdin);
	for (int i=1; i<sizeof(line); i++)
		if (line[i] == '\n') line[i] = '\0';
	if (line[0] == '\n') 
		strcpy(address,ipaddress);
	else {
		strcpy(address,line);
		strcpy(ipaddress,address); // save new address for later
	}

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
	printf("%d cards submitted\n",card);
	return;
}


/* simple telnet client (see RFC854) */
void telnet(char *line) {
	int sockfd, portno, n, kbd;
	struct sockaddr_in serv_addr;
	struct hostent *server;
	char cmd[20], adr[40], port[40];
	char recvbuff[1024], kbdbuff[1024];

	/* get address, port */
	memset(adr,0,sizeof(adr));
	sscanf(line,"%s %s %s",cmd,adr,port);
	// printf("host [%s]  port [%s]\n",adr,port);
	
	portno = atoi(port);
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		printf("Error - cannot open socket\n");
		return;
	}
	server = gethostbyname(adr);
	if (server == NULL) {
		printf("Cannot determine host %s\n",adr);
		return;
	}
	memset(&serv_addr,0,sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)(&serv_addr.sin_addr.s_addr),server->h_length);
	serv_addr.sin_port = htons(portno);
	if (connect(sockfd,(struct sockaddr *)&serv_addr, sizeof(serv_addr))<0) {
		printf("Error connecting - no/bad response.\n");
		return;
	}

	/* open keyboard as a device */
	kbd = open("/dev/stdin", O_RDONLY | O_NONBLOCK);
	if (kbd == 0) {
		printf("Error connecting to stdin\n");
		return;
	}

	/* set socket for non-blocking */
	fcntl(sockfd, F_SETFL, O_NONBLOCK);
	
	/* init buffers */
	memset(kbdbuff,0,sizeof(kbdbuff));
	memset(recvbuff,0,sizeof(recvbuff));

	printf("Connected. Type ^] <enter> to exit\n\n");

	while (1) {
		usleep(2000);	// keep down cpu cycles
		
		/* test keyboard */
		n = read(kbd, kbdbuff, sizeof(kbdbuff));
		if (n > 0) {	// send to socket
			if (kbdbuff[0]==0x1d) break;			// exit connection
			kbdbuff[n] = '\r';		// force a CR
			write(sockfd, kbdbuff, strlen(kbdbuff));
			memset(kbdbuff,0,sizeof(kbdbuff));
		}
		
		/* test socket */
		n = read(sockfd, recvbuff, sizeof(recvbuff));
		if (n < 1) continue;
		recvbuff[n] = '\0';
		/* test received chars from server */
		if (recvbuff[0] == IAC) {	// IAC from server
			printf("IAC: ");	// this line is for debugging only
			if (recvbuff[1] == DO && recvbuff[2] == NAWS) {
				printf("DO NAWS\n");
				unsigned char willnaws[] = {255,251,31};	// IAC,WILL,NAWS
				write(sockfd,willnaws,3);
				unsigned char sbnaws[] = {255,250,31,0,80,0,24,255,240};
				write(sockfd,sbnaws,9);
				continue;
			}
			printf("DO WILL\n");	// this line is for debugging only
			for (int i=0; i<n; i++) {
				if (recvbuff[i] == DO)
					recvbuff[i] = WONT;
				else if (recvbuff[i] == WILL)
					recvbuff[i] = DO;
			}
			write(sockfd,recvbuff,n);
			continue;
		}

		/* display received data */
		printf("%s",recvbuff);
		memset(recvbuff,0,sizeof(recvbuff));
		continue;
	}
	
	close(sockfd);
	printf("Exiting connection\n");
	return;
}



void ruler(void) {
	printf("0        1         2         3         4         5         6         7         8\n");
	printf("12345678901234567890123456789012345678901234567890123456789012345678901234567890\n");
	return;
}

void help(void) {
	printf("\nCommands:\n");
	printf(".quit/.q               Exit.\n");
	printf(".help                  Show this list.\n");
	printf(".save [filename]       Save the card deck to a named file.\n");
	printf(".load [filename]       Load an external file to the card deck.\n");
	printf(".submit                Send cards to tcp-based server/device.\n");
	printf(".repl [card #]         Replace an existing card. You will be prompted for another.\n");
	printf(".new                   Clear existing card deck.\n");
	printf(".list                  Show numbered list of cards.\n");
	printf(".del                   Delete the last card in the deck.\n");
	printf(".ruler                 Display column numbers.\n");
	printf(".telnet [host] [port]  Telnet to a host.\n");

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
	char line[LINESIZE], cmdline[LINESIZE];
	memset(line,0,LINESIZE);

	char card[LINESIZE];
	int cardnumber = 0;

	int n, x;

	/* allow SUBMITADDR to be changed */
	memset(ipaddress,0,sizeof(ipaddress));
	strcpy(ipaddress,SUBMITADDR);

	printf("Sim 029 Card Punch. Type .help for commands\n\n");
	ruler();
	while (1) {
		/* get a line from console */
		memset(line,0,LINESIZE);
		fgets(line,LINESIZE,stdin);
		/* copy line to cmdline, converting to UC */
		for (int i=0; i<LINESIZE; i++) cmdline[i] = toupper(line[i]);
		
		/* exit app */
		if (strncmp(cmdline,".Q",2)==0) {
			free(carddeck);
			exit(0);
		}

		/* telnet client */
		if (strncmp(cmdline,".TELNET",7)==0) {
			telnet(line);
			continue;
		}

		
		/* clear the buffer */
		if (strncmp(cmdline,".NEW",4)==0) {
			memset(carddeck,0,totcards*LINESIZE);
			cardnumber = 0;
			printf("Card deck cleared\n");
			continue;
		}

		/* show column numbers */
		if (strncmp(cmdline,".RULER",4)==0) {
			ruler();
			continue;
		}

		/* show command list */
		if (strncmp(cmdline,".HELP",5)==0) {
			help();
			continue;
		}

		/* delete the last line */
		if (strncmp(cmdline,".DEL",4)==0) {
			if (cardnumber == 0) continue;
			for (int i=0; i<LINESIZE; i++) carddeck[((cardnumber-1)*LINESIZE)+i] = '\0';
			cardnumber--;
			continue;
		}

		/* show all the cards */
		if (strncmp(cmdline,".LIST",5)==0) {
			showcards(carddeck,cardnumber);
			continue;
		}

		/* replace a line: .repl [card number] */
		if (strncmp(cmdline,".REPL",5)==0) {
			replacecard(carddeck,line,cardnumber);
			continue;
		}

		/* save carddeck to a named file */
		if (strncmp(cmdline,".SAVE",5)==0) {
			savefile(carddeck,line,cardnumber);
			continue;
		}

		/* submit the card deck to a tcp server */
		if (strncmp(cmdline,".SUBMIT",3)==0) {
			submit(carddeck,cardnumber);
			continue;
		}

		/* delete existing carddeck, load a file */
		/* NOTE: if cards exceeds buffer, use realloc, so this must be in main() */
		if (strncmp(cmdline,".LOAD",5)==0) {
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



