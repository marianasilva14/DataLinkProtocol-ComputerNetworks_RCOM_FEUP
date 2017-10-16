
typedef struct{
  int fd;
  int status;
} ApplicationLayer;

extern ApplicationLayer * app;

int initApp(char * port, int status);
