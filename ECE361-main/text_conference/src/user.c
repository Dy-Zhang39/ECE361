//
//  user.c
//  textConferingLab
//
//  Created by 张韶阳 on 2021-11-21.
//

#include "user.h"
#include "deliverHelper.h"
#include <string.h>

#define LIST_MAX 12800

struct user users[USER_MAX];
struct session sessions[SESSION_MAX];
int numOfSessions;
int numOfUsers;

void initUserDB(){

    FILE *fp;
    int fileSize;
    
    char fileName[100];
    memset(&fileName, '\0', 1);
    strcpy(fileName, "../database/userInfo.txt");

    fp = fopen(fileName, "r");
    fseek(fp, 0, SEEK_END);
	//fp points to end file
    fileSize = ftell(fp);
    rewind(fp);

    char* data = (char *) malloc(fileSize * sizeof(char));
    fread(data, fileSize, 1, fp);
    fclose(fp);

    numOfSessions = 0;
    numOfUsers = 0;
    char* user, *password;
    user = strtok(data, ",\n");
    password = strtok(NULL, ",\n");
    users[0].sockfd = -1;
    users[0].active = false;
    users[0].currentSession = -1;
    strcpy(users[0].userID, user);
    strcpy(users[0].pwd, password);
    numOfUsers++;

    while (user != NULL){
        struct user newUser;
        users[numOfUsers].sockfd = -1;
        users[numOfUsers].active = false;
        users[numOfUsers].currentSession = -1;
        
        user = strtok(NULL, ",\n");
        password = strtok(NULL, ",\n");

        if(user != NULL && password != NULL){
            strcpy(users[numOfUsers].userID, user);
            strcpy(users[numOfUsers].pwd, password);
            numOfUsers++;
        }else if(user == NULL && password == NULL){
            //do nothing
        }else{
            printf("User #%d has only one attribute\n", numOfUsers);
            exit(1);
        }
        
    }

    free(data);

    
}

//log the user in, and send the corresonding message whether log in is successful
void loginUser(int sockfd, fd_set* server_fds, char *userID, char *pwd){
    
    bool match=false;
    for(int i=0; i<numOfUsers;i++){
        if(strcmp(userID,users[i].userID) == 0 && strcmp(pwd,users[i].pwd) == 0){
            //login details correct;

            if (users[i].active){
                if (sendMessage(sockfd, LO_NAK, "server", "Already logged in") == -1){
                    printf("Error: could not send Already logged in LO_NAK to socket %d\n", sockfd);
                    exit(1);
                }
                return;
            }

            //set active to 0;
            users[i].sockfd=sockfd;
            users[i].active=true;
            users[i].currentSession = -1;           //user is not in any session in default when he logs in
            match=true;
            printf("User %s logged in successful\n", userID);
            
            if(sendMessage(sockfd, LO_ACK, "server", "\0") == -1){
                printf("Error: sending message in loginUser\n");
                exit(1);
            }
        }else if (strcmp(userID,users[i].userID) == 0 && strcmp(pwd,users[i].pwd) != 0){
            printf("User %s is trying to log in with the wrong password\n", userID);
        }
    }
    
    
    if(match==false){
        sendMessage(sockfd, LO_NAK, "server", "wrong user name/password");
        printf("User %s does not exist/the password is wrong\n", userID);

        //close the socket
        closeConnection(sockfd, server_fds);
    }
}

int closeConnection(int sockfd, fd_set* server_fds){
    close(sockfd);
    FD_CLR(sockfd, server_fds);
    if (FD_ISSET(sockfd, server_fds)){
        return -1;
    }else{
        return 0;
    }
}

void logoutUser(int sockfd, fd_set* server_fds, char* userID){
    
    for(int i=0; i<numOfUsers; i++){
        
        if(strcmp(userID,users[i].userID)==0){
            
            if(users[i].active==false){
                printf("User %s logged out fialed, already loggedout\n", userID);
            }else{
                users[i].active=false;
                closeConnection(sockfd, server_fds);
                users[i].sockfd=-1;
                
                //force the user leave the session
                if (users[i].currentSession != -1){
                    if (removeUserFromSession(i, users[i].currentSession) == -1){
                        exit(1);
                    }

                }

                printf("User %s logged out successful\n", userID);
            }
        }
    }
}


