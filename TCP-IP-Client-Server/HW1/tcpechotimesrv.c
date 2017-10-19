#include "unp.h"
#include <time.h>

void echoService(void *args){
	int n;
	char recvline[MAXLINE + 1];
	int connfd = (*(int *)args);

	while( (n = read(connfd, recvline, MAXLINE)) > 0){
		Writen(connfd, recvline, MAXLINE);
		bzero(recvline, MAXLINE);
	}
	if (pthread_detach(pthread_self()) > 0){
		printf("Pthread errored on detach");
		exit(0);
	}
}

void timeService(void *args){
	char buff[MAXLINE];
	time_t ticks;
	int connfd = (*(int *)args);	

	while(1){
		ticks = time(NULL);
		snprintf(buff, sizeof(buff), "%.24s\r\n", ctime(&ticks));
		Writen(connfd, buff, MAXLINE);
		sleep(5);
	}
	if (pthread_detach(pthread_self()) > 0){
		printf("Pthread errored on detach");
		exit(0);
	}
}

void sig_chld(int signo){
	pid_t pid;
	int stat;

	while ((pid = waitpid(-1, &stat, WNOHANG )) > 0)
		printf("child is terminated\n");
	return;
}

int main(int argc, char **argv){
	int listenfd, connfd, inp_ret, listenfd_time, connfd_time;
	struct sockaddr_in servaddr, cliaddr, servaddr_time, cliaddr_time;
	char buff[MAXLINE];
	time_t ticks;

	listenfd = Socket(AF_INET, SOCK_STREAM, 0);
	listenfd_time = Socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(6000);

	bzero(&servaddr_time, sizeof(servaddr_time));
	servaddr_time.sin_family = AF_INET;
	servaddr_time.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr_time.sin_port = htons(6111);

	Bind(listenfd, (SA *) &servaddr, sizeof(servaddr));
	Bind(listenfd_time, (SA *) &servaddr_time, sizeof(servaddr_time));

	Listen(listenfd, LISTENQ);
	Listen(listenfd_time, listenfd);

	Signal(SIGCHLD, sig_chld);

	pid_t pid;
	int len, len_t, maxfd;
	fd_set rset;
	FD_ZERO(&rset);

//	FD_SET(listenfd, &rset);
//	FD_SET(listenfd_time, &rset);
//	maxfd = max(listenfd, listenfd_time) + 1;

	for( ; ; ){
		FD_SET(listenfd, &rset);
		FD_SET(listenfd_time, &rset);
		maxfd = max(listenfd, listenfd_time) + 1;
		Select(maxfd, &rset, NULL, NULL, NULL);

		if (FD_ISSET(listenfd_time, &rset)){

			len = sizeof(cliaddr);
			connfd = Accept(listenfd_time, (SA *) &cliaddr, &len);
			if (connfd < 0){
				if (errno == EINTR) continue;
				else err_sys("accept error");
			}
			else printf("Time Client connected\n");

			pthread_t tid;
			pthread_create(&tid, NULL, timeService, &connfd);

/*			pid = fork();
			if (pid > 0){
				Close(connfd);
				int statusl;
				waitpid(pid, &statusl, 0);
			}

			else if (pid == 0){
				Close(listenfd_time);
				timeService(connfd);
				Close(connfd);
			}
*/		}

		if (FD_ISSET(listenfd, &rset)){

			len = sizeof(cliaddr);
			connfd = Accept(listenfd, (SA *) &cliaddr, &len);
			if (connfd < 0){
				if (errno == EINTR) continue;
				else err_sys("accept error");
			}
			else printf("Echo client connected\n");

			pthread_t tid;
			pthread_create(&tid, NULL, echoService, &connfd);

/*			pid = fork();
			if(pid > 0){
				Close(connfd);
				int status;
				waitpid(pid, &status, 0);
			}
			else if (pid == 0){
				Close(listenfd);
				echoService(connfd);
				Close(connfd);
			}
*/		}
	} 
}
