#ifndef deliverHelper_h
#define deliverHelper_h

int establishConnection(char* ip, char* port);
int sendMessage(int sockfd, int type, char* source, char* data);
int recvMessage(int sockfd, int size, char* dest);

#define MAX_STR_LEN 1200

#endif
