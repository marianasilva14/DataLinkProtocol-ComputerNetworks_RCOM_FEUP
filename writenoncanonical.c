/*Non-Canonical Input Processing*/
#include "Utilities.h"

volatile int STOP=FALSE;
int flag_alarm=0, conta_alarm=0, flag_continue = 0;
struct termios oldtio,newtio;
FileStruct f;
FILE *file;
int filesize;

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

				read(fd, &foo,1);

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
	void treatframe(unsigned char *frameI,unsigned char *Buf,int bufSize){
		unsigned char BCC1;
		//stuffing da camada de ligação
		frameI[0] = FLAG;
		frameI[1] = A;
		frameI[2] = 0x00;
		BCC1 = A^frameI[2];
		frameI[3] = BCC1;

		//preenchimento da frameI com a camada de aplicacao
		int j = 5;
		int k=4;
		for (int i = 0; i < bufSize;i++)
		{
			if (Buf[i] == 0x7E)
			{
				frameI[k] = 0x7D;
				frameI[j] = 0x5E;
				k++;
				j++;
			}

			if (Buf[i] == 0x7D)
			{
				frameI[k] = 0x7D;
				frameI[j] = 0x5D;
				k++;
				j++;
			}

			if(Buf[i] != 0x7D && Buf[i] != 0x7E)
				frameI[k] = Buf[i];

			j++;
			k++;
		}
	}
	void treatArraySize(int bufSize,unsigned char *Buf,int *finalSize){
		unsigned char BCC2;
		//incrementa o tamanho do array cada vez que encontra estes octetos(0x7E e 0x7D)
		finalSize = bufSize+SIZE_CONNECTION_LAYER;
		for (i = 0; i < bufSize;i++)
		{
			if (Buf[i] == 0x7E || Buf[i] == 0x7D)
				finalSize++;
		}

		//tratar BCC2
		BCC2 = Buf[0]^Buf[1];
		for (i = 2; i < bufSize;i++)
		{
			BCC2 = BCC2^Buf[i];
		}
	}
	int splitFile(int fd, unsigned char *Buf, int size){
			unsigned char BCC1;
			int i,j,res,k;
			int finalSize=0;

			treatArraySize(size,Buf,&finalSize);

			unsigned char* frameI = (unsigned char*)malloc(finalSize);
			treatframe(frameI,Buf,size);

			frameI[finalSize-2]=BCC2;
			frameI[finalSize-1]=FLAG;

			res=write(fd,frameI,finalSize);

			return res;
	}

	unsigned char *preparellwrite(int fd){
		unsigned char *buffer2;
		//camada de ligação
		unsigned char C = 0x00;
		unsigned char BCC1 = A^C;
		unsigned char BCC2;
/*
		fseek(file,0,SEEK_END);
		filesize = ftell(file);
		fseek(file,0,SEEK_SET);
*/
		int bufSize = SIZE_FRAME_I+f.fileSize;
		unsigned char *Buf = (unsigned char *)malloc(bufSize);

		//TRAMA I
		//camada de aplicacao
		Buf[0]=0x02;
		Buf[1]=0x00;
		Buf[2]=0x04;
		memcpy(&Buf[3], buffer2, 4);
		Buf[7]=0x01;
		Buf[8]=f.fileSize;

		//inserir o nome do ficheiro no array
		int i;
		for(i=0; i < f.fileSize;i++){
			Buf[i+9]=f.fileArray[i];
		}

		int finalSize=0;
		treatArraySize(bufSize,Buf,&finalSize);

	//se no interior da trama ocorrer o octeto 0x07E, este é substituido pelo 0x7D 0x5E
	unsigned char *frameI;
	if (BCC2 == 0x7E)
	{
		frameI = (unsigned char *)malloc(finalSize+1);
		frameI[finalSize-3] = 0x7D;
		frameI[finalSize-2] = 0x5E;
	}
  //se no interior da trama ocorrer o octeto 0x07D, este é substituido pelo 0x7D 0x5D
	if (BCC2 == 0x7D)
	{
		frameI = (unsigned char *)malloc(finalSize+1);
		frameI[finalSize-3] = 0x7D;
		frameI[finalSize-2] = 0x5D;
	}

	if (BCC2 != 0x7D && BCC2 != 0x7E)
	{
		 frameI = (unsigned char *)malloc(finalSize);
		 frameI[finalSize-2] = BCC2;
	}

	treatframe(&frameI,&Buf,&bufSize);
	frameI[finalSize-1] = FLAG;

	int res;
	res = write(fd, frameI, finalSize);

	printf("%d bytes written\n",res);
	return 0;
	}

	int llwrite(int fd){

		int res;
		preparellwrite(fd);
		
/*
		if(res >0)
		return res;
		return -1;
		*/
		return 0;
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

		f.fileSize = strlen(argv[2]);
		printf("File size %d , argv[2] = %s\n",f.fileSize,argv[2]);
		f.fileArray = (char*)malloc(f.fileSize);
		memcpy(f.fileArray,argv[2],f.fileSize);

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


		while(STOP==FALSE){
			sleep(0.5);
			res = read(fd,buf,1);


			printf("Resultado: %d\n", res);
			buf[res]=0;
			printf(":%s:%d\n", buf, res);
			if(buf[0]=='\0')
			STOP=TRUE;

		}
				fclose(file);
		if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
			perror("tcsetattr");
			exit(-1);
		}

		close(fd);
		return 0;
	}
