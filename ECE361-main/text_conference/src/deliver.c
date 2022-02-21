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

#include "message.h"
#include "deliverHelper.h"

int loginErrorDetection(char *s);
int quitErrorDetection(char* s);
int logoutErrorDetection(char * s);
int JSErrorDetection(char* s);
int LSErrorDetection(char* s);
int CSErrorDetection(char* s);
int listErrorDetection(char* s);
int registerErrorDetection (char* s);
int privateMsgErrorDetection(char* s);
int processMessage(struct message mess);

fd_set masterfds;
int maxFDs;             //the maximum socket file descriptor in the readfds
int commSFD;
bool isConnected;
char myID[MAX_STR_LEN];
char temp[MAX_STR_LEN];

int main(int argc, const char * argv[]){
    struct timeval tv;
    
    fd_set readfds;
    maxFDs = -1;

    commSFD = - 1;          //the socket that is connected to the server
    isConnected = false;   //whether it is connected to the server
    
    //the user ID that used to connected to the server
     
    memset(&myID, '\0', sizeof myID);

    struct sockaddr_storage their_addr;
    int addr_len = sizeof their_addr;


    
    tv.tv_sec = 10;
    tv.tv_usec = 500000;

    //add standard input to the sockets
    FD_ZERO(&masterfds);
    FD_SET(STDIN_FILENO, &masterfds);
    maxFDs = STDIN_FILENO;

    

    for (;;){
        readfds = masterfds;
        printf("start receiving\n");
        if(select(maxFDs + 1, &readfds, NULL, NULL, NULL) == -1){
            printf("select");
            exit(0);
        }
        printf("Receive a message\n");
            

        for (int fd = 0; fd <= maxFDs; fd++){
            printf("Socket: %d, server Socket: %d\n", fd, commSFD);
            if (FD_ISSET(fd, &readfds)){
                if (fd == STDIN_FILENO){
                
                    printf("detect message in input stream\n");
                    memset(&temp, '\0', sizeof(temp));
                    read(STDIN_FILENO, temp, sizeof temp);
                

                    if (strlen(temp) > MAX_DATA){
                        printf("The command/message is too long\n");
                    }else if (strlen(temp) > 1){
                        char* content;          //store the content of the cmd
                        char* cmd;
                        if (temp[0] == '/'){     //it is a commnad
                            content = &temp[1];
                            cmd = strtok(content, " \n");

                            printf("the command is %s\n", cmd);
                            content += strlen(cmd) + 1;
                            if (strcmp(cmd, "login") == 0){
                                
                                loginErrorDetection(content);
                            }else if (strcmp(cmd, "quit") == 0){

                                if (quitErrorDetection(content) == 0){
                                    exit(0);
                                }
                                
                            }else if (strcmp(cmd, "register") == 0){
                                registerErrorDetection(content);

                            }else if (isConnected){
                                printf("enter second phase\n");
                                if (strcmp(cmd, "logout") == 0){

                                    logoutErrorDetection(content);
                                    
                                }else if(strcmp(cmd, "joinsession") == 0){

                                    JSErrorDetection(content);

                                }else if(strcmp(cmd, "leavesession") == 0){

                                    LSErrorDetection(content);
                                    
                                }else if (strcmp(cmd, "createsession") == 0){

                                    CSErrorDetection(content);
                                    
                                }else if (strcmp(cmd, "list") == 0){

                                    listErrorDetection(content);
                                    
                                }else if (strcmp(cmd, "privatemsg") == 0){
                                    
                                    privateMsgErrorDetection(content);
                                }else{
                                    printf("Invalid command\n");
                                }
                            }else{
                                printf("Need to login first\n");
                            }
                        }else{                  //it is a message
                            if (isConnected){
                                
                                if (commSFD == -1){
                                    printf("sfd is -1 but is connected is true\n");
                                    exit(1);
                                }

                                if (strlen(myID) == 0){
                                    printf("did not initialize my userID\n");
                                    exit(1);
                                }

                                if (sendMessage(commSFD, MESSAGE, myID, temp) == -1){
                                    printf("send message in message\n");
                                    exit(1);
                                }else{
                                    printf("Me: %s\n", temp);
                                }
                            }else{
                                printf("Need to login before sending the message\n");
                            }
                            
                        }
                    }else{
                        printf("Please enter something\n");
                    }
                }else{//tcp

                
                    printf("Receive a message from the server\n");
                    if (commSFD == -1){
                        printf("Message received from other connection\n");
                        exit(1);
                    }


                    //read the size of total data first
                    memset(&temp, '\0', sizeof(temp));
                    
                    int byteReceived = recvMessage(commSFD, 20, temp);
                    
                    if(byteReceived == 0 || byteReceived == -1) {
                        printf("server close connection\n");
                        isConnected = false;
                        FD_CLR(commSFD, &masterfds);
                        commSFD = -1;
                        memset(&myID, '\0', sizeof myID);
                        continue;
                    }

                    char* rest;
                    int totalData = (int) strtol(temp, &rest, 10);
                    int remainingData = totalData - TOTAL_SIZE_LEN;

                    if (*rest != '\0'){
                        printf("some extra value is read: %s\n", rest);
                        exit(1);
                    }

                    //read the remaining data
                    memset(&temp, '\0', sizeof temp);
                    if (recvMessage(commSFD, remainingData, temp) == -1){
                        printf("tcp 2nd recv\n");
                        exit(1);
                    }

                    struct message mess;
                    fromStringToMessage(temp, &mess, totalData);

                    printf("The message code is %d\n", mess.type);
                    processMessage(mess);

                }

            }
            
        }
            
        
        
    }
    
    return 0;
}

