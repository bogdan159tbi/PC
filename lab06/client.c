/*
*  	Protocoale de comunicatii: 
*  	Laborator 6: UDP
*	client mini-server de backup fisiere
*/
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "helpers.h"
#include <string.h>
void usage(char*file)
{
	fprintf(stderr,"Usage: %s ip_server port_server file\n",file);
	exit(0);
}

/*
*	Utilizare: ./client ip_server port_server nume_fisier_trimis
*/
int main(int argc,char**argv)
{
	if (argc < 4)
		usage(argv[0]);
	
	int fd;
	struct sockaddr_in to_station;
	char buf[BUFLEN];

	/*Deschidere socket*/
	int sockfd = socket(AF_INET, SOCK_DGRAM,0);
	DIE(sockfd < 0, "socket not opened\n");
	/* Deschidere fisier 1 pentru citire */
	DIE((fd=open(argv[3],O_RDONLY))==-1,"open file");
	
	/*Setare struct sockaddr_in pentru a specifica unde trimit datele*/
	to_station.sin_addr.s_addr = htonl(inet_network(argv[1]));
	to_station.sin_family = AF_INET;
	to_station.sin_port = htons(atoi(argv[2]));
	
	/*
	*  cat_timp  mai_pot_citi
	*		citeste din fisier
	*		trimite pe socket
	*/
	//send nr of files
	char no_files = argc - 2;
	buf[0] = no_files;
	DIE(sendto(sockfd,&buf,sizeof(buf),0,(struct sockaddr*)&to_station, sizeof(to_station)) <= 0, "sending files no failed\n");
	// send file name before
	for(char j  = 0 ; j < no_files;j++){
		DIE(sendto(sockfd,argv[3 + j],sizeof(argv[3+j]),0,(struct sockaddr*)&to_station, sizeof(to_station)) <= 0, "sendto fail");
		size_t bytes_read;
		DIE((fd=open(argv[3+j],O_RDONLY))==-1,"open file");
		while((bytes_read = read(fd, buf,sizeof(buf))) > 0){
				DIE(sendto(sockfd, buf, bytes_read, 0, (struct sockaddr*)&to_station, sizeof(to_station)) <= 0, "sendto fail");
		}
		memset(buf, 0, sizeof(buf));
		sendto(sockfd, buf, 0, 0, (struct sockaddr*)&to_station, sizeof(to_station));
		close(fd);
	}
	/*Inchidere socket*/
	close(sockfd);
	/*Inchidere fisier*/
	close(fd);
	return 0;
}

