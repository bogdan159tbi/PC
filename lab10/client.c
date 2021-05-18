#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"

int main(int argc, char *argv[])
{
    char *message;
    char *response;
    int sockfd;

    char url[30];
    strcpy(url, "/api/v1/dummy");
        
    // Ex 1.1: GET dummy from main server
    sockfd = open_connection("34.118.48.238", 8080, 
                                AF_INET, SOCK_STREAM, 0);
    message = compute_get_request("34.118.48.238", url, NULL,NULL,0);
    send_to_server(sockfd, message);
    puts(message);
    puts(receive_from_server(sockfd));
    close_connection(sockfd);
    // Ex 1.2: POST dummy and print response from main server
    sockfd = open_connection("34.118.48.238", 8080, 
                                AF_INET, SOCK_STREAM, 0);
    message = compute_post_request("34.118.48.238", url, "application/x-www-form-urlencoded", 
                                    "student=sergiu", NULL, 0);
    send_to_server(sockfd, message);
    puts(message);
    puts(receive_from_server(sockfd));
    close_connection(sockfd);

    //ex 1.3 
    strcpy(url, "/api/v1/auth/login");
    sockfd = open_connection("34.118.48.238", 8080, 
                                AF_INET, SOCK_STREAM, 0);
    message = compute_post_request("34.118.48.238", url, "application/x-www-form-urlencoded", 
                                    "username=student&password=student", NULL, 0);
    send_to_server(sockfd, message);
    puts(message);
    puts(receive_from_server(sockfd));
    close_connection(sockfd);
    //ex 1.4 
    strcpy(url, "/api/v1/weather/key");
    sockfd = open_connection("34.118.48.238", 8080, 
                                AF_INET, SOCK_STREAM, 0);
    char **cookies;
    cookies = calloc(1, sizeof(char*));
    if (cookies == NULL){
        return -1;
    }
    cookies[0] = strdup("connect.sid=s%3A7Ra9POih8Pq1WooKMl3CenQ5nr-Oq-gQ.2PaqRk5M9laKZ2emwO7w4mNXDIqtqp%2BAr1ghadJd7o8");
    if (!cookies[0]) 
        return -1;

    message = compute_get_request("34.118.48.238", url, NULL,cookies,1);
    send_to_server(sockfd, message);
    puts(message);
    puts(receive_from_server(sockfd));
    close_connection(sockfd);
    // {"key":"b912dd495585fbf756dc6d8f415a7649"}

    strcpy(url, "/api/v1/auth/login");
    sockfd = open_connection("34.118.48.238", 8080, 
                                AF_INET, SOCK_STREAM, 0);
    message = compute_post_request("34.118.48.238", url, "application/x-www-form-urlencoded", 
                                    "username=student&password=student", cookies, 1);
    send_to_server(sockfd, message);
    puts(message);
    puts(receive_from_server(sockfd));
    close_connection(sockfd);

    // Ex 2: Login into main server

    // Ex 3: GET weather key from main server
    // Ex 4: GET weather data from OpenWeather API
    // Ex 5: POST weather data for verification to main server
    
    //BOnus
    //api.openweathermap.org/data/2.5/weather?lat={lat}&lon={lon}&appid={API key}
    sockfd = open_connection("188.166.16.132", 80, 
                                AF_INET, SOCK_STREAM, 0);
    //host tre sa fie nume,nu adresa ip server
    char query_params[100];
    strcpy(url, "/data/2.5/weather");
    strcpy(query_params, "lat=44&lon=22&appid=b912dd495585fbf756dc6d8f415a7649");
    
    message = compute_get_request("api.openweathermap.org", url, query_params,NULL,0);
    
    char *message_bonus;
    message_bonus = calloc(100000,1);
    if(!message_bonus){
        fprintf(stdout,"calloc failed\n");
        return -1;
    }    
    send_to_server(sockfd, message);
    puts(message);
    message = receive_from_server(sockfd);
    strcpy(message_bonus, basic_extract_json_response(message));
    close_connection(sockfd);

    //verificare
    strcpy(url, "/api/v1/weather/44/22");
    sockfd = open_connection("34.118.48.238", 8080, 
                                AF_INET, SOCK_STREAM, 0);
    message = compute_post_request("34.118.48.238", url, "application/json", 
                                    message_bonus, cookies, 1);
    send_to_server(sockfd, message);
    puts(message);
    message = receive_from_server(sockfd);
    puts(message);
    close_connection(sockfd);

    // Ex 6: Logout from main server
    //cookie devine invalid la delogare
    //daca decomentez, nu mai merge cookie -ul vechi
    //copiez dupa alt cookie(poate expira)
    /*
    strcpy(url, "/api/v1/auth/logout");
    sockfd = open_connection("34.118.48.238", 8080, 
                                AF_INET, SOCK_STREAM, 0);
    message = compute_get_request("34.118.48.238", url, NULL,NULL,0);
    send_to_server(sockfd, message);
    puts(message);
    puts(receive_from_server(sockfd));
    close_connection(sockfd);
    */
    // BONUS: make the main server return "Already logged in!"

    // free the allocated data at the end!
    free(message_bonus);
    return 0;
}
