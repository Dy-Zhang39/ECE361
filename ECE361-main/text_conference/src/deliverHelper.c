#include "deliverHelper.h"
#include "message.h"
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/stat.h>
#include<sys/time.h>
#include<math.h>
#include <unistd.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<stdbool.h>
#include<time.h>
//return sockfd number, otherwise return -1
int establishConnection(char* ip, char* port){
    int rv;
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    memset(&hints, 0, sizeof hints);
    
    hints.ai_family = AF_INET; //IPv4
    hints.ai_socktype = SOCK_STREAM; //TCP

    rv = getaddrinfo(ip, port, &hints, &servinfo);
    if (rv != 0){
        printf("Can not find address info IP: %s:%s\nerror code: %d\n", ip, port, rv);
        return -1;
    }
    

    for (p = servinfo; p != NULL; p = p->ai_next){

        //since the socket type are the same for server and client are the same, 
        //the client can use server socket type to create its own socket
        //socket() will find the client IP and port by itself so it only need the type of socket
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1){
            printf("Create socket failed\n");
            continue;
        }
        
        if (connect(sockfd,p->ai_addr, p->ai_addrlen) == -1){
            printf("connection Failed\n");
            continue;
        }
        break;
    }

    if (p == NULL){
        printf("client: failed to create socket\n");
        return -1;
    }

    
    freeaddrinfo(servinfo);
    return sockfd;
}

int sendMessage(int sockfd, int type, char* source, char* data){
    struct message mess;

    mess.type = type;

    strcpy(mess.source, source);
    strcpy(mess.data, data);

    mess.size = strlen(mess.data);

    char output[MAX_STR_LEN];
    fromMessageToString(&mess, output);

    int totalSize = mess.size + TOTAL_SIZE_LEN + CMD_LEN + SOURCE_LEN + 4;
    if (send(sockfd, output, totalSize, 0) == -1){
        return -1;
    }
    return totalSize;


}

int recvMessage(int sockfd, int size, char* dest){
    int totalByteRev = 0;
    int numByte, remaining;
    char* ptr = dest;

    while (totalByteRev != size){
        if (totalByteRev > size){
            printf("the total byte received is larger than the size desired\n");
            return -1;
        }

        remaining = size - totalByteRev;
        if ((numByte = recv(sockfd, ptr, remaining, 0)) == -1){
            printf("Error: receive byte is -1\n");
            return -1;
        }else if(numByte == 0){
            printf("Error: receive byte is 0\n");
            return 0;
        }

        totalByteRev += numByte;
        if (totalByteRev < size){
            ptr += numByte;
        }
    }

    return totalByteRev;
}