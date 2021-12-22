 // Online C compiler to run C program online
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main() {
     void *recvbuf = "USER neville 1234";
    
    
    char *recvstring =(char*)recvbuf;
    int count = 0 , i=0;
    while (count < strlen(recvstring)){
        if(isspace(recvstring[count]))
        break;
        count++;
    }
    // printf("%d", count);
    
    
    char cmd[4] = "GET";
    // printf("%lu\n", strlen(cmd));
    char recvcmd[256];
    char recvfilename[256];
    strncpy(recvcmd, recvstring, count);
    strncpy(recvfilename, recvstring+count+1, strlen(recvstring)-(count+1));
    char username[256];
    char password[256];
    while (i < strlen(recvfilename)){
        if(isspace(recvfilename[i]))
        break;
        i++;
    }
    
    strncpy(username, recvfilename, i);
    
  
    strncpy(password, recvfilename+(i+1), strlen(recvfilename)-(i+3));
    printf("%d", strcmp(password, "1234"));
	//printf("%s", password);
    
    
    
    return 0;
}
