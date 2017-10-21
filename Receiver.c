/*Non-Canonical Input Processing*/
#include "Utilities.h"

volatile int STOP=FALSE;
struct termios oldtio,newtio;
FILE *file;

int stateMachineReceiver(int fd,unsigned char controlByte)
{
	char supervisionPacket[5] = {FLAG, A, controlByte, A^controlByte, FLAG};
	int state = 0;
	int res=0;
	char buf[1];
	int errorflag = 0;

	while(STOP==FALSE){
		res= read(fd, buf, 1);
		if(res<0)
		return -1;

			switch(state){
				case 0:
				if(buf[0]!=supervisionPacket[0])
				errorflag=-1;
				break;
				case 1:
				if(buf[0]!=supervisionPacket[1])
				errorflag=-1;
				break;
				case 2:
				if(buf[0]!=supervisionPacket[2])
				errorflag=-1;
				break;
				case 3:
				if(buf[0]!=supervisionPacket[3])
				errorflag=-1;
				break;
				case 4:
				if(buf[0]!=supervisionPacket[4])
				errorflag=-1;
				break;
			};
			state++;

		if(state == 5 && errorflag == 0){
			STOP = TRUE;
			return 0;
		}
	}

	return -1;
}

unsigned char *readFrameI(int fd,int * length,unsigned char controlByte){
	char supervisionPacket[5] = {FLAG, A, controlByte, A^controlByte, FLAG};
	unsigned int size=1;
	unsigned char *finalBuf=(unsigned char*)malloc(size);
	unsigned char *buf=(unsigned char*)malloc(sizeof(unsigned char)*1);
	int state = 0;
	int errorflag=0;
	while(state!=5){

		read(fd, &buf, 1);
		switch(state){
			case 0:
			if(buf[0]!=supervisionPacket[0])
				errorflag=-1;
			break;
			case 1:
			if(buf[0]!=supervisionPacket[1])
				errorflag=-1;
			break;
			case 2:
			if(buf[0]!= supervisionPacket[2])
				errorflag=-1;
			break;
			case 3:
			if(buf[0]!=supervisionPacket[3])
				errorflag=-1;
			break;
			case 4:
			if(buf[0]!=supervisionPacket[4])
				errorflag=-1;
			break;
		}
		finalBuf[size-1]=buf[0];
		size+=1;
		finalBuf=(unsigned char*)realloc(finalBuf,size);
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
		if(buf[1]^buf[2]!=buf[3]){
					//*buf=readFrameI(fd,&size);
		}

		else{
			buf=byteDestuffing(buf,size);
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

unsigned char *completSupervisionPacket(unsigned char controlByte){

	unsigned char *supervisionPacket=(unsigned char*)malloc(sizeof(unsigned char)*5);
	supervisionPacket[0]=FLAG;
	supervisionPacket[1]=A;
	supervisionPacket[2]=controlByte;
	supervisionPacket[3]=A^controlByte;
	supervisionPacket[4]=FLAG;

	return supervisionPacket;
}

void sendRRorREJ(int fd,unsigned char *buf){

	unsigned char * supervisionPacket;

	if(verifyBCC2(buf,fd)){
		if(buf[2] == 0x00){
			supervisionPacket= completSupervisionPacket(RR(1));
			write(fd,supervisionPacket,1);
		}
		else if(buf[2] == 0x40){
			supervisionPacket= completSupervisionPacket(RR(0));
			write(fd,supervisionPacket,1);
		}
	}
	else{
		if(buf[2] == 0x00){
			supervisionPacket= completSupervisionPacket(REJ(0));
			write(fd,supervisionPacket,1);
		}
		else if(buf[2] == 0x40){
			supervisionPacket= completSupervisionPacket(REJ(1));
			write(fd,supervisionPacket,1);
		}
	}
}
int llread(int fd){
	int size;
		//unsigned char* buf= readFrameI(fd, &size);
	while (1) {
		//verifyBCC1(&buf,fd);
		//sendRRorREJ(fd,buf);
	}

}
int llopen(int fd){
	unsigned char UA[5];
	UA[0] = FLAG;
	UA[1] = A;
	UA[2] = C_UA;
	UA[3] = UA[1]^UA[2];
	UA[4] = FLAG;

	int k=stateMachineReceiver(fd,UA[2]);

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
