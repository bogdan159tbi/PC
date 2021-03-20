#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10000
#define min(x,y) (x < y) ? x : y

int main(int argc, char *argv[])
{
	msg m;
	int i, res;
	struct my_paket p;
	printf("[SENDER] Starting.\n");	
	init(HOST, PORT);

	/* printf("[SENDER]: BDP=%d\n", atoi(argv[1])); */
	int BDP = atoi(argv[1]);
	int windowSize = (BDP * 1000) / (MSGSIZE * 8);
	
	/*
	windowSize = min(windowSize,COUNT) deoarece daca am avea mai putine mesaje
	de trimis decat incap in fereastra s ar astepta degeaba
	am adaugat xor_sum() in lib.h
	*/
	windowSize = min( windowSize, BDP);
	int controlSum;
	// SEND W msg to fill in window
	for( i = 0 ;i < windowSize ;i++){
		memset(p.rest_payload,5,sizeof(p.rest_payload));
		p.len = MSGSIZE-1;
		/*
		aflu suma de control pe care o adaug in payload
		*/
		controlSum = xor_sum(p.rest_payload,MSGSIZE -1 );
		p.sum = controlSum;
		m.len = MSGSIZE;
		memcpy(m.payload,&p,sizeof(p));
		
		res = send_message(&m);
		if ( res < 0){
			perror("[SENDER] sender error.exiting.. \n");
			return -1;
		}
		// nu se mai asteapta ACK
	}

	for (i = 0; i < COUNT - windowSize; i++) {
		/* wait for ACK */
		res = recv_message(&m);
		if (res < 0) {
			perror("[SENDER] Receive error. Exiting.\n");
			return -1;
		}

		/* cleanup msg */
		memset(p.rest_payload,5,sizeof(p.rest_payload));
		p.len = MSGSIZE-1;
		/* gonna send an empty msg */
		controlSum = xor_sum(p.rest_payload,p.len );
		p.sum = controlSum;
		m.len = MSGSIZE;
		memcpy(m.payload,&p,sizeof(p));
		/* send msg */
		res = send_message(&m);
		if (res < 0) {
			perror("[SENDER] Send error. Exiting.\n");
			return -1;
		}
	}
	for(i = 0;i < windowSize; i++){
		res = recv_message(&m);
		if( res < 0){
			perror("[SENDER] receive error.Exiting.. \n");
			return -1;
		}
	}

	printf("[SENDER] Job done, all %d messages sent using start-stop.\n",COUNT);
		
	return 0;
}
