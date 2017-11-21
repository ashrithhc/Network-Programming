#include "unp.h"
#include "unpifi.h"
#include <time.h>

/*Randomnly generates a port that is valid unless already taken*/
int getClientPort(){
	srand(time(NULL));
	return rand()%55000 + 1024;
}

/* Utility function to convert string to integer*/
int stringToInteger(char str[MAXLINE]){
	int mult = 1, re = 0, len = strlen(str), i;
	for (i = len-1; i>=0; i--){
		re = re + ((int)str[i] - 48) * mult;
		mult = mult*10;
	}
	return re;
}

int splitThisString(char S[MAXLINE]){
	char delim[2] = ":";
	char *token;

	token = strtok(S, delim);
	int retInt = stringToInteger(token);

	printf("Sequence number received = %d\n", retInt);

	while (token != NULL){
		token = strtok(NULL, delim);
		break;
	}

	return retInt;
}

/*Send to server the message types only if it is valid*/
int processInput(char input[MAXLINE], int sockfd, const SA *pservaddr, socklen_t servlen){

	if (!strcmp(input, "quit")){
		printf("-- Client exiting --\n");
		exit(0);
	}
	if ((strstr(input, "download") != NULL) || (!strcmp(input, "list"))){
		Sendto(sockfd, input, strlen(input), 0, pservaddr, servlen);
		return 1;
	}
	else return 0;
}

void ddg_cli(FILE *fp, int sockfd, const SA *pservaddr, socklen_t servlen, char *server_IP, int servPort){
	int n, ret, newPort, len, j;
	char sendline[MAXLINE], recvline[MAXLINE + 1], tempStr[MAXLINE];
	struct sockaddr_in servaddr;

	printf("-- Client message -- Server : %s:%d\n", server_IP, servPort);
	while(Fgets(sendline, MAXLINE, fp) != NULL){
		sendline[strcspn(sendline, "\n")] = '\0';

		ret = processInput(sendline, sockfd, (SA *)pservaddr, sizeof(*pservaddr));

		if (!ret) continue; //To make sure that we 'read' only if we have written data

		n = Read(sockfd, recvline, MAXLINE);
		newPort = stringToInteger(recvline);

		if (getsockname(sockfd, (SA *)&servaddr, &len) < 0) printf("--Client message-- : getsockname error\n");
		servaddr.sin_family = AF_INET;
		servaddr.sin_port = htons(newPort);
		Inet_pton(AF_INET, server_IP, &servaddr.sin_addr);
		printf("-- Client message -- Server : %s:%d\n", server_IP, newPort);
		Sendto(sockfd, "portack", 7, 0, (SA *)&servaddr, servlen); 

		/*Timeout for response from server - this means "ack" is yet to reach the server*/
		if (Readable_timeo(sockfd, 5) == 0){
			Sendto(sockfd, "ack", 3, 0, (SA *)&servaddr, servlen);
		}
		else {
			while(1){
				n = Read(sockfd, recvline, MAXLINE);
				recvline[n] = 0;
				for (j = 0; recvline[j] != '\0'; j++) tempStr[j] = recvline[j];
				if (splitThisString(tempStr) == 0) break;
				Fputs(recvline, stdout);
			}
		}
	printf("File download complete\n");
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(servPort);
	Inet_pton(AF_INET, server_IP, &servaddr.sin_addr);
	printf("Back to Server address : %s:%d\n", server_IP, servPort);
	}
}

int main(int argc, char **argv){
	int sockfd, n, clientportno, i=0, templen;
        char tempserv_address[MAXLINE + 1];
        struct sockaddr_in servaddr, *sa, tempaddr;
	const int on = 1;
	struct ifi_info *ifi, *ifihead;

      	bzero(&servaddr, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
	int portno = atoi(argv[2]);
        servaddr.sin_port = htons(portno);
	Inet_pton(AF_INET, argv[1],  &servaddr.sin_addr);

//	srand(atoi(argv[3]));
	clientportno = getClientPort();

	for (ifihead = ifi = (struct ifi_info *)Get_ifi_info(AF_INET, 1); ifi != NULL; ifi = ifi->ifi_next) {
		sockfd = Socket(AF_INET, SOCK_DGRAM, 0);
		Setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
		sa = (struct sockaddr_in *) ifi->ifi_addr;
		sa->sin_family = AF_INET;
		sa->sin_port = htons(clientportno);
		if(i==0){
			i++;
			continue;
		}
		Bind(sockfd, (SA *) sa, sizeof(*sa));
		printf("-- Client message -- Bound %s\n", Sock_ntop((SA *) sa, sizeof(*sa)));
		break;
	}

	if (getsockname(sockfd, (SA *)&tempaddr, &templen) < 0) printf("getsockname error\n");
	Inet_ntop(AF_INET, &tempaddr, tempserv_address, MAXLINE);
	clientportno = ntohs(tempaddr.sin_port);
//	printf("-- Client message -- Bound %s:%d\n", tempserv_address, clientportno);

	ddg_cli(stdin, sockfd, (SA *)&servaddr, sizeof(servaddr), (char *)argv[1], (int)stringToInteger(argv[2]));

	exit(0);
}
