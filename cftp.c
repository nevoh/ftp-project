#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Socket API headers */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* Definations */
#define DEFAULT_BUFLEN 512
#define PORT 2672
#define SIZE 1024

//function for sending requested file contents to client
void send_file(FILE *fp, int sockfd)
{
    char content[SIZE] = {0};

    while (fgets(content, SIZE, fp) != NULL)
    {
        if (send(sockfd, content, sizeof(content), 0) == -1)
        {
            perror("!! Error in sending file content");
            exit(1);
        }
        bzero(content, SIZE);
    }
}

void do_job(int fd) {
int length,rcnt, condition;
char recvbuf[DEFAULT_BUFLEN],bmsg[DEFAULT_BUFLEN];
int  recvbuflen = DEFAULT_BUFLEN;
char welcomemsg[DEFAULT_BUFLEN] = "Welcome to Neville's server'\n";
char cmd[DEFAULT_BUFLEN] = "GET";
FILE *fp;
char *filename = "file.txt";

    send(fd, welcomemsg, sizeof(welcomemsg), 0);
     do {
        rcnt = recv(fd, recvbuf, recvbuflen, 0);
        char *recvstring =(char*)recvbuf;
        int count = 0, i;
	    while (count < strlen(recvstring)){
	        if(isspace(recvstring[count]))
	        break;
	        count++;
	    }
	    char recvcmd[DEFAULT_BUFLEN];
	    char recvfilename[DEFAULT_BUFLEN];
	    strncpy(recvcmd, recvstring, count);
	    strncpy(recvfilename, (recvstring+count+1), strlen(recvstring)-(count+2));
        if (rcnt > 0 && (*recvcmd == *cmd ) ) {
        	fp = fopen(recvfilename, "r");
		    if (fp == NULL)
		    {
			printf("%d", strlen(recvstring));
		        perror("!! Error in reading file.");
		        exit(1);
		    }
		    send_file(fp, fd);
            if (rcnt < 0) {
                printf("Send failed:\n");
                close(fd);
                break;
            }
        }
        else if (rcnt == 0)
            printf("Connection closing...\n");
        else  {
            printf("Receive failed:\n");
            close(fd);
            break;
        }
    } while (rcnt > 0);
}



int main()
{
int server, client;
struct sockaddr_in local_addr;
struct sockaddr_in remote_addr;
int length,fd,rcnt,optval;
pid_t pid;

/* Open socket descriptor */
if ((server = socket( AF_INET, SOCK_STREAM, 0)) < 0 ) { 
    perror("Can't create socket!");
    return(1);
}


/* Fill local and remote address structure with zero */
memset( &local_addr, 0, sizeof(local_addr) );
memset( &remote_addr, 0, sizeof(remote_addr) );

/* Set values to local_addr structure */
local_addr.sin_family = AF_INET;
local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
local_addr.sin_port = htons(PORT);

// set SO_REUSEADDR on a socket to true (1):
optval = 1;
setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);

if ( bind( server, (struct sockaddr *)&local_addr, sizeof(local_addr) ) < 0 )
{
    /* could not start server */
    perror("Bind error");
    return(1);
}

if ( listen( server, SOMAXCONN ) < 0 ) {
        perror("listen");
        exit(1);
}

printf("Concurrent  socket server now starting on port %d\n",PORT);
printf("Wait for connection\n");

while(1) {  // main accept() loop
    length = sizeof remote_addr;
    if ((fd = accept(server, (struct sockaddr *)&remote_addr, \
          &length)) == -1) {
          perror("Accept Problem!");
          continue;
    }
    

    printf("Server: got connection from %s\n", \
            inet_ntoa(remote_addr.sin_addr));

    /* If fork create Child, take control over child and close on server side */
    if ((pid=fork()) == 0) {
        close(server);
        do_job(fd);
        printf("Child finished their job!\n");
        close(fd);
        exit(0);
    }

}

// Final Cleanup
close(server);

}
