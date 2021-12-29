#include <sys/types.h>

#include <stdio.h>

#include <stdlib.h>

#include <string.h>

#include <dirent.h>

/* Socket API headers */
#include <sys/socket.h>

#include <netinet/in.h>

#include <arpa/inet.h>

/* Definations */
#define DEFAULT_BUFLEN 512
#define PORT 2672
#define SIZE 1024

char * safe_strcpy(char * dest, size_t size, char * src) {
  if (size > 0) {
    size_t i;
    for (i = 0; i < size && src[i]; i++) {
      dest[i] = src[i];
    }
    dest[i] = '\0';
  }
  return dest;
}

void insertString(char str1[], char str2[], int position) {
  int str1Size = strlen(str1);
  int str2Size = strlen(str2);
  int newSize = str2Size + str1Size + 1;
  int cppPosition = position - 1;
  int i = 0;

  for (i = str1Size; i >= cppPosition; i--) str1[i + str2Size] = str1[i];
  for (i = 0; i < str2Size; i++) str1[cppPosition + i] = str2[i];
  str1[newSize] = '\0';
}

void send_file(FILE * fp, int sockfd) {
  char content[SIZE] = {
    0
  };

  while (fgets(content, SIZE, fp) != NULL) {
    if (send(sockfd, content, sizeof(content), 0) == -1) {
      perror("!! Error in sending file content");
      exit(1);
    }
    bzero(content, SIZE);
  }
}
void list_files(int fd) {
  DIR * d;
  struct dirent * dir;
  d = opendir(".");
  if (d) {
    while ((dir = readdir(d)) != NULL) {
      if (dir -> d_type == DT_REG) {
        send(fd, dir -> d_name, strlen(dir -> d_name), 0);
        send(fd, "\n", 2, 0);
      }
    }
    closedir(d);
  }
}
int is_authenticated(int fd) {
  char recvbuf[DEFAULT_BUFLEN], username[DEFAULT_BUFLEN], password[DEFAULT_BUFLEN];
  char * recvstring = (char * ) recvbuf;
  int recvbuflen = DEFAULT_BUFLEN, rcnt, access_value = 0;
  int count = 0, i = 0;

  rcnt = recv(fd, recvbuf, recvbuflen, 0);

  while (count < strlen(recvstring)) {
    if (isspace(recvstring[count]))
      break;
    count++;
  }

  char recvcmd[4];
  char recvfilename[DEFAULT_BUFLEN];

  safe_strcpy(recvcmd, count, recvstring);
  safe_strcpy(recvfilename, strlen(recvstring) - (count + 2), recvstring + count + 1);

  while (i < strlen(recvfilename)) {
    if (isspace(recvfilename[i]))
      break;
    i++;
  }

  safe_strcpy(username, i, recvfilename);
  safe_strcpy(password, strlen(recvfilename) - i, recvfilename + (i + 1));

  if (rcnt > 0 && strcmp(recvcmd, "USER") == 0) {
    if (strcmp(username, "neville") == 0 && strcmp(password, "1234") == 0)
      access_value = 1;

  } else if (rcnt < 0) {
    printf("Send failed:\n");
    close(fd);
  } else if (rcnt == 0)
    printf("Connection closing...\n");
  else {
    printf("Receive failed:\n");
    close(fd);
  }

  return access_value;
}

void do_job(int fd) {
  int length, rcnt, condition;
  char recvbuf[DEFAULT_BUFLEN], bmsg[DEFAULT_BUFLEN];
  int recvbuflen = DEFAULT_BUFLEN, x;
  char errormsg[512];
  FILE * fp;
  char username[DEFAULT_BUFLEN];
  char password[DEFAULT_BUFLEN];

  do {
    memset(recvbuf, 0, strlen(recvbuf));
    rcnt = recv(fd, recvbuf, recvbuflen, 0);
    char * recvstring = (char * ) recvbuf;
    int count = 0, i = 0;
    while (count < strlen(recvstring)) {
      if (isspace(recvstring[count]))
        break;
      count++;
    }
    char recvcmd[4], recvcmdGET[3];
    char recvfilename[DEFAULT_BUFLEN];
    safe_strcpy(recvcmd, count, recvstring);
    safe_strcpy(recvfilename, strlen(recvstring) - (count + 2), recvstring + count + 1);

    if (rcnt > 0 && strcmp(recvcmd, "LIST") == 0) {
      list_files(fd);
      memset(recvcmd, 0, strlen(recvcmd));
    } else if (rcnt > 0 && strcmp(recvcmd, "GET") == 0) {
      fp = fopen(recvfilename, "r");
      if (fp == NULL) {
        strcpy(errormsg, "404  not found");
        insertString(errormsg, recvfilename, 5);
        printf(recvfilename);
        send(fd, errormsg, strlen(errormsg), 0);
        perror("!! Error in reading file.");
        exit(1);
      }
      send_file(fp, fd);

      if (rcnt < 0) {
        printf("Send failed:\n");
        close(fd);
        break;
      }
    } else if (rcnt > 0 && strcmp(recvcmd, "DEL") == 0) {
      if (remove(recvfilename)==0){
      	strcpy(errormsg, "200  deleted\n");
        insertString(errormsg, recvfilename, 5);
        send(fd, errormsg, strlen(errormsg), 0);
	  }
	  else{
	  	strcpy(errormsg, "404  not found\n");
        insertString(errormsg, recvfilename, 5);
        send(fd, errormsg, strlen(errormsg), 0);
	  }
      

    } else if (rcnt == 0)
      printf("Connection closing...\n");
    else {
      printf("Receive failed:\n");
      close(fd);
      break;
    }
  } while (rcnt > 0);
}

int main() {
  int server, client;
  struct sockaddr_in local_addr;
  struct sockaddr_in remote_addr;
  int length, fd, rcnt, optval;
  char welcomemsg[DEFAULT_BUFLEN] = "Welcome to Neville's server'\n";
  pid_t pid;

  /* Open socket descriptor */
  if ((server = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("Can't create socket!");
    return (1);
  }

  /* Fill local and remote address structure with zero */
  memset( & local_addr, 0, sizeof(local_addr));
  memset( & remote_addr, 0, sizeof(remote_addr));

  /* Set values to local_addr structure */
  local_addr.sin_family = AF_INET;
  local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  local_addr.sin_port = htons(PORT);

  // set SO_REUSEADDR on a socket to true (1):
  optval = 1;
  setsockopt(server, SOL_SOCKET, SO_REUSEADDR, & optval, sizeof optval);

  if (bind(server, (struct sockaddr * ) & local_addr, sizeof(local_addr)) < 0) {
    /* could not start server */
    perror("Bind error");
    return (1);
  }

  if (listen(server, SOMAXCONN) < 0) {
    perror("listen");
    exit(1);
  }

  printf("Concurrent  socket server now starting on port %d\n", PORT);
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
    if ((pid = fork()) == 0) {
      close(server);
      send(fd, welcomemsg, sizeof(welcomemsg), 0);
      if (is_authenticated(fd)) {
        send(fd, "200 User neville Granted access\n", 32, 0);
        do_job(fd);
      } else
        send(fd, "400 User not found", 18, 0);
      printf("Child finished their job!\n");
      close(fd);
      exit(0);
    }

  }

  // Final Cleanup
  close(server);

}
