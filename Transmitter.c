#include "Utilities.h"

int counter_alarm = 1;
struct termios oldtio,newtio;
int filesize;
unsigned char C1=0x40;
int switch_C1=1;
unsigned char * message;
int sizeof_message;
char *filename;
unsigned char* initial_buf;
int fsize;
FILE *file;

/**
* Handler the alarm
*/
void handler_alarm()
{
	printf("alarm # %d\n", counter_alarm);
	counter_alarm++;

	if(counter_alarm == 4)
	exit(-1);
	else{
		int res = write(fd,message,sizeof_message);
		if(res<0){
			printf("Cannot write handler_alarm\n");
		}
	}
}

/**
* State machine used to read the information received
* @param controlByte
* @return zero when finished
*/
int stateMachineTransmissor(unsigned char controlByte){
	int state=0;
	char supervisionPacket[5] = {FLAG, A, controlByte, A^controlByte, FLAG};
	char buf;

	while(state != 5){
		read(fd,&buf,1);
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

/**
* State machine used to read the supervision packet received
* @param length size of read buffer
* @return supervision packet
*/
unsigned char *readFrameIConfirmations(unsigned int *length){

	unsigned char buf;
	unsigned char c_info;
	unsigned int size=0;
	unsigned char *finalBuf;
	unsigned char *final=(unsigned char*)malloc(10000);
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
				if((buf== RR(0)) || (buf == RR(1))  ||  (buf== REJ(0))|| (buf == REJ(1))){
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
			memcpy(final+size,&buf,1);
			size+=1;
		}
		else
			continue;
	}
	finalBuf=(unsigned char*)malloc(size);
	memcpy(finalBuf,final,size);
	*length=size;
	return finalBuf;

}

/**
* Connection establishment where the set is sent and the UA received
* @return zero when finished
*/
int llopen()
{
	(void)signal(SIGALRM, handler_alarm);
	int res;
	unsigned char SET[5] = {FLAG, A, C, A^C, FLAG};

	res = write(fd, SET, 5);
	memcpy(message,SET,5);
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

/**
* Add packet application to the read packet
* @param packet read packet
* @param packetSize size of read packet
* @return packet with the application packet
*/
unsigned char* addApplication_Packet(unsigned char *packet,int packetSize){
	unsigned char c_info;
	unsigned char *fileData;
	if(switch_C1==0)
		c_info=C_INFO(0);
	else
		c_info=C_INFO(1);

	fileData=(unsigned char*)malloc(packetSize+4);
	fileData[0]=0x01;
	fileData[1]=c_info;
	fileData[2]=packetSize/256;
	fileData[3]=packetSize%256;

	memcpy(fileData+4,packet,packetSize);

	return fileData;
}

/**
* Create header
* @param frameI frame
* @param counter variable used to know the value of C1
*/
void createHeader(unsigned char* frameI,int counter){

	if(counter==0)
		C1=C_INFO(0);
	else
		C1=C_INFO(1);

	frameI[0]=FLAG;
	frameI[1]=A;
	frameI[2]=C1;
	frameI[3]=A^C1;

}

/**
* Function that calculates BCC2
* @param fileData data packet
* @param size size of data packet
* @return an array with the value of bcc2
*/
unsigned char *calculateBCC2(unsigned char* fileData, unsigned int size){
	unsigned char BCC2=0;

	unsigned char* tail= malloc(1);
	int i;

	for(i=0;i < size;i++)
	BCC2= BCC2 ^ fileData[i];

	tail[0]=BCC2;

	return tail;

}

/**
* Function that does stuffing to the received data packet
* @param fileData data packet
* @param newSize size of data packet
* @return data packet with the bytes stuffing
*/
unsigned char * byteStuffing(unsigned char* fileData, unsigned int *newSize){

	int k,j,i;
	unsigned int size = *newSize;

	for(i=0; i < size;i++)	{

		if((fileData[i]==0x7E)  || (fileData[i]==0x7D))
		(*newSize)++;
	}

	unsigned char* frameI=(unsigned char*)malloc(*newSize+1);

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

/**
* Function that switches C1
* @param answers supervision packet
* @return 1 if can read the next packet
*/
int readAnswers(unsigned char *answers){

	int answer=0;
	if(answers[2]==RR(0)){
		switch_C1=0;
		answer=1;
	}
	else if(answers[2]==RR(1)){
		switch_C1=1;
		answer=1;
	}

	else if(answers[2]==REJ(0)){
		switch_C1=1;
		answer=0;
	}
	else{
		switch_C1=0;
		answer=0;
	}
	return answer;
}

/**
* Create frame start or end
* @param controlByte controlByte of frame start or end
* @return frame start or end
*/
unsigned char* creatFrameI_START_OR_END(unsigned char controlByte){
	unsigned char fsize_1part= DIVIDE_SIZE_1PART(24);
	unsigned char fsize_2part=DIVIDE_SIZE_2PART(16);
	unsigned char fsize_3part=DIVIDE_SIZE_3PART(8);
	unsigned char fsize_4part=DIVIDE_SIZE_4PART(0);

	int start_size=9+strlen(filename);
	unsigned char* start=(unsigned char*)malloc(start_size);

	int i;
	start[0]=controlByte;
	start[1]=0x00;
	start[2]=sizeof(int);
	start[3]=fsize_1part;
	start[4]=fsize_2part;
	start[5]=fsize_3part;
	start[6]=fsize_4part;
	start[7]=0x01;
	start[8]=strlen(filename);
	for(i=0; i < strlen(filename);i++){
		start[9+i]=filename[i];
	}

	return start;
}

/**
* Main cycle of transfer of data transfer. Deals with sending the frames to the receiver
* @param file_buffer data packet received
* @param length size of data packet
* @return zero when finished
*/
int llwrite(unsigned char* file_buffer,int length){
	unsigned int frameI_length=0;

	int canReadNextPacket=0;
	while(!canReadNextPacket){
		unsigned char *frameI = (unsigned char*)malloc(sizeof(unsigned char)*4);
		int i;
		//add header F,A,C1,BCC1
		createHeader(frameI,switch_C1);
		frameI_length=4;

		//add buffer to frameI
		frameI=realloc(frameI,frameI_length+length+2);
		memcpy(frameI+frameI_length,file_buffer,length);
		frameI_length+=length;

		//add BCC2 to frameI
		memcpy(frameI+frameI_length,calculateBCC2(file_buffer,length),1);
		frameI_length+=2;

		//add byteStuffing to frameI
		unsigned char *stuffing_array= byteStuffing(frameI+4,&frameI_length);
		memcpy(frameI+4,stuffing_array+4,frameI_length);

		alarm(3);
		write(fd,frameI,frameI_length);
		memcpy(message,frameI,frameI_length);
		sizeof_message=frameI_length;

		unsigned char* confirmations= readFrameIConfirmations(&frameI_length);

		alarm(0);

		if(readAnswers(confirmations))
		canReadNextPacket=1;
	}
	return 0;
}

/**
* Sends the DISC,receive the DISC and send the UA. In the end, close the program
* @return zero when finished
*/
int llclose()
{
	int res;
	unsigned char DISC[5] = {FLAG, A, C_DISC, A^C_DISC, FLAG};

	res = write(fd, DISC, 5);
	memcpy(message,DISC,5);
	sizeof_message=5;
	if(res < 0){
		printf("Cannot write llclose\n");
		return -1;
	}

	alarm(3);
	stateMachineTransmissor(C_DISC);
	alarm(0);

	unsigned char UA[5] = {FLAG, A, C_UA, A^C_UA, FLAG};

	res = write(fd, UA, 5);
	memcpy(message,DISC,5);
	sizeof_message=5;
	if(res < 0){
		printf("Cannot write llclose\n");
		return -1;
	}

	return 0;
}
/**
* Cycle responsible for sending frames
*/
void sendFrames(){
	unsigned char * aux_buf=(unsigned char*)malloc(sizeof(unsigned char)*PACKET_SIZE);
	int size=0;
	int count=0;
	int frameI_size=9+strlen(filename);
	llwrite(creatFrameI_START_OR_END(frameI_START),frameI_size);
	while (size <= fsize) {
		memcpy(aux_buf,&initial_buf[size],PACKET_SIZE);

		unsigned char *data = addApplication_Packet(aux_buf, PACKET_SIZE);
		llwrite(data,PACKET_SIZE+4);

		count++;
		size+=PACKET_SIZE;
	}
	llwrite(creatFrameI_START_OR_END(frameI_END),frameI_size);
}

int main(int argc, char** argv)
{

	message = malloc(sizeof(unsigned char)*266);
	if ( (argc < 3) ||
	((strcmp("/dev/ttyS0", argv[1])!=0) &&
	(strcmp("/dev/ttyS1", argv[1])!=0) )) {
		printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/tnt0 filename \n");
		exit(1);
	}

	file = fopen(argv[2],"rb");
	if(file < 0){
		printf("Could not open file to be sent\n");
		exit(-1);
	}

	fsize = getFileSize(file);
	filename=argv[2];
	initial_buf = (unsigned char*)malloc(fsize);
	fread(initial_buf,sizeof(unsigned char),fsize,file);

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
	sendFrames();
	llclose();
	fclose(file);

	if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
		perror("tcsetattr");
		exit(-1);
	}

	close(fd);

	return 0;
}
