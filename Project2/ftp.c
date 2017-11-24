/*      (C)2000 FEUP  */

#include "ftp.h"

int parseURL(char *path, url_info *info){
  
  char init[STRING_SIZE];
  char * buffer = (char*)malloc(STRING_SIZE);
  int indexPath = 6;
  int indexBuffer = 0;


  sprintf(init, "ftp://");
  int i;
  for(i=0; i <= 5; i++){
    if(path[i]!=init[i]){
      printf("Wrong path!\n");
      return -1;
    }
  }
  if(path[indexPath] == '['){
    indexPath++;
    while(path[indexPath] != ':'){
      buffer[indexBuffer] = path[indexPath];
      indexBuffer++;
      indexPath++;
    }
    
    //strncpy(info->user, buffer, indexBuffer-1);
    printf("%s\n", buffer);
    printf("%d\n", indexBuffer);
    info->user=malloc(indexBuffer);
    memcpy(info->user, buffer, indexBuffer);
    indexPath++;
    free(buffer);
    buffer = (char*)malloc(STRING_SIZE);
    indexBuffer = 0;
    while(path[indexPath] != '@'){
      buffer[indexBuffer] = path[indexPath];
      indexBuffer++;
      indexPath++;
    }
    printf("HEREEEE!\n");
    info->password=malloc(indexBuffer);
    strncpy(info->password, buffer, indexBuffer);
    indexPath++;
    if(path[indexPath] != ']'){
      printf("Wrong path syntax\n");
      return -1;
    }
    free(buffer);
    buffer = (char*)malloc(STRING_SIZE);
    indexBuffer = 0;
    indexPath++;
  }
  while(path[indexPath] != '/'){
    buffer[indexBuffer] = path[indexPath];
    indexBuffer++;
    indexPath++;
  }
  info->host=malloc(indexBuffer);
  strncpy(info->host, buffer, indexBuffer);

  free(buffer);
  buffer = (char*)malloc(STRING_SIZE);
  indexBuffer = 0;
  indexPath++;
  while(path[indexPath] != '\0'){
    buffer[indexBuffer] = path[indexPath];
    indexBuffer++;
    indexPath++;
  }
  info->path=malloc(indexBuffer);
  strncpy(info->path, buffer, indexBuffer);

  return 0;
}


int connect(const char *ip, int port){
  
}

int main(int argc, char** argv){

  //printf("%d\n", argc);

	// int	sockfd;
	// struct	sockaddr_in server_addr;
	// char	buf[] = "Mensagem de teste na travessia da pilha TCP/IP\n";
	// int	bytes;

	/*server address handling*/
	// bzero((char*)&server_addr,sizeof(server_addr));
	// server_addr.sin_family = AF_INET;
	// server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDR);	/*32 bit Internet address network byte ordered*/
	// server_addr.sin_port = htons(SERVER_PORT);		/*server TCP port must be network byte ordered */

	/*open an TCP socket*/
	// if ((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
  //   		perror("socket()");
  //       	exit(0);
  //   	}
	/*connect to the server*/
  //   	if(connect(sockfd,
	//            (struct sockaddr *)&server_addr,
	// 	   sizeof(server_addr)) < 0){
  //       	perror("connect()");
	// 	exit(0);
	// }
    	/*send a string to the server*/
	// bytes = write(sockfd, buf, strlen(buf));
	// printf("Bytes escritos %d\n", bytes);

  //getip.c

  if (argc != 2) {
      //fprintf(stderr,"usage: getip address\n");
      printf("Incorrect number of arguments\n");
     //exit(1);
  }


  // if ((h=gethostbyname(argv[1])) == NULL) {
  //     herror("gethostbyname");
  //     exit(1);
  // }
  printf("%s\n", argv[0]);

  url_info info;
  if(parseURL(argv[1], &info)==-1)
    exit(0);

    printf("%s\n", info.user);
    printf("%s\n", info.password);
    printf("%s\n", info.host);
    printf("%s\n", info.path);

	//close(sockfd);
	exit(0);
}
