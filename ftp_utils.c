#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include "ftp_utils.h"


const int DATA_BUFF_SIZE = 1024;
const int BUFF_SIZE = 256;
const int ERROR_MSG_SIZE = 300;
const int RETRY_TIMES = 5;

//enum CMD{LS, PWD, CD, GET, PUT};


void error(const char *msg)
{
	perror(msg);
	exit(1);
}

int receiveMsg(int sock, void* buffer, int buffer_size){
	int n = (recv(sock, buffer, buffer_size, 0));
	if(n < 0) {
		error("ERROR receiving message");
	}
	return n;
}

void sendMsg(int sock, void* buffer, int buffer_size){
	int n = (send(sock, buffer, buffer_size, 0));
	
	if( n < 0) {
		error("ERROR sending message");
	}
//	int tmp = RETRY_TIMES;
//	
//	while (tmp--) {
//		if (send(sock, buffer, buffer_size-1, 0) == buffer_size-1) {
//			break;
//		}
//	}
//	
//	if (tmp == -1) {
//		error("ERROR sending message");
//	}
}