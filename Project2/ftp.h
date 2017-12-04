#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <strings.h>
#include <string.h>

#define SERVER_PORT 6000
#define SERVER_ADDR "192.168.28.96"
#define STRING_SIZE 200

typedef struct{
  char *user;
  char *password;
  char *host;
  char *path;
  char *file;
  char *ip;
}url_info;

typedef struct{
	int data;
	int control;
	
}sockets;

int parseURL(char *path, url_info *info);

int connectFTP(const char *ip, int port);

int loginFTP(const char* user, const char* password, sockets* ftp);

int getIpByHost(url_info* url);

int readFtpReply(int control, char* str, size_t size);

int passiveMode(sockets* ftp);

int copyFileFTP(const char* filename, sockets* ftp);