#include "unp.h"

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#define STDIN 0

void sig_pipe(int signo){
	printf("Server is down, cya soon\n");
}

int main(int argc, char **argv){

	int sockfd, n;
	char recvline[MAXLINE + 1], outputStr[MAXLINE + 1];
	struct sockaddr_in servaddr;
//	int writePipe = atoi(argv[3]);

	sockfd = Socket(AF_INET, SOCK_STREAM, 0);
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(6000);

	if (inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr) <= 0) err_quit("inet_pton error for %s", argv[1]);

	if (connect(sockfd, (SA *) &servaddr, sizeof(servaddr)) < 0) err_sys("connect error");
//	write(writePipe, "Echo client connected to server", MAXLINE);

	Signal(SIGPIPE, sig_pipe);

	char sendline[MAXLINE + 1];
	int maxfd;
	fd_set rset;
	FD_ZERO(&rset);
	while(1){
		FD_SET(STDIN, &rset);
		FD_SET(sockfd, &rset);
		maxfd = max(STDIN, sockfd) + 1;
		Select(maxfd, &rset, NULL, NULL, NULL);

		if (FD_ISSET(STDIN, &rset)){
			Fgets(sendline, MAXLINE, stdin);
			Writen(sockfd, sendline, MAXLINE); 
			bzero(sendline, MAXLINE+1);
		}
		if (FD_ISSET(sockfd, &rset)){
			bzero(recvline, MAXLINE+1);
			bzero(outputStr, MAXLINE+1);
			if (read(sockfd, recvline, MAXLINE) == 0) err_quit("str-cli: server terminated prematurely");
			strcat(outputStr, ANSI_COLOR_RED);
			strcat(outputStr, recvline);
			strcat(outputStr, ANSI_COLOR_RESET);
			Fputs(outputStr, stdout);
		}
	}

	exit(0);
}
