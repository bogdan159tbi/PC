#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "link_emulator/lib.h"

#define HOST "127.0.0.1"
#define PORT 10001
#define FILE_NAME_SIZE 20

int main(int argc,char** argv){
  msg receivedMessage;
  init(HOST,PORT);

  if (recv_message(&receivedMessage)<0){
    perror("Receive message");
    return -1;
  }
  //dupa ce trecem de recvprintf("[recv] ACK sent\n");
  //sigur am primit un mesaj
  //recv_message se blocheaza pana l priemeste
  
  printf("[recv] Got msg with payload: <%s>, sending ACK...\n", receivedMessage.payload);
  // primul mesaj este numele fisierul
  // acum deschidem fisierul
  char *backupFile = malloc(FILE_NAME_SIZE);
  if(!backupFile){
    perror("nu s a alocat ");
    return -1;
  }
  strcpy(backupFile, receivedMessage.payload);
  strcat(backupFile,".bk");
  int fileDesc = open(backupFile, O_WRONLY | O_CREAT, 0666);

  // Send ACK: for having received file s name
  sprintf(receivedMessage.payload,"%s", "ACK");
  receivedMessage.len = strlen(receivedMessage.payload) + 1;
  send_message(&receivedMessage);
  printf("[recv] file name ACK sent\n");

  if(fileDesc < 0){
    perror("nu s a creat fisierul");
    exit(-1);
  }
  //odata primit file content
  // trebuie citite bucatile trimise la fiecare pachet
  while(1){
    if (recv_message(&receivedMessage)<0){
      perror("Receive message");
      return -1;
   }
   //luam cazul cand s a trimis mesajul null
   //pentru a stii ca am terminat de primit file s content
    if(receivedMessage.len == 0){
      break;
  }  //altfel continuam primirea pachetelor 
    //trimitem cate un ACK pt fiecare pachet

    printf("[recv] Got msg with payload: <%s>, sending ACK...\n", receivedMessage.payload);
    int wc = write(fileDesc, receivedMessage.payload,receivedMessage.len);
    if(wc < 0){
      perror("nu s a reusit backup -ul ");
      exit(-1);
    }
    //dupa ce am scris file s content in backup
    //trimitem ACK pentru a primi urm bucata
    sprintf(receivedMessage.payload,"%s","ACK");
    receivedMessage.len = strlen(receivedMessage.payload) + 1;
    send_message(&receivedMessage);
    printf("[recv] ACK sent\n");

  }
  free(backupFile);
  close(fileDesc);

  return 0;
}
