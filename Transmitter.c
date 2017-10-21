/*Non-Canonical Input Processing*/
#include "Utilities.h"

volatile int STOP=FALSE;
//tries alarm
int conta_alarm = 1;
int flag_alarm = 0;
struct termios oldtio,newtio;
FILE *file;
int filesize;
unsigned char C1=0x40;

void atende()                   // atende alarme
{
	printf("alarme # %d\n", conta_alarm);
	flag_alarm=1;
	conta_alarm++;
}

	int stateMachineTransmissor(int fd, unsigned char controlByte){
		int state=0;
		char supervisionPacket[5] = {FLAG, A, controlByte, A^controlByte, FLAG};
		char buf[1];
		int errorflag = 0;

			while(state !=5 && STOP == FALSE){

				int res =read(fd,buf,1);
				if(res>0)
				{
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
				else
					return -1;
			}
	}

	int llopen(int fd)
	{
		int res;
		while(conta_alarm < 4){
			unsigned char SET[5] = {FLAG, A, C, A^C, FLAG};

			res = write(fd, SET, 5);

			if(res < 0){
				printf("Cannot write\n");
				return -1;
			}

			alarm(3);

			while(!flag_alarm && STOP == FALSE){
				unsigned char controlByte = C_UA;
				int success  = stateMachineTransmissor(controlByte, fd);
				return success;
			}

			if(STOP == TRUE){
				alarm(0);
				conta_alarm = 0;
				STOP = FALSE;
				flag_alarm = 0;
				return 0;
			}

			else
				flag_alarm = 0;
		}

	}


		stateMachineTransmissor(fd,SET);

		if(flag_continue==1)
		return 0;
		else
		return -1;
	}

	void readPacket_Application(int fd,unsigned char *packet,int packetSize){
		int counter=0;
		unsigned char *fileData[256];

		packet=(unsigned char*)malloc(packetSize+4);
		packet[0]=0x01;
		packet[1]=counter;
		packet[2]=(getFileSize(file)-packet[3])/256;
		packet[3]=getFileSize(file)-256*packet[2];

		read(fd, fileData, packetSize);
		memcpy(packet,fileData,packetSize);
	}

	unsigned char *connectionLayer(unsigned char* fileData, unsigned int *newSize){
		unsigned char BCC2;
		int k,j;

		*newSize = (*newSize)+SIZE_CONNECTION_LAYER;
		unsigned char size= *newSize;

		int i;

		for(i=0; i < size;i++)	{

			if(fileData[i]==0x7E  || fileData[i]==0x7D)
				(*newSize)++;
		}

		BCC2=fileData[0]^fileData[1];

		for(i=2;i < *newSize;i++)
			BCC2= BCC2 ^ fileData[i];

		//allocation of BCC2
		unsigned char *frameI= (unsigned char*)malloc(*newSize);


		frameI= (unsigned char*)malloc(*newSize+5);
		frameI[0]=FLAG;
		frameI[1]=A;
		frameI[2]=C1;
		frameI[3]=A^C1;
		//byte frameIing
		k=4;
		j=5;
		for(i=0;i < *newSize;i++)
		{
			if(fileData[i]==0x7E){
				frameI[k]=0x7D;
				frameI[j]=0x5E;
			}
			else if(fileData[i]==0x7D){
				frameI[k]=0X7D;
				frameI[j]=0x5D;
			}

			if(fileData[i] !=0x7E || fileData[i] != 0x7D){
				frameI[k]=fileData[i];
			}
			k++;
			j++;
		}

		frameI[(*newSize)-2]=BCC2;
		frameI[(*newSize)-1]=FLAG;

		return frameI;
	}

	int detectedFrameIConfirmations(int fd){

		char buf[5];
		read(fd,buf,5);

		//verify FLAG
		if(buf[0] != 0x7e)
		return -1;
		//verify A
		if(buf[1] != 0x03)
		return -1;
		//verify C1
		if(buf[2] != 0x01){
				if((buf[1]^buf[2]) != buf[3]){
					return -1;
					}
				if(buf[4] != 0x7e)
					return -1;

		return -1;

		}
	}

	int llwrite(int fd){
		unsigned char packet[260];
		unsigned char *frameI, buffer[1000];
		int res;
		unsigned int size=0;

		while(!EOF){
			readPacket_Application(fd,packet,sizeof(packet));
			memcpy(buffer, connectionLayer(packet,&size), size);
			frameI = (unsigned char*)malloc(size);
			memcpy(frameI, buffer, size);
			res=write(fd,frameI,size);
			/*
			if(res==0 || res==-1){
			printf("%d bytes written\n",res);
			return res;
		}
		*/

		/*while(){

		}*/
	}
	return res;
}

int transmission(int fd){
		int size = getFileSize(file);
		int counter, finish, read, res, res2 = 0;

		while(finish == 0){
			if(counter >= 3)
				return -1;
			counter++;

			res2 = 0;

			res = llopen(fd);

			if(res < 0)
				return -1;

			res2 = llwrite(fd);

			if(res2 < size)
				continue;
			else
				finish = 1;
		}

		return 0;
}

int main(int argc, char** argv)
{
	(void)signal(SIGALRM, atende);  // instala  rotina que atende interrupcao

	int fd,c,length,res;
	char buf[255];

	int i, sum = 0, speed = 0;

	if ( (argc < 3) ||
	((strcmp("/dev/ttyS0", argv[1])!=0) &&
	(strcmp("/dev/ttyS1", argv[1])!=0) )) {
		printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1 filename \n");
		exit(1);
	}
/*
	unsigned int test_int = 23;
	unsigned char* test_buf = "isto } e um teste ~ dois";
	printf("vai entrar layer\n");
	test_buf = connectionLayer(test_buf,&test_int);
	for (size_t i = 0; i < 30; i++) {
		printf("buf stuffed: %x\n",test_buf[i]);
	}
	return 0;
*/

	file = fopen(argv[2],"rb");

	/*
	Open serial port device for reading and writing and not as controlling tty
	because we don't want to get killed if linenoise sends CTRL-C.
	*/

	fd = open(argv[1], O_RDWR | O_NOCTTY | O_NONBLOCK);
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
	newtio.c_cc[VMIN]     = 5;   /* blocking read until 5 chars received */


	/*
	VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
	leitura do(s) próximo(s) caracter(es)
	*/

	tcflush(fd, TCIOFLUSH);

	if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
		perror("tcsetattr");
		exit(-1);
	}

	printf("New termios structure set\n");

	transmission(fd);
	fclose(file);
	
	if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
		perror("tcsetattr");
		exit(-1);
	}

	close(fd);
	return 0;
}
