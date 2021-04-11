/*
*  	Protocoale de comunicatii: 
*  	Laborator 6: UDP
*	mini-server de backup fisiere
*/

#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h>

#include <arpa/inet.h>
#include "helpers.h"


void usage(char*file)
{
	fprintf(stderr,"Usage: %s server_port file\n",file);
	exit(0);
}

/*
*	Utilizare: ./server server_port nume_fisier
*/
int main(int argc,char**argv)
{
	int fd;

	if (argc!=3)
		usage(argv[0]);
	
	struct sockaddr_in my_sockaddr = {
            .sin_family = AF_INET,
            .sin_port = htons(atoi(argv[1])),
            .sin_addr.s_addr = INADDR_ANY
        } , from_station ;
	char buf[BUFLEN];


	/*Deschidere socket*/
        int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        DIE(sockfd < 0, "socket open fail");	
	
	/*Setare struct sockaddr_in pentru a asculta pe portul respectiv */
        /*struct sockaddr_in sin = {
            .sin_family = AF_INET,
            .sin_port = atoi(arg[1]),
            .sin_addr.s_addr = IPADDR_ANY
        }*/

        int rs = bind(sockfd, (struct sockaddr*)&my_sockaddr, sizeof(struct sockaddr_in));
	DIE(rs < 0, "bind fail");
	/* Legare proprietati de socket */
        int enable = 1;
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) == -1) {
            perror("setsocketopt");
            exit(1);
        }
	
	
	/* Deschidere fisier pentru scriere */
	DIE((fd=open(argv[2],O_WRONLY|O_CREAT|O_TRUNC,0644))==-1,"open file");
	
	/*
	*  cat_timp  mai_pot_citi
	*		citeste din socket
	*		pune in fisier
	*/
        socklen_t adr_len = sizeof(from_station);\
        size_t read_bytes;
        while ((read_bytes = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr*)&from_station, &adr_len)) > 0) {
            DIE(write(fd, buf, read_bytes) <= 0, "write done/fail");
        }

	/*Inchidere socket*/	
        close(sockfd);
	
	/*Inchidere fisier*/
        close(fd);
	return 0;
}
