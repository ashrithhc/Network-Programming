CSE533 Fall 2017 HW#1
Due October 5, 2017 before lecture (11:29AM)

Goal
TCP socket client/server programming using I/O multiplexing, child processes and threads. 

Required Reading
Chapters 1-6
Pipes
Threads (Chap 26.1-26.5)

Client
./client ipaddressORhostname echoport timeport
Use gethostbyaddress or gethostbyname and print the inverse to stdout
Infinitely accept on stdin commands “echo” and “time”, until “quit” is entered. All other inputs result in reprompt.
When “echo” or “time” is entered, the client forks a child process to exec the proper service for the user in a separate xterm window.
EG. execlp(“exterm”, “xterm”, “-e” “./echocli”, “127.0.0.1”, “300”, (char*) 0)
EG. execlp(“exterm”, “xterm”, “-e” “./timecli”, “127.0.0.1”, “400”, (char*) 0)
The child process will use a half-duplex pipe to communicate with the parent.  Note: xterm will close instantaneously upon child termination (normal or abnormal). Use this pipe to communicate status information, at a minimum for:
Connection established/disconnected with server
SIGCHILD 
Abnormal termination conditions 
The parent process will infinitely read and print any status messages from the child process to stdout until the pipe is closed or SIGCHILD is received. 
./echocli ipaddress echoport
Similar to Fig 6.9 or 6.13 
Creates a TCP connection to server based on ipaddress and port
User types on stdin. Upon enter, it is sent to server, which echoes the input back. 
Must use I/O multiplexing (select or poll) between stdin and server socket. Use two different colors (ANSI escape sequences) to display user text vs text received from the server.
To terminate the connection use ^D (CTRL-D, EOF char)
./timecli ipaddress timeport
Creates a TCP connection to server based on ipaddress and port
Infinitely reads from server socket and displays on stdout of xterm.
To terminate the connection use ^C (CTRL-C) - Note: equivalent to “crash” of the client program from the server’s perspective. Server must handle this correctly and close cleanly.
Client code MUST be robust to
Server crashing (Chap 5.12 & 5.13)
EINTR errors during slow system calls (eg. parent reading from pipe or printing to stdout) due to SIGCHLD.
Child processes (Xterm) crashing
What else??

Server
./server ipaddressOrHostname echoport timeport
Handles 2 types of services: echo and time
echo: standard echo seen in class
time: modified daytime service
Infinite loop, send daytime, sleep 5 seconds, and repeat
Server creates listening socket for each service on specified port
Use select or poll to I/O multiplex on client connections. Upon accepting the connection, create a thread to provide the specified service (thread should detach). Main server thread waits for more connections from clients.
Make sure to use thread-safe functions (specific concern for readline library)
Use provided Makefile to link with thread-safe version of readline from ~cse53/Stevens/umpv13e_solaris2.10/threads for server only
Server code MUST be robust to errors and threads terminate cleanly under all circumstances (no ZOMBIES). Server thread should print to stdout a message with appropriate details of termination, like “Time Client 127.0.0.1 termination: EPIPE error detected” or “Echo Client 130.245.182.10 termination: socket read returned 0”. Minimal cases to consider:
^C (CTRL C) from either client looks like a crash (5.11 to 5.13)
EG. Time server will get EPIPE
“Orphaned” sockets - never close socket if thread terminates without closing

Logistics (Will post to PIAZZA when ready)
Login with your CS Dept account to gitlab03.cs.stonybrook.edu. You will receive an email with your gitlab repo account information. Clone your repo to compserv machine. Create a “HW1” directory to contain all your code and your Makefile.

To submit your assignment for grading, tag your final commit with “HW1” before the deadline. You do not have permissions in gitlab to reset or move your commit tag once it is set.  YOU ONLY GET 1 CHANCE TO TAG! Make sure all your files are committed and push to the gitlab server before you tag. 

We will login to a compserv machine, clone your repo, and run make with your HW1/Makefile. If we can not run executables called client and server, you will get a 0. 

We will test your code by running clients and server between the compserv1-compserv4 machines, which you have access to in order to test your code.

