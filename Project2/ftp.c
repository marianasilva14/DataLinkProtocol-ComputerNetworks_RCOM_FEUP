/*      (C)2000 FEUP  */

#include "ftp.h"

int parseURL(char *path, url_info *info){

  char init[STRING_SIZE];
  char * buffer = (char*)malloc(STRING_SIZE);
  int indexPath = 6;
  int indexBuffer = 0;
  int existsUser = 0;

  sprintf(init, "ftp://");
  int i;
  for(i=0; i <= 5; i++){
    if(path[i]!=init[i]){
      printf("Wrong path!\n");
      return -1;
    }
  }

  for(i = 6; i < STRING_SIZE; i++){

    if(path[i] == '@')
      existsUser = 1;
  }

  if(existsUser){
    indexPath++;
    while(path[indexPath] != ':'){
      buffer[indexBuffer] = path[indexPath];
      indexBuffer++;
      indexPath++;
    }

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
    info->password=malloc(indexBuffer);
    strncpy(info->password, buffer, indexBuffer);
    indexPath++;
    free(buffer);
    buffer = (char*)malloc(STRING_SIZE);
    indexBuffer = 0;
    indexPath++;
  }
  else{
    char array[9];
    strcpy(array, "anonymous");
    i=0;
    while(i != 9){
      buffer[indexBuffer] = array[i];
      i++;
      indexBuffer++;
    }
    info->user=malloc(indexBuffer);
    printf("info->user");
    printf("%s", info->user);
    memcpy(info->user, buffer, indexBuffer);
    printf("info->user");
    printf("%s", info->user);
    free(buffer);
    buffer = (char*)malloc(STRING_SIZE);
    indexBuffer = 0;


    info->password="";

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

  char array3[1];
  strcpy(array3, "\n");
  buffer[indexBuffer] = array3[0];
  indexBuffer++;

  info->path=malloc(indexBuffer);
  strncpy(info->path, buffer, indexBuffer);

  char del = '/';
  info->file = strrchr(info->path, del);
  info->file++;

  printf("\n\n\nFilename: %s\n\n\n", info->file);

  return 0;
}


int getIpByHost(url_info* url)
{
  struct hostent *h;

  printf("Host name url  : %s\n", url->host);

  if ((h=gethostbyname(url->host)) == NULL) {
    herror("gethostbyname");
    exit(1);
  }


  char* ip = inet_ntoa(*((struct in_addr *)h->h_addr));
  url->ip=malloc(sizeof(ip));
  strcpy(url->ip, ip);
  printf("%s\n",url->ip);
  return 0;
}

int connectFTP( const char* ip, int port, sockets* ftp){

	char rd[STRING_SIZE];

	ftp->data = 0;
  printf("ip: ");
  printf("%s\n", ip);
	if ((ftp->control = connectSocket(ip, port)) < 0) {
		printf("Socket cannot be connected - connectFTP.\n");
		return 1;
	}

	if(readFtpReply(ftp->control, rd, sizeof(rd))){
		printf("Read from FTP failed - connectFTP.\n");
		return 1;
	}
	return 0;
}


int connectSocket(const char* ip, int port){
  printf("ip: %s\n", ip);
  int sockfd;
  struct  sockaddr_in server_addr;
  printf("Depois da declração de struct\n");
    /*server address handling*/
  bzero((char*)&server_addr,sizeof(server_addr));
  server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip); /*32 bit Internet address network byte ordered*/
    server_addr.sin_port = htons(port);    /*server TCP port must be network byte ordered */

    /*open an TCP socket*/
  if ((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
    perror("socket()");
    exit(0);
  }

    /*connect to the server*/
  if(connect(sockfd,(struct sockaddr *)&server_addr,sizeof(server_addr)) < 0){
    perror("connect()");
    exit(0);
  }

  return sockfd;
}

int readFtpReply(int control, char* str, size_t size){

  FILE* fp = fdopen(control, "r");
  printf("HERE\n");
  do{
    fgets(str, size, fp);
    printf("%s\n",str);
  }while((str[0] <= 5 && str[0] >= 1) || str[3] != ' ');
  printf("HERE\n");

  return 0;
}

int sendToFTP(int ftpControl, char* str, size_t size){
  write(ftpControl, str, size);
  printf("Send: %s", str);
  return 0;
}

int loginFTP(const char* user, const char* password, sockets* ftp){

  char userTest[STRING_SIZE];
  char passTest[STRING_SIZE];


  sprintf(userTest, "USER %s\n", user);
  sprintf(passTest, "PASS %s\n", password);
  printf("UserTest %s", userTest);
  if(sendToFTP(ftp->control, userTest, strlen(userTest))){
    printf("Login failed!\n");
    return -1;
  }
printf("ftpControl %d\n", ftp->control);
  if(readFtpReply(ftp->control, userTest, STRING_SIZE)){
    printf("Login failed!\n");
    return -1;
  }
  printf("aqui2\n");
  printf("passTest %s", passTest);

  if(sendToFTP(ftp->control, passTest, strlen(passTest))){
    printf("Login failed!\n");
    return -1;
  }
  printf("aqui3");

  if(readFtpReply(ftp->control, passTest, STRING_SIZE)){
    printf("Login failed!\n");
    return -1;
  }

  return 0;
}


int cdFTP(const char* path, sockets* ftp){

  char currPath[STRING_SIZE];

  sprintf(currPath, "CWD %s\r\n", path);

  if(sendToFTP(ftp->control, currPath, strlen(currPath))){
    printf("Sending to FTP failed - changeDirFTP().\n");
    return 1;
  }


  if(readFtpReply(ftp->control, currPath, STRING_SIZE)){
    printf("Read from FTP failed - changeDirFTP().\n");
    return 1;
  }

  return 0;
}

int passiveMode(sockets* ftp){
  printf("NO PASSIVE MODE\n");
  char passive[STRING_SIZE];
  char passiveIp[STRING_SIZE];
  sprintf(passive, "PASV\n");

  if(sendToFTP(ftp->control, passive, strlen(passive)))
  {
    printf("Passive mode failed.\n");
    return 1;
  }

  if(readFtpReply(ftp->control, passive, STRING_SIZE))
  {
    printf("Passive mode failed.\n");
    return 1;
  }
  printf("PASSIVE %s\n", passive);
  int ip1,ip2,ip3,ip4;
  int port1, port2;

  if((sscanf(passive,"227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)", &ip1,&ip2,&ip3,&ip4,&port1,&port2)) < 0)
  {
    printf("Incorrect response.");
    return 1;
  }

  sprintf(passiveIp,"%d.%d.%d.%d",ip1,ip2,ip3,ip4);
  printf("PassiveIp %d.%d.%d.%d\n", ip1, ip2, ip3, ip4);
  int port = port1*256 + port2;

  if((ftp->data = connectSocket(passiveIp,port)) < 0)
  {
    printf("Passive mode cannot be entered.");
    return 1;
  }
  printf("NO FIM DO PASSIVE MODE\n");
  return 0;

}

int copyFileFTP(const char* path, sockets* ftp)
{
  char retr[STRING_SIZE];

  sprintf(retr, "RETR %s\n", path);

  if(sendToFTP(ftp->control, retr, strlen(retr))){
    printf("Sending failed.\n");
    return 1;
  }
  if(readFtpReply(ftp->control, retr, STRING_SIZE)){
    printf("Sending failed.\n");
    return 1;
  }
  return 0;
}


int downloadFileFTP(const char* filename, sockets* ftp){

  FILE* fp;
  int bytes;

  if (!(fp = fopen(filename, "w")))
  {
    printf("Download file.\n");
    return 1;
  }

  char buf[STRING_SIZE];

  while((bytes = read(ftp->data, buf, sizeof(buf))))
  {
    if (bytes < 0) {
      printf("Download failed.\n");
      return 1;
    }

    if((bytes = fwrite(buf,bytes, 1 , fp)) < 0 )
    {
      printf("Download failed.\n");
      return 1;
    }
  }

  fclose(fp);
  close(ftp->data);
  return 0;

}

int disconnectFromFTP(sockets* ftp){

  char disc[STRING_SIZE];
  sprintf(disc, "QUIT\n");

  if(sendToFTP(ftp->control, disc, strlen(disc)))
  {
    printf("Sending failed.\n");
    return 1;
  }

  if(readFtpReply(ftp->control, disc, STRING_SIZE))
  {
    printf("Sending failed.\n");
    return 1;
  }

  return 0;

}

int main(int argc, char** argv){

  if (argc != 2) {
      printf("Incorrect number of arguments\n");
  }

  url_info info;

  if(parseURL(argv[1], &info)==-1)
    exit(0);

  printf("\nFez parseURL\n");
  printf("%s\n", info.user);
  printf("%s\n", info.password);
  printf("%s\n", info.host);
  printf("%s\n", info.path);

  getIpByHost(&info);

  int port = 21;

  sockets socket;

  connectFTP(info.ip, port, &socket);

  loginFTP(info.user, info.password, &socket);

  passiveMode(&socket);

  copyFileFTP(info.path, &socket);
  downloadFileFTP(info.file, &socket);

  disconnectFromFTP(&socket);

	exit(0);
}
