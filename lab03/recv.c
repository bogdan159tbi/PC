#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10001

int main(void)
{
	msg r;
	int i, res;
	struct my_paket p;
	printf("[RECEIVER] Starting.\n");
	init(HOST, PORT);
	int errors = 0;
	for (i = 0; i < COUNT; i++) {
		/* wait for message */
		res = recv_message(&r);
		if (res < 0) {
			perror("[RECEIVER] Receive error. Exiting.\n");
			return -1;
		}
		memcpy(&p,m.payload,sizeof(p));
		if (p.sum!= xor_sum(r.payload,r.len -1)){
			errors++;
		}
		/* send dummy ACK */
		res = send_message(&r);
		if (res < 0) {
			perror("[RECEIVER] Send ACK error. Exiting.\n");
			return -1;
		}
	}

	printf("[RECEIVER] Finished receiving..\n");
	printf("BAD packets :%d\n",errors);
	return 0;
}
