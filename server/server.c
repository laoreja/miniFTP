#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <dirent.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>

#include "../ftp_utils.h"

/*
control connection port: 20001
data connection port: start from 30000
*/

void control_connection(int, int); 

int main(int argc, char *argv[])
{
	int next_data_port = 30000;
	int sockfd, newsockfd, ctrl_port, pid;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;
	
	ctrl_port = 20001;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) 
		error("ERROR opening control connection socket");
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0)
		error("setsockopt(SO_REUSEADDR) failed");
	
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(ctrl_port);
	
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr))
			 < 0) 
		error("ERROR on binding");
		
	listen(sockfd,5);
	clilen = sizeof(cli_addr);
	signal(SIGCHLD, SIG_IGN);
	while (1) {
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
		if (newsockfd < 0) 
			error("ERROR on accept");

		pid = fork();
		if (pid < 0)
			error("ERROR on fork");
		if (pid == 0)  { //child
			close(sockfd);
			control_connection(newsockfd, next_data_port);
			close(newsockfd);
			exit(0);
		}
		else { //parent
			next_data_port++;
			close(newsockfd); 
		} /* end of while */
	}
	close(sockfd);
	return 0; /* we never get here */
}

/******** control_connection() *********************
 There is a separate instance of this function 
 for each connection.  It handles all communication
 once a connnection has been established.
 *****************************************/
void control_connection (int sock, int port_num)
{
	int n;
	char buffer[BUFF_SIZE];
	char data_buffer[DATA_BUFF_SIZE];	
	
	int sockfd, newsockfd;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;

//	bzero(buffer, BUFF_SIZE);
    strcpy(buffer, "Welcome to laoreja's miniFTP~\n\
		Supported functions:\n\
		ls dir\n\
		pwd\n\
		cd path\n\
		get filename\n\
		put filename\n\
		quit\n");
	sendMsg(sock, buffer, strlen(buffer)+1);

	receiveMsg(sock, buffer, BUFF_SIZE);
	if (strcmp(buffer, "PASV") == 0){
		printf("receive PASV from client\n");
			
		bzero((char *) &serv_addr, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = INADDR_ANY;
		serv_addr.sin_port = htons(port_num);
		#ifdef DEBUG
			printf("data port num: %d\n", port_num);
		#endif

				
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd < 0) 
			error("ERROR opening data connection socket");
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0)
			error("setsockopt(SO_REUSEADDR) failed");		
		if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr))
			 < 0) 
			error("ERROR on binding");
		listen(sockfd,3);
		clilen = sizeof(cli_addr);
		
		sendMsg(sock, &port_num, sizeof(port_num));

					
//		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, 
//							&clilen);
//		if (newsockfd < 0) 
//			error("ERROR on accept");

			
		while (1) {
			newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, 
								&clilen);
			if (newsockfd < 0) 
				error("ERROR on accept");
			
			struct cmd recvcmd;
			n = receiveMsg(sock, &recvcmd, sizeof(recvcmd));
			#ifdef DEBUG
				printf("receive cmd id: %d, param: %s\n", recvcmd.cid, recvcmd.cparam);
			#endif
			
			switch (recvcmd.cid) {
				case LS:{
					DIR *dir;
					struct dirent *ptr;
					struct stat sb;
					struct tm *ctime;
					struct passwd *pw;
					struct group * gr;
					
					dir = opendir(recvcmd.cparam);
					bzero(data_buffer, DATA_BUFF_SIZE);
					
					while((ptr = readdir(dir)) != NULL)
					{
						if((ptr->d_name[0]) != '.')
						{
							stat(ptr->d_name, &sb);
							ctime = gmtime(&sb.st_mtime);
							
							pw = getpwuid(sb.st_uid);
							gr = getgrgid(sb.st_gid);
							
							sprintf(data_buffer, (S_ISDIR(sb.st_mode)) ? "d" : "-");
							sprintf(data_buffer+1, (sb.st_mode & S_IRUSR) ? "r" : "-");
							sprintf(data_buffer+2, (sb.st_mode & S_IWUSR) ? "w" : "-");
							sprintf(data_buffer+3, (sb.st_mode & S_IXUSR) ? "x" : "-");
							sprintf(data_buffer+4, (sb.st_mode & S_IRGRP) ? "r" : "-");
							sprintf(data_buffer+5, (sb.st_mode & S_IWGRP) ? "w" : "-");
							sprintf(data_buffer+6, (sb.st_mode & S_IXGRP) ? "x" : "-");
							sprintf(data_buffer+7, (sb.st_mode & S_IROTH) ? "r" : "-");
							sprintf(data_buffer+8, (sb.st_mode & S_IWOTH) ? "w" : "-");
							sprintf(data_buffer+9, (sb.st_mode & S_IXOTH) ? "x" : "-");
							
							sprintf(data_buffer+10," %u %s %s %lld %d %d %d %d:%d %s\n", &sb.st_nlink, pw->pw_name, gr->gr_name, sb.st_size, ctime->tm_year+1900, 1+ctime->tm_mon, ctime->tm_mday, ctime->tm_hour, ctime->tm_min, ptr->d_name);
							
							sendMsg(newsockfd, data_buffer, strlen(data_buffer)+1);
						}
					}
					close(newsockfd);
					break;
				}
				case PWD:{
					bzero(data_buffer, DATA_BUFF_SIZE);
					getcwd(data_buffer, DATA_BUFF_SIZE);
					sendMsg(newsockfd, data_buffer, strlen(data_buffer)+1);		
					}
					close(newsockfd);
					break;
				case CD:{
					chdir(recvcmd.cparam);
					getcwd(data_buffer, DATA_BUFF_SIZE);
					sendMsg(newsockfd, data_buffer, strlen(data_buffer)+1);

					}
					close(newsockfd);
					break;
				case PUT:{
					FILE* fp;
					if ((fp = fopen(recvcmd.cparam, "wb")) == NULL) {
						sendMsg(newsockfd, "Error create the file.\n", strlen("Error create the file.\n")+1);
						close(newsockfd);
					}else{
						while (1) {
							bzero(data_buffer, DATA_BUFF_SIZE);
							n = receiveMsg(newsockfd, data_buffer, DATA_BUFF_SIZE);
//							printf("receive msg size: %d\n", n);
							fwrite(data_buffer, sizeof(char), n, fp);
							if (n < DATA_BUFF_SIZE) {
								break;
							}
						}
						fclose(fp);						
						close(newsockfd);	
					}
					break;					
				}
				case GET:{
					FILE* fp;
					if ((fp = fopen(recvcmd.cparam, "rb")) == NULL) {
						sendMsg(newsockfd, "Error open the file.\n", strlen("Error open the file.\n")+1);
						close(newsockfd);
					}else{
						while (1) {
							bzero(data_buffer, DATA_BUFF_SIZE);
							n = fread(data_buffer, sizeof(char), DATA_BUFF_SIZE, fp);
							sendMsg(newsockfd, data_buffer, n);
							if (n < DATA_BUFF_SIZE) {
								break;
							}
						}
						fclose(fp);
						close(newsockfd);
					}
					break;
				}
				default:
					break;
			}

			
		}
		
//		bzero(data_buffer, DATA_BUFF_SIZE);
//		n = receiveMsg(newsockfd, data_buffer, DATA_BUFF_SIZE);
//		printf("Here is the message: %s",data_buffer);
//		
//		strcpy(buffer, "Server got your message\n");
//		sendMsg(newsockfd, buffer, BUFF_SIZE);

		close(newsockfd);
		close(sockfd);

	}else{
		error("client failed to send PASV packet");
	}
}

