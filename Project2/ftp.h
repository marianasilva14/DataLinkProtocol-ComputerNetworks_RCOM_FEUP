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
}url_info;

int parseURL(char *path, url_info *info);

int connect(const char *ip, int port);