void joinSession(int sockfd, char* userID, char* sessionID){
    bool sessionFound = false;
    bool userFound = false;
    int userIndex = -1;
    int sessionIndex = -1;

    //find the user index
    for (int i = 0; i < numOfUsers && !userFound; i++){
        if (strcmp(users[i].userID, userID) == 0){
            userFound = true;
            userIndex = i;

            if (users[i].active == false){
                printf("The user %s is not logged in but he is requesting to join a session\n");
                if (sendMessage(sockfd, JN_NAK, "server", "Need to log in first") == -1){
                    printf("Error: sending message in join session first for loop\n");
                    exit(1);
                }
                return;
            }
        }
    }

    //there is a socket connection but user can not be found
    if (!userFound){
        printf("Error user %s can not be found\n", userID);
        exit(1);
    }

    for (int i = 0; i < numOfSessions && !sessionFound; i++){
        if (strcmp(sessionID, sessions[i].sessionID) == 0){
            sessionIndex = i;
            sessionFound = true;
        }
    }

    //session can not be found
    if (!sessionFound){
        if (sendMessage(sockfd, JN_NAK, "server", "session does not existed") == -1){
            printf("Error: send message for JN_NAK in join session for session does not existed\n");
            printf("socket: %d\n", sockfd);
            exit(1);
        }else{
            printf("Message JN_NAK is sent to the user %s on socket %d beacuse the session does not existed\n", userID, sockfd);
            return;
        }
    }

    //user already in the session
    if (users[userIndex].currentSession == sessionIndex){
        if (sendMessage(sockfd, JN_NAK, "server", "Already in the session") == -1){
            printf("send message for JN_NAK in join session for alreay in\n");
            exit(1);
        }else{
            printf("Message JN_NAK is sent to the user %s beacuse he is already in the session\n", userID);
        }
        return;

    //user is in other session
    }else if (users[userIndex].currentSession != -1){
        if (sendMessage(sockfd, JN_NAK, "server", "currently in other session")==-1){
            printf("send message for JN_NAK in join session for alreay in other session\n");
            exit(1);
        }else{
            printf("Message JN_NAK is sent to the user %s beacuse he is currently in other session\n", userID);
        }

        return;

    }else{
        
        int sessionSize = sessions[sessionIndex].size;
        //session is full
        if (sessionSize == SESSION_CAPACITY){
            printf("Session %s is reached maximum capacity\n", sessionID);

            if (sendMessage(sockfd, JN_NAK, "server", "Seesion is full")){
                printf("send message for JN_NAK in join session for session is full\n");
                exit(1);
            }

            return;

        }

        if (sendMessage(sockfd, JN_ACK, "server", "\0") == -1){
            printf("send message for JN_ACK in join session\n");
            exit(1);
        }else{
            printf("Message JN_ACK is sent to the user %s\n", userID);
        }

        
        users[userIndex].currentSession = sessionIndex;
        sessions[sessionIndex].guests[sessionSize] = userIndex;
        sessions[sessionIndex].size = sessions[sessionIndex].size + 1;

    }


}

void leaveSeesion(int sockfd, char* userID){

    bool userFound = false;
    int userIndex = -1;
    for (int i = 0; i < numOfUsers && !userFound; i++){
        if(strcmp(users[i].userID, userID) == 0){
            userFound = true;
            userIndex = i;
        }
    }
    
    //user can not found but there is a connection
    if (!userFound){
        printf("Error: Can not find user %s in function leaveSession\n", userID);
        exit(1);
    }


    if (users[userIndex].currentSession != -1){
        int sessionIndex = users[userIndex].currentSession;
        if (removeUserFromSession(userIndex, sessionIndex) ==  -1){
            printf("Error: can not remove user %s from the current session\n", userID);
            exit(1);
        }
    }


    if (sendMessage(sockfd, LS_ACK, "server", "\0") == -1){
        printf("send message for LS_ACK in leave session\n");
        exit(1);
    }else{
        printf("Message LS_ACK is sent to the user %s\n", userID);
    }
}

void newSession(int sockfd, char* userID, char* sessionID){
    bool sessionFound = false;
    for (int i = 0; i < numOfSessions && !sessionFound; i++){
        if (strcmp(sessions[i].sessionID, sessionID) == 0){
            sessionFound = true;

            if (sendMessage(sockfd, NS_NAK, "server", "session already existed") == -1){
                printf("send message for NS_NAK in new session\n");
                exit(1);
            }else{
                printf("Message NS_NAK is sent to the user %s because session already existed\n", userID);
            }
        }
    }

    if (!sessionFound){

        if (createNewSession(sessionID) == -1){
            if (sendMessage(sockfd, NS_NAK, "server", "number of sessions reaches maximum") == -1){
                printf("send message for NS_NAK for number of sessions reaching maximum\n");
                exit(1);
            }else{
                printf("Message NS_NAK is sent to the user%s\n", userID);
            }
        
        }else{
           
           
            if (sendMessage(sockfd, NS_ACK, "server", "") == -1){
                printf("send message for NS_ACK in new session\n");
                exit(1);
            }else{
                printf("Message NS_ACK is sent to the user %s\n", userID);
            }

            joinSession(sockfd, userID, sessionID);
        
        }
    }
}

