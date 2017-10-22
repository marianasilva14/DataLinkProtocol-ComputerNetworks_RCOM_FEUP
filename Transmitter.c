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
	char buf;
	int res;

	while(state != 5){
		res = read(fd,&buf,1);
		printf("depois do read \n");
		if(res > 0)
		{
			printf("antes do switch \n");
			switch(state){
				case 0:
				if(buf==supervisionPacket[0])
					state=1;
				break;
				case 1:
				if(buf==supervisionPacket[1])
					state=2;
				else if(buf == supervisionPacket[0])
					state=1;
				else
					state=0;
				break;
				case 2:
				if(buf==supervisionPacket[2])
					state=3;
				else if (buf == supervisionPacket[0])
					state=1;
				else
					state=0;
				break;
				case 3:
				if(buf==supervisionPacket[3])
					state=4;
				else
					state=0;
				break;
				case 4:
				if(buf==supervisionPacket[4])
					state=5;
				else
					state=0;
				break;
			}
		}
		else
			continue;
	}
	return 0;
}

int llopen()
{
	(void)signal(SIGALRM, atende);  // instala  rotina que atende interrupcao
	int res;
	unsigned char SET[5] = {FLAG, A, C, A^C, FLAG};

		res = write(fd, SET, 5);
		message=SET;
		sizeof_message=5;
		if(res < 0){
			printf("Cannot write\n");
			return -1;
		}
		printf("antes do alarme \n");
		alarm(3);
		printf("antes da maquina \n");
		stateMachineTransmissor(C_UA);
			printf("depois da maquina \n");
		alarm(0);

		return 0;
}

unsigned char* readPacket_Application(unsigned char *packet,int packetSize){
	int counter=0;
	unsigned char *fileData;
	printf("estou aqui\n");
	fileData=(unsigned char*)malloc(packetSize+4);
	fileData[0]=0x01;
	fileData[1]=counter;
	fileData[2]=packetSize/256;
	fileData[3]=packetSize%256;
	printf("Antes do memcpy\n");
	memcpy(fileData+4,packet,packetSize);
	printf("No readPacket_Application\n");

	return fileData;
}
void addHeader(unsigned char* frameI, int sizeBuf){
	//allocation of BCC2

	frameI= realloc(frameI,sizeBuf+4);
	frameI[0]=FLAG;
	frameI[1]=A;
	frameI[2]=C1;
	frameI[3]=A^C1;

}
unsigned char *calculateBCC2(unsigned char* fileData, unsigned int newSize){
unsigned char BCC2=0;

unsigned char* tail= malloc(1);
int i;

for(i=0;i < newSize;i++)
BCC2= BCC2 ^ fileData[i];

tail[0]=BCC2;

return tail;

}
unsigned char * byteStuffing(unsigned char* fileData, unsigned int *newSize){

	int k,j,i;
	unsigned int size = *newSize;

	for(i=0; i < size;i++)	{

		if((fileData[i]==0x7E)  || (fileData[i]==0x7D))
		(*newSize)++;
	}


	unsigned char* frameI=(unsigned char*)malloc(*newSize+1);

	//byte frameIing
	k=4;
	j=5;
	for(i=0;i < *newSize-5;i++)
	{
		if(fileData[i]==0x7E){
			frameI[k]=0x7D;
			frameI[j]=0x5E;
			k++;
			j++;
		}
		else if(fileData[i]==0x7D){
			frameI[k]=0x7D;
			frameI[j]=0x5D;
			k++;
			j++;
		}

		else if(fileData[i] !=0x7E || fileData[i] != 0x7D){
			frameI[k]=fileData[i];
		}
		k++;
		j++;
	}

frameI[*newSize-1]=FLAG;
	return frameI;
}
/*
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
}*/

int llwrite(unsigned char*  buffer,int length){
	int res=0;
	unsigned int frameI_length=0;
	unsigned char* frameI= malloc(0);

	//add header F,A,C1,BCC1
	addHeader(frameI,frameI_length);
	frameI_length+=4;

	//add buffer to frameI
	frameI=realloc(frameI,frameI_length+length+2);
	memcpy(frameI+frameI_length,buffer,length);
	frameI_length+=length;

	//add BCC2 to frameI
	memcpy(frameI+frameI_length,calculateBCC2(buffer,length),2);
	frameI_length+=2;

	//add byteStuffing to frameI
	unsigned char *stuffing_array= byteStuffing(frameI+4,&frameI_length);
	memcpy(frameI+4,stuffing_array+4,frameI_length);

	res = write(fd,frameI,frameI_length);

return res;
}

int main(int argc, char** argv)
{

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

printf("Antes llopen\n");
llopen();
printf("llopen feito \n");
int res= llwrite(buf,fsize);
printf("write feito:%d \n",res);

fclose(file);

if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
	perror("tcsetattr");
	exit(-1);
}

close(fd);

return 0;
}