int loginErrorDetection(char * s){
    char* userID, *password, *serverIP, *serverPort, *extra;

    userID = strtok(s, " \n");
    password = strtok(NULL, " \n");
    serverIP = strtok(NULL, " \n");
    serverPort = strtok(NULL, " \n");
    extra = strtok(NULL, " \n");

    if (userID == NULL || password == NULL || serverIP == NULL || serverPort == NULL){
        printf("Too few arguments\n");
        return -1;
    }else if (extra != NULL){
        printf("Too many arguments\n");
        return -1;
    }else{
        if(isConnected){
            printf("You are already logged in\n");
            return 0;
        }
        if (commSFD == -1){
            commSFD = establishConnection(serverIP, serverPort);
        }
        
        if (commSFD == -1){
            printf("connection failed\n");
            exit(1);
        }
        printf("Connection established sucessfully\n");

        FD_SET(commSFD, &masterfds);
        if (commSFD > maxFDs){
            maxFDs = commSFD;
        }

        isConnected = true;
        strcpy(myID, userID);
        
        if (strcmp(myID, userID)!= 0){
            printf("ID is not copied sucessfully in log in\n");
            exit(1);
        }
        
        if (sendMessage(commSFD, LOGIN, userID, password) == -1){
            printf("send message in login");
            exit(1);
        }

        printf("Log in message is sent to the server\n");

        
    }

}

int quitErrorDetection(char* s){
    char* extra;
    extra = strtok(s, " \n");

    if (extra != NULL){
        printf("Too many argument for quit\n");
        return -1;
    }else{

        //close the connection before shutdown the program
        if (isConnected){
            if (commSFD == -1){
                printf("sfd is -1 but is connected is true\n");
                exit(1);
            }
            if (strlen(myID) == 0){
                printf("did not initialize my userID\n");
                exit(1);
            }
            char data[10];
            memset(&data, '\0', sizeof data);
            if (sendMessage(commSFD, EXIT, myID, data) == -1){
                printf("send message in logout\n");
                exit(1);
            }
            isConnected = false;
            FD_CLR(commSFD, &masterfds);
            memset(&myID, '\0', sizeof myID);
            printf("Log out from the server successfully\n");
        }


        return 0;
    }
}

int logoutErrorDetection(char* s){
    char* extra = strtok(s, " \n");
                                
    if (extra != NULL){
        printf("Too many argument for logout");
        return -1;
    }else{
        if (commSFD == -1){
            printf("sfd is -1 but is connected is true\n");
            exit(1);
        }
        if (strlen(myID) == 0){
            printf("did not initialize my userID\n");
            exit(1);
        }
        char data[10];
        memset(&data, '\0', sizeof data);
        if (sendMessage(commSFD, EXIT, myID, data) == -1){
            printf("send message in logout\n");
            exit(1);
        }

        printf("Logout message is sent to the server\n");
        isConnected = false;
        memset(&myID, '\0', sizeof myID);
        return 0;
    }
}

