#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include "../ftp_utils.h"

//enum CMD{LS, PWD, CD, GET, PUT};

char buffer[256];
char data_buffer[1024];

void put(int ctrl_sock, int data_sock){
	struct cmd mycmd;
	mycmd.cid = PUT;
	scanf("%s", mycmd.cparam);
	sendMsg(ctrl_sock, &mycmd, sizeof(mycmd));	
	
	FILE* fp;
	int read_size;
	
	if ((fp = fopen(mycmd.cparam, "rb")) == NULL) {
		printf("Error open file: %s.\n", mycmd.cparam);
		return;
	}
	
	while (1) {
		bzero(data_buffer, DATA_BUFF_SIZE);
		read_size = fread(data_buffer, sizeof(char), DATA_BUFF_SIZE, fp);
//		printf("send read_size: %d\n", resad_size);
		sendMsg(data_sock, data_buffer, read_size);
		if (read_size < DATA_BUFF_SIZE) {
			break;
		}
	}
	fclose(fp);
	printf("PUT file %s completed.\n", mycmd.cparam);
}

void get(int ctrl_sock, int data_sock){
	struct cmd mycmd;
	mycmd.cid = GET;
	scanf("%s", mycmd.cparam);
	sendMsg(ctrl_sock, &mycmd, sizeof(mycmd));
	
	FILE* fp;
	int write_size;
	
	if ((fp = fopen(mycmd.cparam, "wb")) == NULL) {
		printf("Error open file: %s.\n", mycmd.cparam);
		return;
	}	
	
	while (1) {
		bzero(data_buffer, DATA_BUFF_SIZE);
		write_size = receiveMsg(data_sock, data_buffer, DATA_BUFF_SIZE);
		fwrite(data_buffer, sizeof(char), write_size, fp);
		if (write_size < DATA_BUFF_SIZE) {
			break;
		}
	}
	fclose(fp);
	printf("GET file %s completed.\n", mycmd.cparam);
}

void ls(int ctrl_sock, int data_sock){	
	struct cmd mycmd;
	mycmd.cid = LS;
	scanf("%s", mycmd.cparam);
	sendMsg(ctrl_sock, &mycmd, sizeof(mycmd));
	
	while (1) {
		int n = receiveMsg(data_sock, data_buffer, DATA_BUFF_SIZE);
		if (n == 0) {
			break;
		}
		printf("%s", data_buffer);
	}
	return;	
}

void pwd(int ctrl_sock, int data_sock){
	struct cmd mycmd;
	mycmd.cid = PWD;
	sendMsg(ctrl_sock, &mycmd, sizeof(mycmd));
	
	receiveMsg(data_sock, data_buffer, DATA_BUFF_SIZE);
	printf("%s\n", data_buffer);
	return;	
}

void cd(int ctrl_sock, int data_sock){
	struct cmd mycmd;
	mycmd.cid = CD;
	scanf("%s", mycmd.cparam);
	sendMsg(ctrl_sock, &mycmd, sizeof(mycmd));
	
	receiveMsg(data_sock, data_buffer, DATA_BUFF_SIZE);
	printf("Current working dir: %s\n", data_buffer);
	return;	
}


int main(int argc, char *argv[])
{
	int sockfd, portno, n;
	int data_sockfd, data_portno;
	struct sockaddr_in serv_addr, data_serv_addr;
	struct hostent *server;
			
	if (argc < 2) {
	   fprintf(stderr,"usage %s hostname\n", argv[0]);
	   exit(0);
	}
	
	portno = 20001;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) 
		error("ERROR opening socket");
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0)
		error("setsockopt(SO_REUSEADDR) failed");		
	server = gethostbyname(argv[1]);
	if (server == NULL) {
		fprintf(stderr,"ERROR, no such host\n");
		exit(0);
	}
	
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, 
		 (char *)&serv_addr.sin_addr.s_addr,
		 server->h_length);
	serv_addr.sin_port = htons(portno);
	
	if ((connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr))) < 0) 
		error("ERROR connecting");
	
/////////////// connection build	
	receiveMsg(sockfd, buffer, BUFF_SIZE);
	printf("%s", buffer);//recv welcome, instructions
	
	sendMsg(sockfd, "PASV", strlen("PASV")+1);
	
	n = receiveMsg(sockfd, &data_portno, sizeof(data_portno));
	#ifdef DEBUG
		printf("got data port num: %d\n", data_portno);
	#endif

	bzero((char *) &data_serv_addr, sizeof(data_serv_addr));
	data_serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, 
		 (char *)&data_serv_addr.sin_addr.s_addr,
		 server->h_length);
	data_serv_addr.sin_port = htons(data_portno);

	
	char cmd[10];	
	while(1){
		data_sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (data_sockfd < 0) {
			error("ERROR opening data socket");
		}
		if (setsockopt(data_sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0)
			error("setsockopt(SO_REUSEADDR) failed");
		if (connect(data_sockfd,(struct sockaddr *) &data_serv_addr,
		 sizeof(data_serv_addr)) < 0) 
			error("ERROR connecting data socket");
		
		printf("miniFTP>>");
		scanf("%s", cmd);
		switch (cmd[0]) {
			case 'l':
				ls(sockfd, data_sockfd);
				break;
			case 'c':
				cd(sockfd, data_sockfd);
				break;
			case 'g':
				get(sockfd, data_sockfd);
				break;
			case 'p':
				if (cmd[1] == 'w') {
					pwd(sockfd, data_sockfd);
				}else{
					put(sockfd, data_sockfd);
				}
			case 'q':
				break;
			default:
				printf("Command not valid.\n");
				break;				
		}
		close(data_sockfd);
		if (cmd[0] == 'q') {
			break;
		}
		
	}
	
	close(sockfd);
	return 0;
}
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			