#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "helpers.h"

void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}

int main(int argc, char *argv[])
{
	int sockfd, newsockfd, portno;
	char buffer[BUFLEN];
	struct sockaddr_in serv_addr, cli_addr;
	int n, i, ret;
	socklen_t clilen;

	fd_set read_fds;	// multimea de citire folosita in select()
	fd_set tmp_fds;		// multime folosita temporar
	int fdmax;			// valoare maxima fd din multimea read_fds

	if (argc < 2) {
		usage(argv[0]);
	}

	// se goleste multimea de descriptori de citire (read_fds) si multimea temporara (tmp_fds)
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	portno = atoi(argv[1]);
	DIE(portno == 0, "atoi");

	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	ret = bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind");

	ret = listen(sockfd, MAX_CLIENTS);
	DIE(ret < 0, "listen");

	// se adauga noul file descriptor (socketul pe care se asculta conexiuni) in multimea read_fds
	FD_SET(sockfd, &read_fds);
	fdmax = sockfd;
	char msg[BUFLEN];
	char *list_fds = calloc(100,sizeof(char));
	DIE(!list_fds," list failed\n");

	while (1) {
		tmp_fds = read_fds; 
		
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				if (i == sockfd) {
					// a venit o cerere de conexiune pe socketul inactiv (cel cu listen),
					// pe care serverul o accepta
					clilen = sizeof(cli_addr);
					newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
					DIE(newsockfd < 0, "accept");
					char newsockfdstr[3];
					sprintf(newsockfdstr,"%d",newsockfd);
					strcat(list_fds, newsockfdstr);
					strcat(list_fds," ");
					// se adauga noul socket intors de accept() la multimea descriptorilor de citire
					FD_SET(newsockfd, &read_fds);
					if (newsockfd > fdmax) { 
						fdmax = newsockfd;
					}
					//TODO: trimite fd al lui 
					//in read_fds exista socket de ascultare + sockets clients
					//iterate read_fds ,send sockid != sockfd
					sprintf(msg,"clientul %d s a conectat",newsockfd);
					for(int fds = 0; fds <= fdmax; fds++){
						if (fds != sockfd){
							if(FD_ISSET(fds, &read_fds)){
								puts("here");
								int r = send(fds, msg, strlen(msg) + 1,0);
								DIE(r < 0 , "send failed\n");
								//add every client to list
							}
						} 
					}
					int res = send(newsockfd, list_fds,strlen(list_fds)+1, 0);
					DIE(res < 0 , "send failed\n");
					printf("Noua conexiune de la %s, port %d, socket client %d\n",
							inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), newsockfd);
				} else {
					// s-au primit date pe unul din socketii de client,
					// asa ca serverul trebuie sa le receptioneze
					memset(buffer, 0, BUFLEN);
					n = recv(i, buffer, sizeof(buffer), 0);
					DIE(n < 0, "recv");

					if (n == 0) {
						// conexiunea s-a inchis
						printf("Socket-ul client %d a inchis conexiunea\n", i);
						sprintf(msg,"clientul %d s a deconectat",i);
						for(int fds = 0; fds <= fdmax; fds++){
							if (fds != sockfd){
								if(FD_ISSET(fds, &read_fds)){
									puts("here");
									int r = send(fds, msg, strlen(msg) + 1,0);
									DIE(r < 0 , "send failed\n");
									//add every client to list
								}
							} 
						}
						close(i);
						
						// se scoate din multimea de citire socketul inchis 
						FD_CLR(i, &read_fds);
					} else {

						char s1[BUFLEN];
						int nr;
						int elem = sscanf(buffer, "%d%[^\n]s", &nr, s1);
						if(elem == 2){
							printf ("S-a primit de la clientul de pe socketul %d mesajul: %s\n", i, buffer);
							int res = send(nr, s1, strlen(s1) + 1,0);// + \0
							DIE(res < 0 ,"send failed\n");
						} else{
							fprintf(stdout,"message ignored\n");
						}
					}
				}
			}
		}
	}

	close(sockfd);

	return 0;
}
