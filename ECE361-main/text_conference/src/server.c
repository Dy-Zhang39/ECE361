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
#include<stdbool.h>

#include "user.h"
#include "deliverHelper.h"

int writeToFile(char* totalData, int totalSize, char* fileName);
bool processUserRequest(int sockfd, fd_set* readfds, int fdMax, struct message *msg);
//arry of session
//arry of user info: ID (string), current Session (char), repsecttive socket(int), connection(bool),


#define MAX_DATA_SIZE = 1000
#define MAXBUFLEN 100
#define MAXPORTLEN 100




void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


int main(int argc, char** argv) {
    if(argc != 2){
        printf("Not enough or too many arguments.\n");
        return -1;
    }
    char PORT[MAXPORTLEN];
        strcpy(PORT, argv[1]);
    
    fd_set master;    // master file descriptor list
    fd_set read_fds;  // temp file descriptor list for select()
    int fdmax;        // maximum file descriptor number

    int listener;     // listening socket descriptor
    int newfd;        // newly accept()ed socket descriptor
    struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen;

    char buf[256];    // buffer for client data
    int nbytes;

    char remoteIP[INET6_ADDRSTRLEN];

    int yes=1;        // for setsockopt() SO_REUSEADDR, below
    int i, j, rv;

    struct addrinfo hints, *ai, *p;

    initUserDB();        //initialize database
    FD_ZERO(&master);    // clear the master and temp sets
    FD_ZERO(&read_fds);

    // get us a socket and bind it
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; //
    hints.ai_socktype = SOCK_STREAM; //TCP
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
        printf("ERROR");
        exit(1);
    }
    
    for(p = ai; p != NULL; p = p->ai_next) {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0) {
            continue;
        }
        
        // lose the pesky "address already in use" error message
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            continue;
        }

        break;
    }

    // if we got here, it means we didn't get bound
    if (p == NULL) {
        printf("ERROR");
        exit(1);
    }

    freeaddrinfo(ai); // all done with this

    // listen
    if (listen(listener, 10) == -1) {
        perror("listen");
        exit(1);
    }

    // add the listener to the master set
    FD_SET(listener, &master);

    // keep track of the biggest file descriptor
    fdmax = listener; // so far, it's this one

    // main loop
    for(;;) {
        read_fds = master; // copy it
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(1);
        }

        // run through the existing connections looking for data to read
        for(i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) { // we got one!!
                if (i == listener) {
                    // handle new connections
                    addrlen = sizeof remoteaddr;
                    newfd = accept(listener,
                        (struct sockaddr *)&remoteaddr,
                        &addrlen);

                    if (newfd == -1) {
                        perror("accept");
                    } else {
                        FD_SET(newfd, &master); // add to master set
                        if (newfd > fdmax) {    // keep track of the max
                            fdmax = newfd;
                        }
                        printf("selectserver: new connection from %s on "
                            "socket %d\n",
                            inet_ntop(remoteaddr.ss_family,
                                get_in_addr((struct sockaddr*)&remoteaddr),
                                remoteIP, INET6_ADDRSTRLEN),
                            newfd);
                    }
                    
                } else {
                    struct message msg;
                    processUserRequest(i, &master, fdmax, &msg);
//                    // handle data from a client
//                    //check the size and the cmd type
//
//                    if ((v = recv(i, buf, 20 , 0)) <= 0) {
//                        // got error or connection closed by client
//                        if (nbytes == 0) {
//                            // connection closed
//                            printf("selectserver: socket %d hung up\n", i);
//                        } else {
//                            perror("recv");
//                        }
//                        close(i); // bye!
//                        FD_CLR(i, &master); // remove from master set
//                    } else {
//                        // we got some data from a client
//                        for(j = 0; j <= fdmax; j++) {
//                            // send to everyone!
//                            if (FD_ISSET(j, &master)) {
//                                // except the listener and ourselves
//                                if (j != listener && j != i) {
//                                    //send the message to other client
//                                    if (send(j, buf, nbytes, 0) == -1) {
//                                        perror("send");
//                                    }
//                                }
//                            }
//                        }
//                    }
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    }
    
    return 0;
}

//return true if it is not a <text> message, false if it is <text> message that need to be brodcast
bool processUserRequest(int sockfd, fd_set* readfds, int fdMax, struct message *msg){
    
    //get the whole message by fisrt finding the size of the message, and store the message in the msg
    char buf[MAX_STR_LEN];
    memset(&buf, '\0', sizeof buf);

    //check the size of packet first
    int byteReceived = recvMessage(sockfd, 20, buf);
    if (byteReceived == -1){
        printf("tcp 1st recv\n");
        exit(1);
    }
    //client lose connection and send 0; logout the client
    else if(byteReceived == 0) {
        
        forceQuitUser(sockfd);
        FD_CLR(sockfd, readfds);
        close(sockfd);
        printf("server close connection on %d\n", sockfd);
        return true;
    }

    /*
    //client lose connection and send 0; logout the client
    if(er==0){
        for(int i=0; i<numOfUsers;i++){
            if(users[i].sockfd==sockfd){
                users[i].active=false;
                FD_CLR(sockfd, &readfds);
                close(sockfd);
                users[i].sockfd=-1;
                memset(&users[i].currentSession, '\0', sizeof users[i].currentSession);
                printf("User %s logged out due to lost connection", users[i].userID);
            }
        }
        //lost user logged out
        return true;
    }
    */


    char *ptr;
    int messageSize=(int) strtol(buf, &ptr, 10);
    
    int remainingSize = messageSize - TOTAL_SIZE_LEN;

    memset(&buf, '\0', sizeof buf);
    if(recvMessage(sockfd, remainingSize ,buf)==-1){
        printf("receive from error");
        return false;
    }
    


    fromStringToMessage(buf, msg, messageSize);
    //process message by case
    /*LOGIN, LO_ACK, LO_NAK, EXIT,
    JOIN, JN_ACK, JN_NAK,
    LEAVE_SESS, LS_ACK, LS_NAK,
    NEW_SESS, NS_ACK, NS_NAK,
    MESSAGE,
    QUERY, QU_ACK
    */
    

    switch (msg->type){
        case LOGIN:
            loginUser(sockfd, readfds, msg->source, msg->data);
            break;
        case EXIT:
            logoutUser(sockfd, readfds, msg->source);
            break;
        case JOIN:
            joinSession(sockfd, msg->source, msg->data);
            break;
        case LEAVE_SESS:
            leaveSeesion(sockfd, msg->source);
            break;
        case NEW_SESS:
            newSession(sockfd, msg->source, msg->data);
            break;
        case QUERY:
            getInfo(sockfd, msg->source);
            break;
        case MESSAGE:
            clientSendMessage(readfds, fdMax, sockfd, *msg);
            break;
        case REG:
            regUser(sockfd, msg->data);
            break;
        case P_MESS:
            privateMsg(sockfd, msg->source, msg->data);
            break;
        default:
            printf("entered default cmd in server.c");
            break;
    }
    


    
}
