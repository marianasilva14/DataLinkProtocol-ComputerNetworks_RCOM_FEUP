#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>
#include <stdio.h>

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FLAG 0x7e
#define A 0x03
#define C_SET 0x03

#define FALSE 0
#define TRUE 1

int getMode(){
  int mode = -1;

  while(mode != 1 && mode != 2){
    printf("Are you the receiver or the sender? \n");
    printf("Receiver - 1\n");
    printf("Sender - 2\n");
    scanf("%d", &mode);
  }

  return mode;
}

char * getPort(){
  int port = -1;

  while(port != 1 && port != 2){
    printf("Wat port?\n\n");
    printf("/dev/ttyS0 - 1\n");
    printf("/dev/ttyS1 - 2\n");
    scanf("%d", &port);
  }
  if(port==1)
    return "/dev/ttyS0";
  else
    return "/dev/ttyS1";
}

int main(int argc, char** argv){

  if(argc > 1){
    printf("ERROR: This program doesn't have arguments");
    exit(-1);
  }

  int mode = getMode();
}
