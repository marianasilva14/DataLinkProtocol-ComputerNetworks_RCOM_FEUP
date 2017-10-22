#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FLAG 0x7e
#define A 0x03
#define C 0x03
#define C_UA 0x07
#define C_SET 0x03
#define FALSE 0
#define TRUE 1
#define SIZE_CONNECTION_LAYER 6
#define SIZE_FRAME_I 9
#define RR(x) (0b00000101 + ((x) << 7))
#define REJ(y) (0b00000001 + ((y) << 7))
#define C_INFO(x) (0b00000000 + ((x) << 6))


int fd;


int getFileSize(FILE* file) {
	// saving current position
	long int currentPosition = ftell(file);

	// seeking end of file
	if (fseek(file, 0, SEEK_END) == -1) {
		printf("ERROR: Could not get file size.\n");
		return -1;
	}

	// saving file size
	long int size = ftell(file);

	// seeking to the previously saved position
	fseek(file, 0, currentPosition);

	// returning size
	return size;
}
