#include "unp.h"
#include "unpifi.h"
#include <time.h>
#include <dirent.h>

static void recvfrom_int(int);
static void sig_chld(int);
static int count;
char filelist[MAXLINE] = ""; 

struct datagram512 {
	int info; //0 if it contains data, else it is the port number to bind on
	int sequence; //Sequence number of data being sent. 0 if a port number is being sent.
	char data[MAXLINE]; //Contains data for file download or list
};

int stringToInteger(char str[MAXLINE]){
	int mult = 1, re = 0, len = strlen(str), i;
	for (i = len-1; i>=0; i--){
		re = re + ((int)str[i] - 48) * mult;
		mult = mult*10;
	}
	return re;
}

void getcurrentfilelist(){
	struct dirent *pDirent;
	DIR *pDir;

	pDir = opendir("bin/");
	while((pDirent = readdir(pDir)) != NULL){
		strcat(filelist, pDirent->d_name);
		strcat(filelist, "\n");
	}
	closedir(pDir);
}

void printSomethingFromFile(char filename[MAXLINE], int sockfd, SA *pcliaddr, socklen_t clilen){
	FILE *fp;
	char buff[MAXLINE];
	char filepath[MAXLINE] = "bin/";
	char seqnum[MAXLINE];
	int i = 1;

	strcat(filepath, filename);
	fp = fopen(filepath, "r");
	while (Fgets(buff, 506, fp)){
		sprintf(seqnum, "%d:", i++);
		if (buff == NULL) break;
		strcat(seqnum, buff);
		Sendto(sockfd, seqnum, strlen(seqnum), 0, pcliaddr, clilen);
		sleep(1);
	}
	Sendto(sockfd, "0:", 2, 0, pcliaddr, clilen);
	fclose(fp);
}

void talktoclient(int sockfd, int slidWinSize){
	int n;
	struct sockaddr_in pcliaddr, servaddr;
	socklen_t len = sizeof(pcliaddr);
	char mesg[MAXLINE], cli_address[MAXLINE], serv_address[MAXLINE], temp[MAXLINE];
	const char delim[2] = " ";
	char *token;

	n = Recvfrom(sockfd, mesg, MAXLINE, 0, (SA *)&pcliaddr, &len);

	Inet_ntop(AF_INET, &pcliaddr.sin_addr, cli_address, MAXLINE);
	printf("Received request from %s:%d\n", cli_address, ntohs(pcliaddr.sin_port));

	/*Creating new port to send it to client to rebind*/
	srand(time(NULL));
	int newPort = rand()%55000 + 1024;
	char newPortStr[10];
	sprintf(newPortStr, "%d", newPort);
	
	/*Bind to the new port*/
	if (getsockname(sockfd, (SA *)&servaddr, &len) < 0) printf("getsockname error\n");
	Inet_ntop(AF_INET, &servaddr.sin_addr, serv_address, MAXLINE);
	printf("Client should now bind to : %s:%d\n", serv_address, newPort); 
	servaddr.sin_port = htons(newPort);
	Sendto(sockfd, newPortStr, MAXLINE, 0, (SA *)&pcliaddr, len);
	Close(sockfd);
	sockfd = Socket(AF_INET, SOCK_DGRAM, 0);
	Bind(sockfd, (SA *) &servaddr, sizeof(servaddr));

	/*Wait for acknowledgement form client on the new port*/
	char ack[MAXLINE];
	n = Recvfrom(sockfd, ack, MAXLINE, 0, (SA *)&pcliaddr, &len);

	if (!strcmp(ack, "portack")) {
		printf("--ACK on new port received--\n");
	}

	int i=1;
	if (!strcmp(mesg, "list")) {
		getcurrentfilelist();
		sprintf(temp, "%d:", i);
		strcat(temp, filelist);
		Sendto(sockfd, temp, MAXLINE, 0, (SA *)&pcliaddr, len);
	} 
	else {
		token = strtok(mesg, delim);
		int i; char *filename;
		for(i=0; token != NULL; i++){
			if (i==1) filename = token;
			token = strtok(NULL, delim);
		}
		printf("file name is %s\n", filename);
		printSomethingFromFile(filename, sockfd, (SA *)&pcliaddr, len);
	}
	Close(sockfd);
	count++;
}

int main(int argc, char **argv){
	int sockfd, i=0;
	struct sockaddr_in servaddr, cliaddr, *sa;
	const int on = 1;
	struct ifi_info *ifi, *ifihead;
	int portno = atoi(argv[1]);
	int sockfd_list[MAXLINE];
	int slidWinSize = stringToInteger(argv[2]);
//	char filepath[MAXLINE] = argv[3];

	/*To print total number of datagrams received*/
	Signal(SIGINT, recvfrom_int);

	/*Handling death of child*/
	Signal(SIGCHLD, sig_chld);

	/*Binding on all unicast addresses*/
	for (ifihead = ifi = (struct ifi_info *)Get_ifi_info(AF_INET, 1); ifi != NULL; ifi = ifi->ifi_next) {
		sockfd = Socket(AF_INET, SOCK_DGRAM, 0);
		sockfd_list[i++] = sockfd;

		Setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

		sa = (struct sockaddr_in *) ifi->ifi_addr;
		sa->sin_family = AF_INET;
		sa->sin_port = htons(portno);
		Bind(sockfd, (SA *) sa, sizeof(*sa));
		printf("-- Server message -- Bound %s\n", Sock_ntop((SA *) sa, sizeof(*sa)));
	}

	/*Select between all unicast addresses bound*/
	int maxfd = -9999, N = i;
	fd_set rset;
	FD_ZERO(&rset);

	for (i=0; i<N; i++){
		maxfd = max(maxfd, sockfd_list[i]);
	}
	maxfd = maxfd + 1;

	/*Listen to client continuosly*/
	pid_t pid;
	for( ; ; ){
		for (i=0; i<N; i++){
			FD_SET(sockfd_list[i], &rset);
		}
		Select(maxfd, &rset, NULL, NULL, NULL);

		for (i=0; i<N; i++){
			if (FD_ISSET(sockfd_list[i], &rset)){
				if ((pid = fork()) == 0){
					talktoclient(sockfd_list[i], slidWinSize);
				}
				else {
					int status;
					//waitpid(pid, &status, 0);
				}
				break;
			}
		}
	}
}

/*Handling exit of server through Ctrl-C*/
static void recvfrom_int(int signo){
        printf("\n-- Server message -- Received %d datagrams\n", count);
	exit(0);
}

/*Handling SIGCHLD*/
static void sig_chld(int signo){
	pid_t pid;
	int stat;

	while ((pid = waitpid(-1, &stat, WNOHANG)) > 0)
		printf("\n-- Server message -- Child with pid %d terminated\n", pid);

	//exit(0); 
}
