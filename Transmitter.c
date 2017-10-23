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
		int res = write(fd,message,sizeof_message);
		if(res<0){
			printf("Cannot write atende\n");
		}
	}
}

int stateMachineTransmissor(unsigned char controlByte){
	int state=0;
	char supervisionPacket[5] = {FLAG, A, controlByte, A^controlByte, FLAG};
	char buf;

	while(state != 5){
		read(fd,&buf,1);
		//	printf("buf: %x\n ", buf);
		//	printf("state: %d\n",state);
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
	return 0;
}

unsigned char *readFrameIConfirmations(unsigned int *length){

	unsigned char buf;
	unsigned char c_info;
	unsigned int size=0;
	unsigned char *finalBuf=(unsigned char*)malloc(size);
	int state = 0;
	int res;
	while (state!=5) {

		res= read(fd, &buf, 1);
		if(res > 0){
			switch(state){
				case 0:
				if(buf==FLAG)
				state=1;
				break;
				case 1:
				if(buf==A)
				state=2;
				else if(buf == FLAG)
				state=1;
				else
				state=0;
				break;
				case 2:
				if((buf== RR(0)) || (buf == RR(1))){
					C1=buf;
					c_info=buf;
					state=3;
				}
				else if (buf == FLAG)
				state=1;
				else
				state=0;
				break;
				case 3:
				if(buf==(A^c_info))
				state=4;
				else
				state=0;
				break;
				case 4:
				if(buf==FLAG)
				state=5;
				break;
			}
			finalBuf[size]=buf;
			size+=1;
			finalBuf=(unsigned char*)realloc(finalBuf,size);

		}
		else{
			printf("Else RES:%d\n", res);
			return finalBuf;
		}
	}
	printf("chegou:%d\n", res);
	*length=size;
	return finalBuf;

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
		printf("Cannot write llopen\n");
		return -1;
	}

	alarm(3);
	stateMachineTransmissor(C_UA);
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
void createHeader(unsigned char* frameI){

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

int llwrite(unsigned char* file_buffer){
	unsigned int frameI_length=0;

	int length=PACKET_SIZE;
	unsigned char *frameI = (unsigned char*)malloc(sizeof(unsigned char)*4);

	//add header F,A,C1,BCC1
	createHeader(frameI);
	frameI_length=4;
printf("entreii\n" );
	//add buffer to frameI
	frameI=realloc(frameI,frameI_length+length+2);
	memcpy(frameI+frameI_length,file_buffer,length);
	frameI_length+=length;
printf("entreii\n" );
	//add BCC2 to frameI
	memcpy(frameI+frameI_length,calculateBCC2(file_buffer,length),1);
	frameI_length+=2;
printf("entreii\n" );
	//add byteStuffing to frameI
	unsigned char *stuffing_array= byteStuffing(frameI+4,&frameI_length);
	memcpy(frameI+4,stuffing_array+4,frameI_length);

printf("entreii\n" );
	write(fd,frameI,frameI_length);
	memcpy(message, frameI, frameI_length);
	sizeof_message=frameI_length;

	printf("aqui\n");
	//alarm(3);
	//readFrameIConfirmations(&frameI_length);
	//alarm(0);

	printf("aqui22222222\n");
	return 0;
}

int main(int argc, char** argv)
{

	if ( (argc < 3) ||
	((strcmp("/dev/tnt0", argv[1])!=0) &&
	(strcmp("/dev/tnt1", argv[1])!=0) )) {
		printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/tnt0 filename \n");
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

	llopen();
	unsigned char * aux_buf=(unsigned char*)malloc(sizeof(unsigned char)*PACKET_SIZE);
	int size=0;
	int count=0;
	while (size <= fsize) {
		memcpy(aux_buf,&buf[size],PACKET_SIZE);
		for(int i=0; i < PACKET_SIZE;i++)
			printf("buf: %x\n", aux_buf[i]);

		int resultado = llwrite(aux_buf);

		printf("Resultado %d.\n", resultado);
		printf("AQUIII\n");
		count++;
		printf("Numero de vez: %d", count);
		size+=PACKET_SIZE;
	}

	fclose(file);

	if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
		perror("tcsetattr");
		exit(-1);
	}

	close(fd);

	return 0;
}
