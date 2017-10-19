/*Non-Canonical Input Processing*/
#include "Utilities.h"

volatile int STOP=FALSE;
int flag_alarm=0, conta_alarm=0, flag_continue = 0;
struct termios oldtio,newtio;
FILE *file;
int filesize;
unsigned char C1=0x40;

void atende()                   // atende alarme
{
	printf("alarme # %d\n", conta_alarm);
	flag_alarm=1;
	conta_alarm++;
	if(conta_alarm==3){
		printf("Exiting...");
		exit(-1);}
	}

	void stateMachineTransmissor(int fd,unsigned char SET[5]){
		(void)signal(SIGALRM, atende);  // instala  rotina que atende interrupcao
		int state=0;
		unsigned char foo;
		int bytes;

		while(!flag_continue){

			bytes = write(fd,SET,5);
			alarm(3);
			flag_alarm = 0;
			while(state!=5 && !flag_alarm){

				int res =read(fd, &foo,1);
				if(res>0)
				{
					switch(state){

						case 0: if(foo==FLAG)
						state=1;
						break;
						case 1: if(foo==FLAG)
						state=1;
						if(foo==A)
						state=2;
						else
						state=0;
						break;
						case 2:	if(foo==FLAG) state=1;
						if(foo==C_SET) state=3;
						else state=0;
						break;
						case 3: if(foo==FLAG) state=1;
						if(!A^C_SET) state=4;
						else state=0;
						break;
						case 4: if(foo==FLAG) {
							state=5;
							flag_continue = 1;
							alarm(0);
						}
						else state=0;
						break;
						default: continue;
					}
				}
			}
		}
	}


	int llopen(int fd)
	{
		unsigned char SET[5];
		SET[0]=FLAG;
		SET[1]=A;
		SET[2]=C_SET;
		SET[3]=SET[1]^SET[2];
		SET[4]=FLAG;

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
		*newSize = sizeof(fileData)+SIZE_CONNECTION_LAYER;
		int i;
		for(i=0; i < sizeof(fileData);i++)	{
			if(fileData[i]==0x7e  || fileData[i]==0x7d)
			newSize++;
		}

		BCC2=fileData[0]^fileData[1];

		for(i=2;i < sizeof(fileData);i++)
		BCC2= BCC2 ^ fileData[i];

		//allocation of BCC2
		unsigned char *frameI= (unsigned char*)malloc(*newSize);


		frameI= (unsigned char*)malloc(sizeof(fileData)+5);
		frameI[0]=FLAG;
		frameI[1]=A;
		frameI[2]=C1;
		frameI[3]=A^C1;
		//byte frameIing
		k=4;
		j=5;
		for(i=0;i < sizeof(fileData);i++)
		{
			if(fileData[i]==0x7e){
				frameI[k]=0x7d;
				frameI[j]=0x5e;
			}
			else if(fileData[i]==0x7d){
				frameI[k]=0x7d;
				frameI[j]=0x5d;
			}

			if(fileData[i] !=0x7e || fileData[i] != 0x7d){
				frameI[k]=fileData[i];
			}
			k++;
			j++;
		}

		frameI[*newSize-2]=BCC2;
		frameI[*newSize-1]=FLAG;

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
int main(int argc, char** argv)
{
	int fd,c,length,res;
	char buf[255];

	int i, sum = 0, speed = 0;

	if ( (argc < 3) ||
	((strcmp("/dev/ttyS0", argv[1])!=0) &&
	(strcmp("/dev/ttyS1", argv[1])!=0) )) {
		printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1 filename \n");
		exit(1);
	}

	struct stat st;
	stat(argv[2], &st);
	int size = st.st_size;

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
	llopen(fd);

	res=llwrite(fd);

	fclose(file);
	if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
		perror("tcsetattr");
		exit(-1);
	}

	close(fd);
	return 0;
}