void getInfo(int sockfd, char* userID){
    char temp[MAX_STR_LEN];
    memset(&temp, '\0', sizeof temp);
    strcpy(temp, "Avaliable users:\n");

    for(int i = 0; i < numOfUsers; i++){
        if (users[i].active){
            printf("user %s is active\n", users[i].userID);
            sprintf(temp, "%s%s\n", temp, users[i].userID);
        }
    }

    sprintf(temp, "%s\nAvaliable Sessions:\n", temp);
    for(int i = 0; i < numOfSessions; i++){
        sprintf(temp, "%s%s\n", temp, sessions[i].sessionID);
    }

    printf("%s",temp);
    if (sendMessage(sockfd, QU_ACK, "server", temp) == -1){
        printf("Error: send message for QU_ACK in getInfo\n");
        exit(1);
    }else{
        printf("Message QU_ACK is sent to the user %s\n", userID);
    }

}


void clientSendMessage(fd_set* readfds, int fdMax, int sockfd, struct message msg){
    int userIndex = -1, sessionIndex = -1;

    
    for (int i = 0; i < numOfUsers && userIndex == -1; i++){
        if (strcmp(users[i].userID, msg.source) == 0){
            userIndex = i;
        }
    }

    if (userIndex == -1){
        printf("Error: can not found user in function clientSendMessage\n");
        exit(1);
    }

    sessionIndex = users[userIndex].currentSession;

    //not in any session
    if (sessionIndex == -1){
        if (sendMessage(sockfd, MESS_NAK, "server", "not in any session") == -1){
            printf("send message for MESS_NAK in clientSendMessage\n");
            exit(1);
        }else{
            printf("Message MESS_NAK is sent to the user %s\n", msg.source);
            return;
        }
    }

    //broadcast
    int size = sessions[sessionIndex].size;
    for (int i = 0; i < size; i++){
        
        int receiverIdx = sessions[sessionIndex].guests[i];

        //user must be online in order to be in a session
        if (!users[receiverIdx].active){
            printf("Error: user %s is not active but he is in session %s\n", users[receiverIdx].userID, sessions[sessionIndex].sessionID);
            exit(1);
        }
        if (receiverIdx != userIndex){//not send to itself
            if (sendMessage(users[receiverIdx].sockfd, MESSAGE, msg.source, msg.data) == -1){
                printf("Error: send message in clientSendMessage while sending to user %s", users[receiverIdx].userID);
                exit(1);
            }else{
                printf("The message from %s is sent to user %s in session %s\n", msg.source, users[receiverIdx].userID, sessions[sessionIndex]);
            }
        }
        

    }

}

//helper functions

