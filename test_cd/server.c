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

struct client_t{
	int checkings_nr;
	int sockfd;
	char *username;
	char *password;
	int logged_in;
};
struct database{
	int clients_nr;
	char **users;
	char **pass;	
};
void add_random_users(struct database **db){
	struct database *data = *db;
	data->clients_nr = MAX_CLIENTS;
	data->users = calloc(MAX_CLIENTS, sizeof(char*));
	DIE(!data->users, "data users failed to alloc\n");
	
	data->pass = calloc(MAX_CLIENTS, sizeof(char*));
	DIE(!data->pass, "data users failed to alloc\n");

	char *name = strdup("user");
	char *pass = strdup("pass");
	for(int i = 0 ; i < MAX_CLIENTS; i++){
		data->users[i] = calloc(100, sizeof(char));
		DIE(!data->users[i], "user");
		strcpy(data->users[i], name);
		char *user_name = data->users[i];
		user_name[strlen(user_name)] = i + '0';;

		data->pass[i] = calloc(100, sizeof(char));
		DIE(!data->pass[i], "user");
		strcpy(data->pass[i], pass);
		data->pass[i][strlen(data->pass[i])] = i + '0';
	}
}
void show_database(struct database *db){
	for(int i = 0; i < db->clients_nr; i++){
		printf("%s %s\n", db->users[i], db->pass[i]);
	}
}
int get_user_index(char *user, char **users){
	int index = -1;
	for(int i = 0 ; i < MAX_CLIENTS; i++){
		if(!strcmp(user, users[i])){
			return i;
		}
	}
	return index;
}
int login(char *user, char *pass, struct database *db){
	int index = get_user_index(user, db->users);

	if(index > -1){
		if(!strcmp(pass, db->pass[index])){
			printf("User %s logged in successfully\n", user);
		} else{ 
			printf("Password incorrect.Try again!\n");
			return -1;
		}
	} else {
		printf("User is not correct.Try again!\n");
		return -1;
	}
	return 0;
}
void get_credentials(char *msg, char **user, char **pass){
	char *tok = strtok(msg, " ");
	
	*user = calloc(30,1);
	strcpy(*user , tok);
	
	tok = strtok(NULL, " ");
	*pass = calloc(30, 1);
	strcpy(*pass, tok);

}
struct client_t * init_client(char *user, char *pass, int sockfd){
	struct client_t * client = calloc(1, sizeof(struct client_t));
	client->username = user;
	client->password = pass;
	client->logged_in = 1;
	client->sockfd = sockfd;
	client->checkings_nr = 0;

	return client;
}
int verify_nr(char *buffer){
	char *tok = strtok(buffer, " ");
	tok = strtok(NULL, " ");
	int nr = atoi(tok);

	for(int i = 2; i < nr;i++){
		if(nr % i == 0)
			return 0;
	}
	return 1;
}
struct client_t *is_logged(int sockfd, struct client_t **clients){
	for(int i = 0 ; i < MAX_CLIENTS; i++){
		if(clients[i] != NULL){
			if(clients[i]->sockfd == sockfd && clients[i]->logged_in == 1){
				return clients[i];
			}
		}
	}
	return NULL;
}
int main(int argc, char *argv[])
{
	int sockfd, newsockfd, portno;
	char *buffer = calloc(BUFLEN , 1);
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

	struct database *db = calloc(1, sizeof(struct database));
	struct client_t **clients = calloc(MAX_CLIENTS, sizeof(struct clien_t*));
	int nr_clients = 0;
	
	add_random_users(&db);
	
	while (1) {
		tmp_fds = read_fds; 
		//select pastreaza doar descriptorii pe care se primesc data
		//de aia se foloseste o multime temporare pt descriptori
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
					// se adauga noul socket intors de accept() la multimea descriptorilor de citire
					FD_SET(newsockfd, &read_fds);
					if (newsockfd > fdmax) { 
						fdmax = newsockfd;
					}
					
					printf("Noua conexiune de la %s, port %d, socket client %d\n",
							inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), newsockfd);
				} else {
					// s-au primit date pe unul din socketii de client,
					// asa ca serverul trebuie sa le receptioneze
					memset(buffer, 0, BUFLEN);
					n = recv(i, buffer, BUFLEN, 0);
					DIE(n < 0, "recv");
					buffer[strlen(buffer)-1] = '\0';
					if (n == 0) {
						// conexiunea s-a inchis
						printf("Socket-ul client %d a inchis conexiunea\n", i);
						close(i);
						// se scoate din multimea de citire socketul inchis 
						FD_CLR(i, &read_fds);
					} else {
						//interpreteaza mesajele primite de la client 
						if(buffer[0] == 'l'){
							//login part
							buffer = buffer + strlen("login") + 1;
							char *user, *pass;
							get_credentials(buffer, &user , &pass);
							ret = login(user, pass, db);

							if(ret == 0){
								clients[nr_clients] = init_client(user, pass, i);
								char *msg = strdup("Logged in");
								ret = send(i, msg, strlen(msg), 0);
								DIE(ret < 0 ," send msg failed\n");
								nr_clients++;
							}
						} else if(buffer[0] == 'v' ){
							struct client_t *cl = is_logged(i, clients); 
							cl->checkings_nr++;
							if(cl){
								ret = verify_nr(buffer);
								char *msg;

								if(ret == 1){
									msg = strdup("Numarul e prim");
								} else {
									msg = strdup("Numarul nu e prim");
								}
								ret = send(i, msg, strlen(msg), 0);
								DIE(ret < 0, "sending failed");
							} else{
								printf("User not logged in\n");
							}
						} else if(buffer[0] == 'h'){
							struct client_t *cl = is_logged(i, clients);
							char *msg = calloc(20, 1);
							if(cl){
								sprintf(msg, "History = %d", cl->checkings_nr);
								ret = send(i, msg, strlen(msg), 0 );
								DIE(ret < 0 , "sending failed\n");
							} else
								printf("User not logged in\n");
						} else {
							close(i);
							FD_CLR(i, &read_fds);
						}
					}
				}
			}
		}
	}

	close(sockfd);

	return 0;
}
