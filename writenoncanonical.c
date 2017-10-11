/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>
#include <stdio.h>


#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FLAG 0x7e
#define A 0x03
#define C_SET 0x03

#define FALSE 0
#define TRUE 1

volatile int STOP=FALSE;
int flag_alarm=0, conta_alarm=0, flag_continue = 0;

void atende()                   // atende alarme
{
	printf("alarme # %d\n", conta_alarm);
	flag_alarm=1;
	conta_alarm++;
	if(conta_alarm==3){
		printf("Exiting...");
		exit(-1);}
}
int main(int argc, char** argv)
{

    int fd,c, res;
	unsigned char foo;
    struct termios oldtio,newtio;
    char buf[255];
	unsigned char SET[5];
	int bytes;

	SET[0]=FLAG;
	SET[1]=A;
	SET[2]=C_SET;
	SET[3]=SET[1]^SET[2];
	SET[4]=FLAG;


    int i, sum = 0, speed = 0;

    if ( (argc < 2) ||
         ((strcmp("/dev/ttyS0", argv[1])!=0) &&
          (strcmp("/dev/ttyS1", argv[1])!=0) )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }

	(void)signal(SIGALRM, atende);  // instala  rotina que atende interrupcao


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
	
	
	

	int state=0;

	
	

		while(!flag_continue){

			bytes = write(fd,SET,5);
			alarm(3);
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
								}
								else state=0;
								break;
						default: continue;
					}
				}
				alarm(0);
				flag_continue = 1;
				
}
    

	gets(buf);
    int length;
    length = strlen(buf);
	buf[length] = 0;


    res = write(fd,buf,length+1);
    printf("%d bytes written\n", res);

    while(STOP==FALSE){
		sleep(0.5);
        res = read(fd,buf,1);
	

        printf("Resultado: %d", res);
        buf[res]=0;
        printf(":%s:%d\n", buf, res);
        if(buf[0]=='\0')
            STOP=TRUE;

    }

    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    close(fd);
    return 0;
}



