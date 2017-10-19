#include "unp.h"

int main(int argc, char **argv){

	int sockfd, n;
	char recvline[MAXLINE + 1];
	struct sockaddr_in servaddr;

	sockfd = Socket(AF_INET, SOCK_STREAM, 0);
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(6111);

	if (inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr) <= 0) err_quit("inet_pton error for %s", argv[1]);

	if (connect(sockfd, (SA *) &servaddr, sizeof(servaddr)) < 0) err_sys("connect error");
	
	while (1){
		n = read(sockfd, recvline, MAXLINE);
		recvline[n] = 0;
		if (fputs(recvline, stdout) == EOF) err_sys("fputs error");
		bzero(recvline, MAXLINE);
	}

	if (n<0) err_sys("Read error");

	exit(0);
}
