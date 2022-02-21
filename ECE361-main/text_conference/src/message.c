#include "message.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

//string format totalDataSize:commandType:sourceName:data:
//len(totalDataSize) = 20 (fixed)
//len(commandType) = 5 (fixed)
//len(sourceName) = 15 (fixed)
//len(data) = totalDataSize - len(totalDataSize) - len(commandType) - len(sourceName)  - 4
void fromMessageToString(struct message* src, char* dest){
    
    int totalSize = src->size + TOTAL_SIZE_LEN + CMD_LEN + SOURCE_LEN + 4;

    sprintf(dest, "%020d:%05d:%-15s:%s:", totalSize, src->type, src->source, src->data);    
}

void fromStringToMessage(char* src, struct message* dest, int totalSize){
    
    
    int startIndex;
    char* rest = src;   //pointer to the first element of the string that has not been proceed
    
    //used for storing string 
    char temp[1000];
    int tempIndex = 0;

    memset(&temp, '\0', sizeof(temp));

    dest->size = totalSize - (TOTAL_SIZE_LEN + CMD_LEN + SOURCE_LEN) - 4;

    rest++;             //skip the : after the totalDataSize
    
    dest->type=strtol(rest, &rest, 10);
    rest++;

    //store the id of the source
    tempIndex = 0;
    for (int i = 0; i < 15; i++){
        
        //Ignore the space pad at the end of the source name
        if (*rest != ' '){
            temp[tempIndex] = *rest;
            tempIndex++;
        }
        rest++;
    }
    temp[tempIndex] = '\0';
    strcpy(dest->source, temp);
    rest++;

    //store the data
    tempIndex = 0;
    memset(&dest->data, '\0', sizeof(dest->data));
    for(int i = 0; i < dest->size; i++){
        dest->data[i] = *rest;
        rest++;
    }

}