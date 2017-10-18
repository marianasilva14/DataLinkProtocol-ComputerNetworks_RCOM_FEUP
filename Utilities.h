
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>
#include <stdio.h>

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FLAG 0x7e
#define A 0x03
#define C_SET 0x03
#define FALSE 0
#define TRUE 1
#define SIZE_CONNECTION_LAYER 6
#define SIZE_FRAME_I 9

typedef struct{
	char* fileArray;
	int fileSize;
} FileStruct;
