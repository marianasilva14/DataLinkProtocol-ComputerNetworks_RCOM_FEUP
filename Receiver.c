#include "Utilities.h"

struct termios oldtio,newtio;
FILE *file;

/**
* State machine where SET is read that is sent by the transmitter, in the connection establishment phase
* @param controlByte
* @return 0 if the reading is doing
*/
int stateMachineReceiver(unsigned char controlByte)
{
	char supervisionPacket[5] = {FLAG, A, controlByte, A^controlByte, FLAG};
	int state = 0;
	int res=0;
	char buf;
	while (state!=5) {
		res= read(fd, &buf, 1);
		if(res > 0){
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

/**
* State machine where the frame sent by the sender is read in the file transfer stage
* @param length of array read
* @return array read
*/
unsigned char *readFrameI(int * length){

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
				if((buf== C_INFO(0)) || (buf == C_INFO(1))){
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
			size++;
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
* Function responsible for the inverse processor to the byte stuffing (transparency mechanism), so that the message is read correctly
* @param array received from the transmitter
* @param size of the array received from the transmitter
* @return array destuffed
*/
unsigned char *byteDestuffing(unsigned char *buf, int *sizeBuf){

	unsigned char *newBuf=(unsigned char*)malloc(*sizeBuf);
	unsigned char *finalBuf;
	int countSize_newBuf=0;
	int i,j,k;

	k=0;
	j=1;
		for(i=0;i < *sizeBuf;i++)
	{
		if(buf[k]==0x7d && buf[j]==0x5e){
			newBuf[countSize_newBuf]=0x7e;
			countSize_newBuf++;
			i++;
			k++;
			j++;
		}
		else if(buf[k]==0x7d && buf[j]==0x5d){
			newBuf[countSize_newBuf]=0x7d;
			countSize_newBuf++;
			i++;
			k++;
			j++;
		}
		else {
			newBuf[countSize_newBuf]=buf[k];
			countSize_newBuf++;
		}
		k++;
		j++;
	}

	*sizeBuf=countSize_newBuf;
	finalBuf= (unsigned char*)malloc(*sizeBuf);
	memcpy(finalBuf,newBuf,*sizeBuf);
	free(newBuf);

	return finalBuf;
}

/**
* Function that verifies if the BCC2 is correct and corresponds to the xor of the remaining elements except the header
* @param array received from the transmitter
* @param size of the array received from the transmitter
* @return returns 1 if BCC2 is correct, returns 0 otherwise
*/
int verifyBCC2(unsigned char *buf, int size){
	unsigned char BCC2;
	unsigned char BCC2_XOR=0;
	int i;

	if(buf[size-1] == FLAG){
		BCC2 = buf[size-2];
	}

	for(i=4;i < size-2;i++)
	BCC2_XOR ^= buf[i];

	if(BCC2 == BCC2_XOR)
		return 1;
	else
		return 0;
}

/**
*	Complete the response header given by the receiver
* @param control byte that can be RR (0), RR (1), REJ (0), REJ (1)
* @return array with header
*/
unsigned char *completeSupervisionPacket(unsigned char controlByte){

	unsigned char *supervisionPacket=(unsigned char*)malloc(sizeof(unsigned char)*5);
	supervisionPacket[0]=FLAG;
	supervisionPacket[1]=A;
	supervisionPacket[2]=controlByte;
	supervisionPacket[3]=A^controlByte;
	supervisionPacket[4]=FLAG;

	return supervisionPacket;
}

/**
* If BCC2 is correct, the receiver sends RR. If you receive from the transmitter a 0x00 sends an RR (1), if it receives a 0x40 it sends
* an RR (0). If it is wrong, the receiver sends a REJ. It is REJ (0) if you receive a 0x00 from the transmitter, REJ (1) if you receive
* a 0x40.
* @param array received from the transmitter
* @param size of the array received from the transmitter
*/
void sendRRorREJ(unsigned char *buf,int bufSize){

	unsigned char *supervisionPacket=(unsigned char*)malloc(sizeof(unsigned char)*5);

	if(verifyBCC2(buf,bufSize)){

		if(buf[2] == C_INFO(0)){
			memcpy(supervisionPacket, completeSupervisionPacket(RR(1)), 5);
			write(fd,supervisionPacket,5);

		}
		else if(buf[2] == C_INFO(1)){
			memcpy(supervisionPacket, completeSupervisionPacket(RR(0)), 5);
			write(fd,supervisionPacket,5);
		}
	}
	else{
		if(buf[2] == C_INFO(0)){
			memcpy(supervisionPacket, completeSupervisionPacket(REJ(0)), 5);
			write(fd,supervisionPacket,5);

		}
		else if(buf[2] == C_INFO(1)){
			memcpy(supervisionPacket, completeSupervisionPacket(REJ(1)), 5);
			write(fd,supervisionPacket,5);
		}
	}
}

/**
* Removes header to array received by transmitter
* @param array received from the transmitter
* @return array without header
*/
unsigned char* applicationPacket(unsigned char* buffer, int buffer_size){
	int i;

	int finalSize= buffer_size-SIZE_CONNECTION_LAYER;
	unsigned char* applicationPacket_aux = (unsigned char*)malloc(finalSize);
	int j=0;

	for(i=4; i < buffer_size-2;i++){
		applicationPacket_aux[j]=buffer[i];
		j++;
	}

	return applicationPacket_aux;
}

/**
*	Main function of transferring files on the receiver side, where it deals with the reception of the frames
* @param length of array read
* @return array read
*/
unsigned char* llread(int *packetSize){
	unsigned char *appPacket;
	unsigned char *buf;
	int size_buf=0;

	buf=readFrameI(&size_buf);

	unsigned char *buf2=(unsigned char*)malloc(size_buf);

	buf2=byteDestuffing(buf, &size_buf);

	memcpy(buf, buf2, size_buf);

	sendRRorREJ(buf,size_buf);

	appPacket= applicationPacket(buf,size_buf);
	*packetSize=size_buf - SIZE_CONNECTION_LAYER;

	return appPacket;
}

/**
*	Connection establishment where the UE is sent and received SET
* @return -1 if can't write UA
*/
int llopen(){
	unsigned char UA[5]={FLAG,A,C_UA,A^C_UA,FLAG};
	int res;

	stateMachineReceiver(C_SET);

	res=write(fd,UA,5);

	if(res<0){
		printf("Cannot write\n");
		return -1;
	}

	return 0;
}

/**
* Main cycle where the received array of the transmitter is processed, if it is identified the START frame is created and written the
* result file. If you receive the START frame, the file is closed.
*/
void createFile(){
	int i;
	int fsize=0;
	char *filename=malloc(0);
	FILE *file;
	while(1){
			unsigned char *appPacket;
			int packetSize=0;
			appPacket=llread(&packetSize);

			if(appPacket[0]==frameI_START){
				for(i=0;i<appPacket[2];i++){
					fsize+=appPacket[3+i];

				}
				for(i=0;i<appPacket[8];i++){
					filename[i]=appPacket[9+i];
				}
					file=fopen(filename,"wb");
			}
			else if(appPacket[0]==frameI_END){
					fclose(file);
					break;
			}
			else{
				fwrite(appPacket+4,sizeof(unsigned char),packetSize-4,file);
			}
	}
}

/**
* Read DISC returned by emissor, sends DISC and read UA returned by emissor
* @return -1 if can't write, return 0 if can read and write
*/
int llclose(){
	int res;
	
	unsigned char DISC[5] = {FLAG, A, C_DISC, A^C_DISC, FLAG};

	stateMachineReceiver(C_DISC);

	res=write(fd,DISC,5);

	if(res<0){
		printf("Cannot write\n");
		return -1;
	}

	stateMachineReceiver(C_UA);

	return 0;
}

int main(int argc, char** argv)
{
	if ( (argc < 2) ||
	((strcmp("/dev/ttyS0", argv[1])!=0) &&
	(strcmp("/dev/ttYS1", argv[1])!=0) )) {
		printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/tnt1\n");
		exit(1);
	}

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

	llopen();
	createFile();
	llclose();

	sleep(3);

	tcsetattr(fd,TCSANOW,&oldtio);
	close(fd);

	return 0;
}