int JSErrorDetection(char* s){
    char* sessionID, *extra;
    sessionID = strtok(s, " \n");
    extra = strtok(NULL, " \n");

    if (sessionID == NULL){
        printf("Too few argument for joinsession\n");
        return -1;
    }else if (extra != NULL){
        printf("Too many argument for joinsession\n");
        return -1;
    }else{
        if (commSFD == -1){
            printf("sfd is -1 but is connected is true\n");
            exit(1);
        }
        
        if (strlen(myID) == 0){
            printf("did not initialize my userID\n");
            exit(1);
        }

        if (sendMessage(commSFD, JOIN, myID, sessionID) == -1){
            printf("send message in logout\n");
            exit(1);
        }

        printf("Join session message is sent to the server\n");
        return 0;
    }
}

int LSErrorDetection(char* s){
    char* extra;
    extra = strtok(s, " \n");

    if (extra != NULL){
        printf("Too many argument for leavesession\n");
        return -1;
    }else{
        if (commSFD == -1){
            printf("sfd is -1 but is connected is true\n");
            exit(1);
        }
        
        if (strlen(myID) == 0){
            printf("did not initialize my userID\n");
            exit(1);
        }

        char data[10];
        memset(&data, '\0', sizeof data);
        if (sendMessage(commSFD, LEAVE_SESS, myID, data) == -1){
            printf("send message in leavesession\n");
            exit(1);
        }
        printf("Leave session message is sent to the server\n");
        return 0;
    }
}

int CSErrorDetection(char* s){
    char* sessionID, *extra;
    sessionID = strtok(s, " \n");
    extra = strtok(NULL, " \n");

    if (sessionID == NULL){
        printf("Too few argument for createsession\n");
        return -1;
    }else if (extra != NULL){
        printf("Too many argument for createsession\n");
        return -1;
    }else{
        if (commSFD == -1){
            printf("sfd is -1 but is connected is true\n");
            exit(1);
        }
        
        if (strlen(myID) == 0){
            printf("did not initialize my userID\n");
            exit(1);
        }

        if (sendMessage(commSFD, NEW_SESS, myID, sessionID) == -1){
            printf("send message in createsession\n");
            exit(1);
        }

        printf("Create Session message is sent to the server\n");
        return 0;
    }
}

int listErrorDetection(char* s){
    char* extra;
    extra = strtok(s, " \n");

    if (extra != NULL){
        printf("Too many argument for list\n");
        return -1;
    }else{
        if (commSFD == -1){
            printf("sfd is -1 but is connected is true\n");
            exit(1);
        }
        
        if (strlen(myID) == 0){
            printf("did not initialize my userID\n");
            exit(1);
        }

        char data[10];
        memset(&data, '\0', sizeof data);
        if (sendMessage(commSFD, QUERY, myID, data) == -1){
            printf("send message in list\n");
            exit(1);
        }

        printf("list message is sent to the server\n");
        return 0;
    }
}

int registerErrorDetection (char* s){
    char *userID, *password, *ip, *port,*extra;
    char data[MAX_STR_LEN];
    memset(&data, '\0', sizeof data);

    userID = strtok(s, " \n");
    password = strtok(NULL, " \n");
    ip = strtok(NULL, " \n");
    port = strtok(NULL, " \n");
    extra = strtok(NULL, " \n");
    sprintf(data, "%s\n%s", userID, password);

    if (ip == NULL && port == NULL && extra == NULL){
        if(commSFD != -1){
            if (sendMessage(commSFD, REG, myID, data) == -1){
                printf("error:can not send register message\n");
                return -1;
            }
            return 0;
        }else{
            printf("Currently not connecting to the server, please enter the server IP and port\n");
            return 0;
        }
    }else if (extra != NULL){
        printf("too many arguments\n");
        return 0;
    }else if (userID == NULL || password == NULL || ip == NULL || port == NULL){
        printf("too few arguments\n");
        return 0;
    }else if (userID != NULL && password != NULL && ip != NULL && port != NULL){
        if (commSFD != -1){
            if (sendMessage(commSFD, REG, myID, data) == -1){
                printf("error:can not send register message\n");
                return -1;
            }
            return 0;
        }else{
            commSFD = establishConnection(ip, port);
            memset(&myID, '\0', sizeof myID);
            FD_SET(commSFD, &masterfds);
            if (commSFD > maxFDs){
                maxFDs = commSFD;
            }
            if (sendMessage(commSFD, REG, "\0", data) == -1){
                printf("error:can not send register message\n");
                return -1;
            }
            return 0;
        }
    }else {
        printf("Unknown Error\n");
        return -1;
    }
}

