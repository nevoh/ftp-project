#include <sys/types.h>

#include <stdio.h>

#include <stdlib.h>

#include <string.h>

#include <dirent.h>

#include<unistd.h>

/* Socket API headers */ #include <sys/socket.h>

#include <netinet/in.h>

#include <arpa/inet.h>

/* Definations */
#define DEFAULT_BUFLEN 512

#define SIZE 1024
struct users {
  char * file_username;
  char * file_password;
};
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
int is_authenticated(int fd, char uFile[]) {
  char recvbuf[DEFAULT_BUFLEN], username[DEFAULT_BUFLEN], password[DEFAULT_BUFLEN];
  char * recvstring = (char * ) recvbuf;
  int recvbuflen = DEFAULT_BUFLEN, rcnt, access_value = 0;
  int count = 0, i = 0;

  rcnt = recv(fd, recvbuf, recvbuflen, 0);
  do {
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
      FILE * fp = NULL;
      int i = 0;

      struct users
      var = {
        NULL,
        NULL
      };
      char line[SIZE] = {
        0
      }, * ptr = NULL;
      //	for password thingy, remember                                              
      if (NULL == (fp = fopen(uFile, "r"))) {
        perror("Error opening the file.\n");
        exit(EXIT_FAILURE);
      }
      var.file_username = malloc(SIZE);
      var.file_password = malloc(SIZE);
      while (EOF != fscanf(fp, "%s", line)) {
        ptr = strtok(line, ":");
        var.file_username = ptr;

        while (NULL != (ptr = strtok(NULL, ":"))) {
          i++;
          if (i == 1)
            var.file_password = ptr;
        }
        i = 0;
        if (strcmp(var.file_username, username) == 0 && strcmp(var.file_password, password) == 0)
          access_value = 1;
      }

    } else if (rcnt < 0) {
      printf("Send failed:\n");
      close(fd);
    }
    return access_value;
  } while (rcnt > 0);
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
        strcpy(errormsg, "404  not found\n");
        insertString(errormsg, recvfilename, 5);
        send(fd, errormsg, strlen(errormsg), 0);
      } else {
        send_file(fp, fd);
        memset(recvcmd, 0, strlen(recvcmd));
      }
      if (rcnt < 0) {
        printf("Send failed:\n");
        close(fd);
        break;
      }
    } else if (rcnt > 0 && strcmp(recvcmd, "DEL") == 0) {
      if (remove(recvfilename) == 0) {
        strcpy(errormsg, "200  deleted\n");
        insertString(errormsg, recvfilename, 5);
        send(fd, errormsg, strlen(errormsg), 0);
      } else {
        strcpy(errormsg, "404  not found\n");
        insertString(errormsg, recvfilename, 5);
        send(fd, errormsg, strlen(errormsg), 0);
      }
    } else if (rcnt > 0 && strcmp(recvcmd, "QUIT") == 0) {
      strcpy(errormsg, "Goodbye!\n");
      send(fd, errormsg, strlen(errormsg), 0);
      close(fd);
      printf("Child finished their job!\n");
    } else {
      strcpy(errormsg, "404 Enter a valid command\n");
      send(fd, errormsg, strlen(errormsg), 0);
    }

  } while (rcnt > 0);
}

int main() {
  char cmd[100], d[10], dir[100], p[10], u[10], uFile[100];
  unsigned int port;

  do {
    scanf("%s %s %s %s %u %s %s", & cmd, & d, & dir, & p, & port, & u, & uFile);
    if (strcmp(cmd, "server") == 0) {
      if (strcmp(d, "-d") == 0) {
        if (chdir(dir) != 0) {
          perror("Directory change failed");
          continue;
        }
      } else {
        printf("%s not recognized\n", d);
        continue;
      }
      if (strcmp(p, "-p") != 0) {
        printf("%s not recognized\n", p);
        continue;
      }

      if (strcmp(u, "-u") == 0) {
        if (access(uFile, F_OK) == 0) {
          break;
        } else {
          perror("Users file not found");
          continue;
        }
      } else {
        printf("%s not recognized\n", u);
        continue;
      }
      if (strcmp(d, "-d") == 0 && strcmp(p, "-p") == 0 && strcmp(u, "-u") == 0)
        break;
    } else {
      printf("%s not recognized\n", cmd);
      continue;
    }
  } while (1);

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
  local_addr.sin_port = htons(port);

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
  printf("Concurrent  socket server now starting on port %d\n", port);
  printf("Wait for connection\n");

  while (1) { // main accept() loop
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
      if (is_authenticated(fd, uFile)) {
        send(fd, "200 User neville Granted access\n", 32, 0);
        do_job(fd);
      } else
        send(fd, "400 User not found\n", 19, 0);
      //      printf("Child finished their job!\n");
      //      close(fd);
      exit(0);
    }
  }
  close(server);
}
