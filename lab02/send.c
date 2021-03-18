#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "link_emulator/lib.h"

#define HOST "127.0.0.1"
#define PORT 10000
#define MAX_NAME_FILE 50
#define MTP 1400
/*
2800 octeti ,MTU = 1400
1400 + 1400

2850 OCTETI, MTU = 1400
1400 + 1400 + 50 
trebuie sa ma asigur ca stiu cand sa ma opresc
receiver ul trebuie sa stie ca a primit toate bucatile din fisier

o varianta = sa trimitem un mesaj cu dim fisierul si pe urma
               receiver ul numara
a doua varianta = mesaj special pentru terminarea unui mesaj
*/

//codul default pentru a exemplifica
 /*
  //Send dummy message:
  printf("[send] Sending dummy...\n");
  sprintf(t.payload,"%s", "This is a dummy.");
  t.len = strlen(t.payload)+1;
  send_message(&t);
  
  // Check response:
  if (recv_message(&t)<0){
    perror("Receive error ...");
    return -1;
  }
  else {
    printf("[send] Got reply with payload: %s\n", t.payload);
  }

 */

int main(int argc,char** argv){
  init(HOST,PORT);
  msg t;

  //send file's name to receiver
  //then wait for ack 
  printf("sending file name \n");
  sprintf(t.payload,"%s",argv[1]);
  t.len = strlen(t.payload) + 1;
  send_message(&t);

  if (recv_message(&t)<0){
    perror("Receive error ...");
    return -1;
  }
  else {
    printf("[send] Got reply with payload: %s\n", t.payload);
  }

  //send actual file content depending on size
  //then wait for ack
  int filepointer;
  int length;
  filepointer = open("fisier.in", O_RDONLY);
  if( filepointer < 0){
    perror("nu s a putut deschide fisieru");
    exit(-1);
  }
  
  char buffer[MTP];
  int fileRead ;
  while(fileRead = read(filepointer, t.payload,MAX_LEN)){
    if(fileRead < 0){
      perror("Eroare citire");
      exit(-1);
    }
    t.len = fileRead;
    if(t.len == 0 ){
      break;
    }
    //daca nu s a oprit
    //trebuie trimis filee content 
    printf("sending file content\n");
    memcpy(t.payload, buffer,sizeof(buffer));
    send_message(&t);

    if (recv_message(&t)<0){
      perror("Receive error ...");
      return -1;
    }
    else {
      printf("[send] Got reply with payload: %s\n", t.payload);
    }
    
  }
  printf("done with sending file content\nSending end message\n");
  //build end message
  
  close(filepointer);
  
  return 0;
}
