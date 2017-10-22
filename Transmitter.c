#include "Utilities.h"

volatile int STOP=FALSE;
//tries alarm
int conta_alarm = 1;
struct termios oldtio,newtio;
FILE *file;
int filesize;
unsigned char C1=0x40;
unsigned char *message;
int sizeof_message;

void atende()                   // atende alarme
{
	printf("alarme # %d\n", conta_alarm);
	conta_alarm++;

	if(conta_alarm == 4)
		exit(-1);
	else{
		int res= write(fd,message,sizeof_message);
		if(res<0){
				printf("Cannot write\n");
		}
	}
}

int stateMachineTransmissor(unsigned char controlByte){
	int state=0;
	char supervisionPacket[5] = {FLAG, A, controlByte, A^controlByte, FLAG};
	char buf[1];
	int res;

	while(state != 5){

		res =read(fd,buf,1);
		if(res>0)
		{
			switch(state){
				case 0:
				if(buf[0]==supervisionPacket[0])
					state=1;
				break;
				case 1:
				if(buf[0]==supervisionPacket[1])
					state=2;
				else if(buf[0] == supervisionPacket[0])
					state=1;
				else
					state=0;
				break;
				case 2:
				if(buf[0]==supervisionPacket[2])
					state=3;
				else if (buf[0] == supervisionPacket[0])
					state=1;
				else
					state=0;
				break;
				case 3:
				if(buf[0]==supervisionPacket[3])
					state=4;
				else
					state=0;
				break;
				case 4:
				if(buf[0]==supervisionPacket[4])
					state=5;
				else
					state=0;
				break;
			};
		}
		else
		return -1;
	}
	return 0;
}

int llopen()
{

	int res;
	unsigned char SET[5] = {FLAG, A, C, A^C, FLAG};

		res = write(fd, SET, 5);
		message=SET;
		sizeof_message=5;
		if(res < 0){
			printf("Cannot write\n");
			return -1;
		}

		alarm(3);
		int success = stateMachineTransmissor(C_UA);

		if(success < 0){
			printf("message wasn't sent\n");
		}
		else
		alarm(0);

		return 0;
}

void readPacket_Application(unsigned char *packet,int packetSize){
	int counter=0;
	unsigned char *fileData;

	fileData=(unsigned char*)malloc(packetSize+4);
	fileData[0]=0x01;
	fileData[1]=counter;
	fileData[2]=(getFileSize(file)-fileData[3])/256;
	fileData[3]=getFileSize(file)-256*fileData[2];
	printf("Antes do memcpy\n");
	memcpy(fileData+4,packet,packetSize);
	printf("No readPacket_Application\n");
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

int detectedFrameIConfirmations(){

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

int llwrite(){
	unsigned char packet[260];
	unsigned char *frameI, buffer[1000];
	int res;
	unsigned int size=0;

	while(!EOF){
		memcpy(buffer, connectionLayer(packet,&size), size);
		frameI = (unsigned char*)malloc(size);
		memcpy(frameI, buffer, size);
		res=write(fd,frameI,size);
}
return res;
}

int main(int argc, char** argv)
{
	(void)signal(SIGALRM, atende);  // instala  rotina que atende interrupcao

	int c,length,res;

	int i;

	if ( (argc < 3) ||
	((strcmp("/dev/ttyS0", argv[1])!=0) &&
	(strcmp("/dev/ttyS1", argv[1])!=0) )) {
		printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1 filename \n");
		exit(1);
	}

file = fopen(argv[2],"rb");
if(file < 0){
	printf("Could not open file to be sent\n");
	exit(-1);
}

int fsize = getFileSize(file);
printf("size of file: %d\n", fsize);
unsigned char* buf = (unsigned char*)malloc(fsize);
fread(buf,sizeof(unsigned char),fsize,file);
printf("fread feito\n");
readPacket_Application(buf,fsize);
printf("Antes llopen\n");
llopen();
printf("llopen feito \n");
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

fclose(file);

if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
	perror("tcsetattr");
	exit(-1);
}

close(fd);
return 0;
}