int privateMsgErrorDetection(char* s){
    char* userID, *temp;
    userID = strtok(s, " \n");
    temp = strtok(NULL, "\n");

    printf("%s\n",temp);
    char msg[MAX_STR_LEN];
    sprintf(msg, "%s\n%s", userID, temp);

    if (commSFD != -1){
        if (sendMessage(commSFD, P_MESS, myID, msg) == -1){
            printf("Error: can not send private message\n");
            return -1;
        }
        return 0;
    }else{
        printf("Error: there is no socket connection\n");
        return -1;
    }
}

int processMessage(struct message mess){
    switch (mess.type){
        case LO_ACK:
            if (mess.size != 0){
                printf("There are data in the LO_ACK message\n");
                exit(1);
            }

            if (strcmp(mess.source, "server")){
                printf("LO_ACK should be from the server but it is from %s\n", mess.source);
                exit(1);
            }

            printf("Logged in sucessfully for %s\n", myID);
            break;
        
        case LO_NAK:
            if (strcmp(mess.source, "server")){
                printf("LO_NAK should be from the server but it is from %s\n", mess.source);
                exit(1);
            }

            printf("Logged in FAILED\n Reason: %s\n", mess.data);
            if (commSFD == -1){
                printf("sfd is -1 but is connected is true\n");
                exit(1);
            }
            if (strlen(myID) == 0){
                printf("did not initialize my userID\n");
                exit(1);
            }
            
            //reset login status
            isConnected = false;
            memset(&myID, '\0', sizeof myID);
            break;

        case JN_ACK:
            if (strcmp(mess.source, "server")){
                printf("JN_ACK should be from the server but it is from %s\n", mess.source);
                exit(1);
            }

            printf("Joined session successfully\n");
            break;
        
        case JN_NAK:
            if (strcmp(mess.source, "server")){
                printf("JN_NAK should be from the server but it is from %s\n", mess.source);
                exit(1);
            }

            
            
            printf("Failed to joined session\nreason: %s\n", mess.data);
            break;

        case LS_ACK:
            if (strcmp(mess.source, "server")){
                printf("LS_ACK should be from the server but it is from %s\n", mess.source);
                exit(1);
            }

            printf("Leave session successfully\n");
            break;
        
        case LS_NAK:
            if (strcmp(mess.source, "server")){
                printf("LS_NAK should be from the server but it is from %s\n", mess.source);
                exit(1);
            }

            printf("Leave session failed\nReason: %s", mess.data);
            break;
        
        case NS_ACK:
            if (strcmp(mess.source, "server")){
                printf("NS_ACK should be from the server but it is from %s\n", mess.source);
                exit(1);
            }

            printf("Create session successfully\n");
            break;

        case NS_NAK:
            if (strcmp(mess.source, "server")){
                printf("NS_NAK should be from the server but it is from %s\n", mess.source);
                exit(1);
            }

            printf("Create session failed\nReason: %s", mess.data);
            break;

        case MESSAGE:
            printf("%s: %s\n", mess.source, mess.data);
            break;
        
        case MESS_NAK:
            if (strcmp(mess.source, "server")){
                printf("MESS_NAK should be from the server but it is from %s\n", mess.source);
                exit(1);
            }

            printf("Message could not be sent\nReason: %s\n", mess.data);
            break;
        
        case QU_ACK:
            if (strcmp(mess.source, "server")){
                printf("QU_ACK should be from the server but it is from %s\n", mess.source);
                exit(1);
            }

            printf("%s\n", mess.data);
            break;
        
        case REG_ACK:
            printf("New user registered successfully\n");
            break;

        case REG_NAK:
            printf("Register failled\nReason: %s\n", mess.data);
            break;

        case P_MESS:
            printf("A private message received from %s: %s\n", mess.source, mess.data);
            break;
        case PMESS_NAK:
            printf("Private message can not be sent\nReason: %s\n", mess.data);
            break;
        
        default:
            printf("Error: The type of message is %d\n", mess.type);
            exit(1);
            break;
    }
    return 0;

}