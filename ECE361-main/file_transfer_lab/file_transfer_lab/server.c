//
//  server.c
//  file_transfer_lab
//
//  Created by 张韶阳 on 2021-09-22.
//
//The following code is referenced to Beej's Guide to Network Programming, by Brain Beej Jorgensen Hall
#include<stdio.h>
#include <string.h>
#include<stdlib.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<time.h>



#include "packet.h"

#define MAXBUFLEN 100

#define MAXPORTLEN 100


int writeToFile(char* totalData, int totalSize, char* fileName);
double uniform_rand(double M, double N);

int main(int argc, const char * argv[]) {

    if(argc != 2 ){
        printf("The number of argument is not 2\n");
        exit(1);
    }
    
    //store listen port
    char port[MAXPORTLEN];
    strcpy(port, argv[1]);
    
    
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    struct sockaddr_storage their_addr;
    char buf[MAXBUFLEN];
    char reply[MAXBUFLEN];
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; //use IPv4
    hints.ai_socktype = SOCK_DGRAM; //use UPD
    hints.ai_flags = AI_PASSIVE; //look for the current ip
    
    if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) { //store info in the servinfo
         printf("getaddrinfo error");
         return 1;
    }
    
    //create a socket for server, use IPv4, UDP, 0 as prams.
    for(p = servinfo; p != NULL; p = p->ai_next){
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol))== -1){
            printf("creating socket...");
            continue;
        }
        //bind the socket
        if(bind(sockfd, p->ai_addr, p->ai_addrlen) == -1){
            close(sockfd); //close the FD
            printf("socket biding...");
            continue;
        }
        
        break;
    }
    
    if(p==NULL){
        printf("fail to bind socket\n ");
        return 2;
    }
    
    
    //free the storage
    freeaddrinfo(servinfo);
    //---
    //server start to receive message from client
    printf("Server: waiting to recvfrom...\n");
    /*lab1*/
    addr_len = sizeof their_addr;
    if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0, (struct sockaddr *)
            &their_addr, &addr_len))==-1){
        printf("recvfrom error");
        exit(1);
    }
    
    if(strcmp(buf, "ftp") == 0){
        sendto(sockfd, "yes", 4, 0, (struct sockaddr *)&their_addr, addr_len);
    }else{
        sendto(sockfd, "no", 3, 0, (struct sockaddr *)&their_addr, addr_len);
    }
    printf("yes/no sent");
    
    char* totalData;
    int* fragNo;
    int currentSize=0;
    struct packet pkt;
    
    char temp[1200] = "\0";
    memset(&temp, '\0', sizeof temp);
    /*

    if ((numbytes = recvfrom(sockfd, temp, 1200 , 0, (struct sockaddr *) &their_addr, &addr_len))==-1){
        printf("recvfrom error");
        exit(1);
    }
    printf("data received;");
    fromStringToPacket(temp, &pkt);
    printf("fromStringToPacket");
    totalData = malloc(pkt.totalFrag * sizeof(char));

    for(int i=0; i<pkt.size; i++){
        totalData[currentSize+i]=pkt.fileData[i];
    }
    currentSize+=pkt.size;
    printf("total packet: %d, current packet is %d, current filled totalData size is: %d and the pkt size is %d\n", pkt.totalFrag, pkt.fragNo, currentSize, pkt.size);
    sendto(sockfd, "ACK", 4, 0, (struct sockaddr *)&their_addr, addr_len);
    */


    while(1){
        //get data from que
        addr_len = sizeof their_addr;
        memset(&temp, '\0', sizeof temp);
        if ((numbytes = recvfrom(sockfd, temp, 1200 , 0, (struct sockaddr *)
                                 &their_addr, &addr_len))==-1){
            printf("recvfrom error");
            exit(1);
        }
        fromStringToPacket(temp, &pkt);
        
        
        if (uniform_rand(0, 1)>1e-2){
            //send ack and process packet
            if (pkt.fragNo == 1){
                totalData = malloc(pkt.totalFrag * 1000* sizeof(char));
                fragNo = malloc((pkt.totalFrag+1)*sizeof(int));
                memset(fragNo, 0, sizeof fragNo);
            }
            //printf("fromStringToPacket");
	
            if(fragNo[pkt.fragNo]!= 1){
                //firt time receive the packet
                fragNo[pkt.fragNo]=1;
            }else{
                printf("repeated packet %d revceived", pkt.fragNo);
            }

            for(int i=0; i<pkt.size; i++){
                totalData[currentSize+i]=pkt.fileData[i];
            }
            currentSize+=pkt.size;
            //printf("total packet: %d, current packet is %d, current filled totalData size is: %d and the pkt size is %d\n", pkt.totalFrag, pkt.fragNo, currentSize, pkt.size);
            //printf("packet %d received\n", pkt.fragNo);
	    char message[1000];
 	    sprintf(message, "ACK %d", pkt.fragNo);
            sendto(sockfd, message, strlen(message)+1, 0, (struct sockaddr *)&their_addr, addr_len);
            //exit
            if(pkt.fragNo==pkt.totalFrag){
                break;
            }
            
        }else{
            //drop packet
            printf("Packet %d dropped\n", pkt.fragNo);
        }
        
    }
    
    if(writeToFile(totalData, currentSize, pkt.fileName)==0){
        printf("error when writing to file");
    }
    currentSize=0;
    close(sockfd);
    
    return 0;
}

int writeToFile(char* totalData, int totalSize, char* fileName){
    FILE* fPtr;
    printf("file size: %d\n", totalSize);
    //create/open the file to write
    fPtr = fopen(fileName, "wb");
    //file create/open failed
    if(fPtr==NULL){
        printf("Unable to create file.\n");
        return 0;
    }
    /* Write data to file */
    //fputs(totalData, fPtr);
    fwrite(totalData, 1, totalSize, fPtr);
    /* Close file to save file data */
    fclose(fPtr);
    return 1;
}

double uniform_rand(double M, double N){
    return M + (rand() / ( RAND_MAX / (N-M) ) ) ;
}
