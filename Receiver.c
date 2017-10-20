/*Non-Canonical Input Processing*/
#include "Utilities.h"

volatile int STOP=FALSE;
struct termios oldtio,newtio;
FILE *file;

void stateMachineReceiver(int fd,unsigned char UA[5])
{
	unsigned char foo;
	int state = 0;
	while(state!=5){

		read(fd, &foo, 1);
		switch(state){
			case 0:
			if(foo==FLAG) state = 1;
			break;
			case 1:
			if(foo==FLAG) state = 1;
			if(foo==A) state = 2;
			else state = 0;
			break;
			case 2:
			if(foo==FLAG) state = 1;
			if(foo==C_SET) state = 3;
			else state = 0;
			break;
			case 3:
			if(foo==FLAG) state = 1;
			if(!A^C_SET) state = 4;
			else state = 0;
			break;
			case 4:
			if(foo==FLAG) state = 5;
			else state = 0;
			break;
		}
	}

		printf("SET received successfully!\n");
		write(fd, UA, 5);
}
unsigned char *readFrameI(int fd,int * length){
	unsigned char *foo;
	int size=1;
	unsigned char *buf=(unsigned char*)malloc(size);
	int state = 0;
	while(state!=5){

		read(fd, &foo, 1);
		switch(state){
			case 0:
			if(foo==FLAG) state = 1;
			break;
			case 1:
			if(foo==FLAG) state = 1;
			if(foo==A) state = 2;
			else state = 0;
			break;
			case 2:
			if(foo==FLAG) state = 1;
			if(foo==C_SET) state = 3;
			else state = 0;
			break;
			case 3:
			if(foo==FLAG) state = 1;
			if(!A^C_SET) state = 4; //devia ser foo==A^C_SET
			else state = 0;
			break;
			case 4:
			if(foo==FLAG) state = 5;
			else state = 0;
			break;
		}
		buf[size-1]=foo;
		size+=1;
		realloc(buf,size);
}
			*length=size;
			return buf;
	}

unsigned char *byteDestuffing(unsigned char  *buf, int sizeBuf){

	unsigned char *newBuf=(unsigned char*)malloc(sizeBuf);
	unsigned char *finalBuf;
	int countSize_newBuf=0;
	int i,j,k;
	k=4;
	j=5;
	for(i=0;i < sizeof(buf);i++)
	{
		if(buf[k]==0x7d && buf[j]==0x5e){
			newBuf[i]=0x7e;
			countSize_newBuf++;
		}
		else if(buf[k]==0x7d && buf[j]==0x5d){
			newBuf[i]=0x7d;
			countSize_newBuf++;
		}

		else {
			newBuf[i]=buf[k];
			countSize_newBuf++;
		}
		k++;
		j++;
	}

	finalBuf= (unsigned char*)malloc(sizeof(countSize_newBuf));
	memcpy(finalBuf,newBuf,countSize_newBuf);
	free(newBuf);

	return finalBuf;

}
void verifyBCC1(unsigned char  *buf,int fd){

int size;

while(1){
	if(buf[1]^buf[2]!=buf[3])
		*buf=readFrameI(fd,&size);
	else{
		*buf=byteDestuffing(buf,size);
		return;
		}
	}
}

int verifyBCC2(unsigned char *buf, int size){
	unsigned char BCC2;
	unsigned char BCC2_XOR=0;

		if(buf[size-1] == FLAG){
			BCC2 = buf[size-2];
	 }

	 for(int i=4;i < size;i++)
		 BCC2_XOR ^= buf[i];

	if(BCC2 == BCC2_XOR)
		return 1;
	else
		return 0;
}

void sendRRorREJ(int fd,unsigned char *buf){
	if(verifyBCC2(buf,fd)){
		if(buf[2] == 0x00){
			write(fd,RR(1),1);
		}
		else if(buf[2] == 0x40){
			write(fd,RR(0),1);
		}
	}
	else{
		if(buf[2] == 0x00){
			write(fd,REJ(0),1);
		}
		else if(buf[2] == 0x40){
			write(fd,REJ(1),1);
		}
	}
}
int llread(int fd){
	int size;
	unsigned char* buf= readFrameI(fd, &size);
		while (1) {
			verifyBCC1(&buf,fd);
			sendRRorREJ(fd,buf);
		}

	}
int llopen(int fd){
	unsigned char UA[5];
	UA[0] = FLAG;
	UA[1] = A;
	UA[2] = C_SET;
	UA[3] = UA[1]^UA[2];
	UA[4] = FLAG;

	stateMachineReceiver(fd,UA);
}
int main(int argc, char** argv)
{
	int fd,c, res;
	char buf[255];
	buf[254] = '\0';

	if ( (argc < 3) ||
	((strcmp("/dev/ttyS0", argv[1])!=0) &&
	(strcmp("/dev/ttyS1", argv[1])!=0) )) {
		printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
		printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1 filename \n");
		exit(1);
	}



		struct stat st;
		stat(argv[2], &st);
		int size = st.st_size;

		file = fopen(argv[2],"wb");
	/*
	Open serial port device for reading and writing and not as controlling tty
	because we don't want to get killed if linenoise sends CTRL-C.
	*/


	fd = open(argv[1], O_RDWR | O_NOCTTY );
	if (fd <0) {perror(argv[1]); exit(-1); }

	if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
		perror("tcgetattr");
		exit(-1);
	}

	bzero(&newtio, sizeof(newtio));
	newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag = 0;

	/* set input mode (non-canonical, no echo,...) */
	newtio.c_lflag = 0;

	newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
	newtio.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */

	/*
	VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
	leitura do(s) pr�ximo(s) caracter(es)
	*/

	tcflush(fd, TCIOFLUSH);

	if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
		perror("tcsetattr");
		exit(-1);
	}

	printf("New termios structure set\n");
	llopen(fd);

	sleep(3);


	tcsetattr(fd,TCSANOW,&oldtio);
	close(fd);
	return 0;
}
