//
//  deliver.c
//  file_transfer_lab
//
//  Created by 张韶阳 on 2021-09-22.
//
//The following code is referenced to Beej's Guide to Network Programming, by Brain Beej Jorgensen Hall
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/stat.h>
#include<sys/time.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<stdbool.h>
#include<time.h>
#include<math.h>
#include "packet.h"

#define SERVERPORT "4950"
#define MAXBUFLEN 100
#define MAXSTRLEN 100

bool checkFileExist(const char* fileName);
struct timeval updateRTT (struct timeval currentRTT, struct timeval start, struct timeval end);

int main(int argc, const char * argv[]) {
    
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    char buf[MAXBUFLEN];
    struct sockaddr_storage their_addr;
    char msg[4] = "ftp";
    char command[MAXSTRLEN], fileName[MAXSTRLEN], plainName[MAXSTRLEN];
    struct timeval start, end;
    double cpuTimeUsed;

    if (argc != 3){
        printf("The number of argument is not 3\n");
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    memset(&command, '\0', sizeof command);
    memset(&fileName, '\0', sizeof fileName);
    
    hints.ai_family = AF_INET; //IPv4
    hints.ai_socktype = SOCK_DGRAM; //UDP


    if ((rv = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0){
        printf("Can not find address info\n");
        exit(1);
    }

    
    for (p = servinfo; p != NULL; p = p->ai_next){

        //since the socket type are the same for server and client are the same, 
        //the client can use server socket type to create its own socket
        //socket() will find the client IP and port by itself so it only need the type of socket
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1){
            printf("Create socket failed\n");
            continue;
        }

        break;
    }

 
    if (p == NULL){
        printf("client: failed to create socket\n");
        exit(1);
    }

    //check the user enter ftp <file name>

    do{
        printf("Please enter command: ftp <file name>\n");
        scanf("%s", command);
	    scanf("%s", fileName); //what if uesr does not enter a file
	
    }while(strcmp(command, "ftp") != 0);

    
    //check the file exist or not
    if (!checkFileExist(fileName)){
        printf("File does not exist");
        exit(1);
    }
    
    for (int i = 6; i < strlen(fileName); i++){
        plainName[i - 6] = fileName[i];
    }

    //send the file to the server
    gettimeofday(&start, 0);
    if ((numbytes = sendto(sockfd, msg, strlen(msg) + 1, 0, p->ai_addr, p->ai_addrlen)) == -1){
        printf("client: sendto");
        exit(1);
    }


    int addr_len = sizeof their_addr;
    if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN - 1, 0, (struct sockaddr *)&their_addr, &addr_len)) == -1){
        printf("recvfrom");
        exit(1);
    }
    
    gettimeofday(&end, 0);

    struct timeval estimatedRTT;
    estimatedRTT.tv_sec = 1;
    estimatedRTT.tv_usec = 0;

    printf("A round trip takes %lf s\n", cpuTimeUsed);

    //check if the sender address is the server address
    if (strcmp(((struct sockaddr *) &their_addr) -> sa_data, p -> ai_addr -> sa_data) != 0){
        printf("sender address is not matched with the server address");
        exit(1);
    }

    //check the whether the message is yes or no
    if (strcmp(buf, "yes") == 0){
        printf("A file transfer can start");
    }else{
        exit(1);
    }
    
    
    //read the file
    FILE *fp;
    int fileSize;
    
    
    fp = fopen(fileName, "rb");

    fseek(fp, 0, SEEK_END);
	//fp points to end file
    fileSize = ftell(fp);
    printf("file size: %d\n", fileSize);
    rewind(fp);

    char* data = malloc(fileSize * sizeof(char));
    fread(data, fileSize, 1, fp);
    fclose(fp);
    

    int totalPacket = (int) ceil((double)fileSize/1000);
    struct packet *packets = malloc(totalPacket * sizeof(struct packet));
    
    //for every 1000 bytes convert the string into packet
    for (int i = 0; i < totalPacket; i++){
        int fragNum = i + 1;
        packets[i].totalFrag = totalPacket;
        packets[i].fragNo = fragNum;
        packets[i].fileName = malloc((strlen(plainName) + 1) * sizeof(char));
        strcpy(packets[i].fileName, plainName);

        int byte = 0;
        for (byte = 0; byte < 1000 && (i * 1000 + byte) < fileSize; byte++){
            int index = i * 1000 + byte;
            packets[i].fileData[byte] = data[index];
        }
        packets[i].size = byte;
    }
    
    //for each packet convert it to string and send it to the server
    char temp[1200] = "\0";
    memset(&temp, '\0', sizeof temp);

    for (int i = 0; i < totalPacket; i++){
        memset(&temp, '\0', sizeof temp);
        fromPacketToString(&packets[i], temp);
        bool ackIsReceived = false;
        bool timeout = false;           //whether timeout happen for this packet
        while(!ackIsReceived){
            //set the start timer for recvfrom
            if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &estimatedRTT, sizeof(estimatedRTT)) == -1){
                printf("client: setsockopt()");
                exit(1);
            }
            
            gettimeofday(&start, 0);
            if ((numbytes = sendto(sockfd, temp, 1200, 0, p->ai_addr, p->ai_addrlen)) == -1){
                printf("client: file sendto");
                exit(1);
            }

            memset(&buf, '\0', sizeof(buf));
            if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN - 1, 0, (struct sockaddr *)&their_addr, &addr_len)) == -1){
                printf("packet %d time out, resend the packet\n", i + 1);
                timeout = true;
            }else{
                gettimeofday(&end, 0);
                
		char temp[1000];
		sprintf(temp, "ACK %d", i+ 1);
                if (strcmp(buf, temp) == 0){
                    printf("Ack %d received\n", i + 1);
		    ackIsReceived = true;
                }else{
                    printf("%s, but we want %s\n", buf, temp);
                }

                
                if (!timeout){
                    estimatedRTT = updateRTT(estimatedRTT, start, end);
                }
                
            }
           

        }
            
            
        /*
        if ((numbytes = sendto(sockfd, temp, 1200, 0, p->ai_addr, p->ai_addrlen)) == -1){
            printf("client: file sendto");
            exit(1);
        }

        memset(&buf, '\0', sizeof(buf));
        if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN - 1, 0, (struct sockaddr *)&their_addr, &addr_len)) == -1){
            printf("recvfrom: after sending data");
            exit(1);
        } 

        if (strcmp(buf, "ACK") == 0){
            printf("Ack %d received\n", i + 1);
        }else{
            printf("File is lost\n");
            exit(1);
        }*/
    }

    printf("File transfter is completed.");
    freeaddrinfo(servinfo);


    close(sockfd);
    
    return 0;
}

bool checkFileExist(const char* fileName){
    FILE *file;
    if ((file = fopen(fileName, "r"))){
        fclose(file);
        return true;
    }else{
        return false;
    }
  
}

struct timeval updateRTT (struct timeval currentRTT, struct timeval start, struct timeval end){
    double sampleRTT, estimatedRTT;
    struct timeval output;
    sampleRTT = end.tv_sec - start.tv_sec + 1e-6 * (end.tv_usec - start.tv_usec);
    estimatedRTT = currentRTT.tv_sec + 1e-6 * currentRTT.tv_usec;
    double a = 0.125;

    estimatedRTT = (1 - a) * estimatedRTT + a * sampleRTT;

    output.tv_sec = (int) estimatedRTT;
    output.tv_usec = (int) ((estimatedRTT - output.tv_sec) * 1e6);
    return output;

}