int removeUserFromSession(int userIndex, int sessionIndex){
    
    if (sessionIndex >= numOfSessions || userIndex >= numOfUsers || sessionIndex < 0 || userIndex < 0){
        printf("""Error: trying to passing a invalid user index or session index into function removeUserFromSession\n\
        session index: %d, Maximum index: %d\n\
        user index:    %d, Maximum index: %d\n", sessionIndex, numOfSessions, userIndex, numOfUsers);
        return -1;
    }
    struct session* curr = &sessions[sessionIndex];

    bool isFound = false;
    for (int i = 0; i < curr->size && !isFound; i++){
        if (curr->guests[i] == userIndex){

            //swap the last guest to this position
            curr->guests[i] = curr->guests[curr->size - 1];
            curr->guests[curr->size - 1] = -1;
            curr->size = curr->size - 1;
            users[userIndex].currentSession = -1;
            isFound = true;
        }
    }
    
    if (!isFound){
        printf("session index %d does not have user index %d\n", sessionIndex, userIndex);
        return -1;
    }else{
        return 0;
    }

}

int createNewSession(char* sessionID){

    if (numOfSessions > SESSION_MAX){
        printf("The amount of session in the server is reached maximum capacity\n");
        return -1;
    }

    strcpy(sessions[numOfSessions].sessionID, sessionID);
    sessions[numOfSessions].size = 0;
    numOfSessions++;
    return 0;
}

//force quit the user out of the server, find the user given his socket connection
int forceQuitUser(int sockfd){
    for (int i = 0; i < numOfUsers; i++){
        if (users[i].sockfd == sockfd){
            users[i].active = false;
            int sessionIdx = users[i].currentSession;
            users[i].currentSession = -1;
            removeUserFromSession(i, sessionIdx);
            printf("Force quit user %s\n", users[i].userID);
            return 0;
        }
    }

    printf("can not find the user that has socket connection %d\n", sockfd);
    return -1;
}


//return true if reg success and add user to the users list, if reg fail return false
bool regUser(int sockfd, char *data){
    printf("current registered user is :%d", numOfUsers);
    char * userID = strtok(data, "\n");
    char * pwd = strtok(NULL, "\n");
    //leagel check
    if(strlen(userID)<=3){
        printf("User name at least 3 characters, please retry");
        if (sendMessage(sockfd, REG_NAK, "server", "User name at least 3 characters, please retry") == -1){
            printf("Error: sending message in response to resgister user\n");
            exit(1);
        }
        return false;
    }else if(strlen(userID)>=NAME_MAX){
        printf("User name no more than 25 characters, please retry");
        if (sendMessage(sockfd, REG_NAK, "server", "User name no more than 25 characters, please retry") == -1){
            printf("Error: sending message in response to resgister user\n");
            exit(1);
        }
        return false;
    }else if(strlen(pwd)<=3){
        printf("Password at least 3 characters, please retry");
        if (sendMessage(sockfd, REG_NAK, "server", "Password at least 3 characters, please retry") == -1){
            printf("Error: sending message in response to resgister user\n");
            exit(1);
        }
        return false;
    }else if(strlen(pwd)>=PWD_MAX){
        printf("Password no more than 100 characters, please retry");
        if (sendMessage(sockfd, REG_NAK, "server", "Password no more than 100 characters, please retry") == -1){
            printf("Error: sending message in response to resgister user\n");
            exit(1);
        }
        return false;
    }else{
        //check if userID already exits
        for(int i=0; i<numOfUsers; i++){
            if(strcmp(userID,users[i].userID)==0){
                printf("User Name %s already exits, please enter a new one\n", userID);
                if (sendMessage(sockfd, REG_NAK, "server", "User Name %s already exits, please enter a new one\n") == -1){
                    printf("Error: sending message in response to resgister user\n");
                    exit(1);
                }
                return false;
            }
        }
    }
    
    printf("current registered user is :%d", numOfUsers);
    //leagel reg, put on temp user list, will be sotred in database after program exit
    //initial the user info
    
    users[numOfUsers].sockfd=sockfd;
    users[numOfUsers].active=false;                     //reg does not mean login, need to login after reg
    users[numOfUsers].currentSession = -1;           //no current session
    strcpy(users[numOfUsers].userID, userID);
    strcpy(users[numOfUsers].pwd, pwd);
    numOfUsers++;
    printf("current registered user is :%d", numOfUsers);
    printf("User %s is registered, password is %s, please use password to login\n", userID, pwd);
    //send feedback to client
    if(sendMessage(sockfd, REG_ACK, "server", "User is registered, please use password to login") == -1){
        printf("Error: sending message in register user\n");
        exit(1);
    }
    printf("new user is register in the temp list");
    storeUserToDatabase();
    return true;
    
}

void storeUserToDatabase(){

    //organize all data into userList formate
    char userList[MAX_STR_LEN];
    memset(&userList, '\0', sizeof userList);
    char user[MAX_STR_LEN];
    memset(&user, '\0', sizeof user);
    char fileName[100];
    memset(&fileName, '\0', 1);
    strcpy(fileName, "../database/userInfo.txt");
    
    for(int i=0; i<numOfUsers;i++){
        sprintf(userList, "%s%s,%s\n", userList, users[i].userID, users[i].pwd);
    }
    printf("Start wrting user info into local file");
    writeToFile(userList, strlen(userList), fileName);
    return;
    
}

int writeToFile(char* totalData, int totalSize, char* fileName){
    FILE* fPtr;
    printf("file size: %d\n", totalSize);
    //create/open the file to write
    fPtr = fopen(fileName, "w");
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
    printf("User info has been updated on the local file\n");
    return 1;
}

void privateMsg(int sockfd, char* source,char* data){
    printf("the private msg is : %s\n",data);
    char* destUser=strtok(data, "\n");
    char* msg=strtok(NULL, "\n");
    int destIndex = -1;
    //find the dest user
    for (int i = 0; i < numOfUsers && destIndex == -1; i++){
        if (strcmp(users[i].userID, destUser) == 0){
            destIndex = i;
        }
    }
    //if cannot find the user
    if (destIndex == -1){
        printf("Error: can not found user\n");
        if (sendMessage(sockfd, PMESS_NAK, "server", "Error: can not found user") == -1){
            printf("Error: sending message in response to private message \n");
            return;
        }
        
        return;
    }
    printf("the private msg is : %s\n",msg);
    //send to the speficied user
    if (sendMessage(users[destIndex].sockfd, P_MESS, source, msg) == -1){
        printf("Error:  Server side send message failed");
        if (sendMessage(users[destIndex].sockfd, PMESS_NAK, "server", "Server side send message failed") == -1){
            printf("Error: sending message in response to private message \n");
            return;
        }
        return;
    }else{
        //success delivered private message
        printf("server has deliver the private message to %s\n",destUser);
    }

}
