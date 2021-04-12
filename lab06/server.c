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
#include <string.h>
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
void create_file_bk(char *filename){
        char *tok = strtok(filename,".");
        strcat(tok,".bk");
        strcpy(filename,tok);
}
void receive_fcontent(struct sockaddr_in from_station, int sockfd){
        int fd;
	char buf[BUFLEN];
        socklen_t adr_len = sizeof(from_station);
        DIE(recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr*)&from_station, &adr_len) <= 0,"read file name failed");
        create_file_bk(buf);
        DIE((fd=open(buf,O_WRONLY|O_CREAT|O_TRUNC,0666))==-1,"open file");
        size_t read_bytes;
        memset(buf,0,sizeof(buf));
        while ((read_bytes = 
                recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr*)&from_station, &adr_len)) > 0) {
            DIE(write(fd, buf, read_bytes) <= 0, "write done/fail");
        }
        close(fd);

}
int main(int argc,char**argv)
{
	int fd;

	if (argc <  2)
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
	
	// receive nr of files
        socklen_t adr_len = sizeof(from_station);
        DIE(recvfrom(sockfd,buf,sizeof(buf),0, (struct sockaddr*)&from_station, &adr_len) <= 0,"receive files nr failed\n");
	/* Deschidere fisier pentru scriere */
        fprintf(stdout,"%d files\n",buf[0]);
	for(char j = 0 ; j < buf[0] - 1;j++){
                receive_fcontent(from_station,sockfd);
        }
       
	/*Inchidere socket*/	
        close(sockfd);
	
	/*Inchidere fisier*/
        //close(fd);
	return 0;
}

