#include <string.h>
#include <stdlib.h>
struct packet{
    unsigned int totalFrag;     //total fragment of the whole data
    unsigned int fragNo;        //sequence/fragment number of the single data
    unsigned int size;          //size of the data that this packet holds
    char* fileName;             //the name of the file this data belongs
    char fileData[1000];        //the content of the packet, maximum size 1000 bytes
};

void fromPacketToString(struct packet* src, char* dest){
    sprintf(dest, "%d:%d:%d:%s:", src->totalFrag, src->fragNo, src->size, src->fileName);
    int headerSize=strlen(dest);

    for(int i=0; i<=src->size; i++){
        dest[headerSize]=src->fileData[i];
        headerSize++;
    }

}



void fromStringToPacket(char* src, struct packet* dest){
    
    int index = 0;
    int startIndex = 0;
    char* rest;

    dest ->totalFrag = strtol(src, &rest, 10);
    rest++;

    dest->fragNo = strtol(rest, &rest, 10);
    rest++;

    dest->size = strtol(rest, &rest, 10);
    rest++;

    char temp[1000] = "\0";
    int tempIndex = 0;
    while(*rest != ':'){
        temp[tempIndex] = *rest;
        tempIndex++;
        rest++;
    }
    temp[tempIndex] = '\0';
    dest->fileName = malloc((strlen(temp) + 1) * sizeof(char));
    strcpy(dest->fileName, temp);
    rest++;


    for (int i = 0; i < dest->size; i++){
        dest->fileData[i] = *rest;
        rest++;
    }
    
}
