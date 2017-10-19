#include "unp.h"

void printInverse(char *inp){
	int i;
	struct hostent *hostinfo;
	char *address, *hostname;

	if( (hostinfo = gethostbyname(inp))== NULL) {
		herror("gethostbyname");
		exit(0);
	}

	struct in_addr **addr_list;
	addr_list = (struct in_addr **)hostinfo->h_addr_list;
	address = inet_ntoa(*addr_list[0]);
	hostname = hostinfo->h_name;
	
	if(strcmp(hostname, address)){
		printf("%s\n", address);
		return;
	}

	struct in_addr ipaddress;
	if (0 == (inet_aton(inp, &ipaddress))){
		printf("Invalid address\n");
		exit(0);
	}
	if( (hostinfo = gethostbyaddr(&ipaddress, sizeof(ipaddress), AF_INET)) == NULL) {
		herror("gethostbyaddr");
		exit(0);
	}

	addr_list = (struct in_addr **)hostinfo->h_addr_list;
	address = inet_ntoa(*addr_list[0]);
	hostname = hostinfo->h_name;

	printf("%s\n", hostname);

	return;
}

int main(int argc, char **argv){

	if(argc == 1) err_quit("Not the right usage");

	printInverse(argv[1]);

	char requestLine[MAXLINE + 1];
	pid_t pid;
	int i;
	int pipefd[2];
	while( Fgets(requestLine, MAXLINE, stdin) != NULL){
		requestLine[strcspn(requestLine, "\n")] = '\0';
		if (!strcmp(requestLine, "quit")) exit(1);
		if(!strcmp(requestLine, "echo") || !strcmp(requestLine, "time")){
			pipe(pipefd);
			pid = fork();
			if(pid == 0){
				Close(pipefd[0]);
				if(!strcmp(requestLine, "echo")){
					 i = execlp("xterm", "xterm", "-e", "./echo_cli", argv[1], argv[2], (char *)0);
				}
				else if(!strcmp(requestLine, "time")){
					i = execlp("xterm", "xterm", "-e", "./time_cli", argv[1], argv[3], (char *)0);
				}
				else if(!strcmp(requestLine, "quit")){
					exit(0);
				}
				Close(pipefd[1]);						
			}
			if(pid > 0) {
				Close(pipefd[1]);
				char buff[MAXLINE];
				while (read(pipefd[0], &buff, MAXLINE) > 0) Fputs(buff, stdout);
				int status;
				waitpid(pid, &status, 0);
				Close(pipefd[0]);
			}
		}
	}	

	exit(0);

}
