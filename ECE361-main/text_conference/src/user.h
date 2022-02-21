//
//  user.h
//  textConferingLab
//
//  Created by 张韶阳 on 2021-11-21.
//

#ifndef user_h
#define user_h

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
#include"message.h"

#define NAME_MAX 25
#define SESSION_ID_SIZE 100
#define PWD_MAX 100
#define USER_MAX 100
#define SESSION_CAPACITY 100
#define SESSION_MAX 100

struct user{
    char userID[NAME_MAX];
    char pwd[PWD_MAX];
    int currentSession; //the corresponding index in the session array
    int sockfd;         //user's conneccted socket with server
    bool active;        //wether the user is online
};

struct session{
    char sessionID[SESSION_ID_SIZE];
    int guests[SESSION_CAPACITY];     //the corresponding index in the users array 
    int size;                                   //current amount of people in the session
};

extern struct user users[USER_MAX];
extern int numOfUsers;

void loginUser(int sockfd, fd_set* server_fds, char *userID, char *pwd);
void logoutUser(int sockfd, fd_set* server_fds, char* userID);
void joinSession(int sockfd, char* userID, char* sessionID);
void leaveSeesion(int sockfd, char* userID);
void newSession(int sockfd, char* userID, char* sessionID);
void getInfo(int sockfd, char* userID);
void clientSendMessage(fd_set* readfds, int fdMax, int socketfd, struct message msg);
void initUserDB();
//register a user
bool regUser(int sockfd, char *data);
void storeUserToDatabase();
int writeToFile(char* totalData, int totalSize, char* fileName);
void privateMsg(int sockfd, char* source,char* data);

//helper function
int removeUserFromSession(int userIndex, int sessionIndex);
int createNewSession(char* sessionID);
int forceQuitUser(int sockfd);
int closeConnection(int sockfd, fd_set* server_fds);
#endif /* user_h */
