#ifndef message_h
#define message_h 0


#define MAX_NAME 20
#define MAX_DATA 1000

//change these constant also need to chagne the value in sprintf() in fromMessageToString()
#define TOTAL_SIZE_LEN 20
#define CMD_LEN 5
#define SOURCE_LEN 15

enum{
    LOGIN, LO_ACK, LO_NAK, EXIT, 
    JOIN, JN_ACK, JN_NAK,
    LEAVE_SESS, LS_ACK, LS_NAK,
    NEW_SESS, NS_ACK, NS_NAK, 
    MESSAGE, MESS_NAK,
    QUERY, QU_ACK, 
    REG, REG_ACK, REG_NAK,
    P_MESS, PMESS_NAK
};

struct message{
    unsigned int type;                  //the type of the command
    unsigned int size;                  //the size of the data
    unsigned char source[MAX_NAME];     //the id of client who send the message
                                        //if the source is server, it will be "server"
    unsigned char data[MAX_DATA];       //the data of the message
    
};

void fromMessageToString(struct message* src, char* dest);
void fromStringToMessage(char* src, struct message* dest, int totalSize);

#endif
