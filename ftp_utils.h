#ifndef FTP_UTILS_H
#define FTP_UTILS_H
	extern const int DATA_BUFF_SIZE;
	extern const int BUFF_SIZE;
	extern const int ERROR_MSG_SIZE;
	enum CMD{LS, PWD, CD, GET, PUT};
	struct cmd{
		enum CMD cid;
		char cparam[256];
	};
	
	void error(const char *msg);

	int receiveMsg(int sock, void* buffer, int buffer_size);
	void sendMsg(int sock, void* buffer, int buffer_size);
#endif