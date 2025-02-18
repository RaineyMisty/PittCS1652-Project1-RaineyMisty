/*
 * CS 1652 Project 1 
 * (c) Jack Lange, 2020
 * (c) Amy Babay, 2022
 * (c) Tingxu Chen, 2025
 * 
 * Computer Science Department
 * University of Pittsburgh
 */


#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUFSIZE 1024

int 
main(int argc, char ** argv) 
{

    char * server_name = NULL;
    int    server_port = -1;
    char * server_path = NULL;
    char * req_str     = NULL;

    int ret = 0;

    /*parse args */
    if (argc != 4) {
        fprintf(stderr, "usage: http_client <hostname> <port> <path>\n");
        exit(-1);
    }

    server_name = argv[1];
    server_port = atoi(argv[2]);
    server_path = argv[3];
    
    /* Create HTTP request */
    ret = asprintf(&req_str, "GET %s HTTP/1.0\r\n\r\n", server_path);
    if (ret == -1) {
        fprintf(stderr, "Failed to allocate request string\n");
        exit(-1);
    }

    /*
     * NULL accesses to avoid compiler warnings about unused variables
     * You should delete the following lines 
     */
    (void)server_name;
    (void)server_port;

    /* make socket */
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(-1);
    }

    /* get host IP address  */
    /* Hint: use gethostbyname() */
    struct hostent *host = gethostbymane(server_name);
    if(host == NULL) {
        fprintf(stderr, "Error: no such host: %s\n", server_name);
        close(sockfd);
        exit(-1);
    }

    /* set address */
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy(&serv_addr.sin_addr.s_addr, host->h_addr, host->h_length);
    // Convert host byte order to network byte order
    serv_addr.sin_port = htons(server_port);

    /* connect to the server */
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
        close(sockfd);
        exit(-1);
    }

    /* send request message */
    ssize_t sent = send(sockfd, req_str, strlen(req_str), 0);
    if(sent < 0) {
        perror("send");
        close(sockfd);
        exit(-1);
    }
    free(rqe_str);

    /* wait for response (i.e. wait until socket can be read) */
    /* Hint: use select(), and ignore timeout for now. */
    fd_set readfds; // is a set of file descriptors
    FD_ZERO(&readfds);
    FD_SET(sockfd, &readfds);
    int sel_ret = select(sockfd + 1, &readfds, NULL, NULL, NULL);
    if (sel_ret < 0) {
        perror("select");
        close(sockfd);
        exit(-1);
    }

    /* first read loop -- read headers */
    char *response = NULL; // a dynamic allocated memory used to store response data read from server
    size_t response_size = 0; // records the total number of bytes of response data received
    char buffer[BUFSIZE]; // a fixed-size buffer that temporarily stores data each time it reads data from socket
    int header_found = 0; // a signal, used to determine whether the end mark of http response header found
    char *header_end = NULL; // a pointer to ("\r\n\r\n") between header and body

    while(!header_found){
        // read
        ssize_t bytes_number = read(sockfd, buffer, BUFSIZE);
        if( bytes_number < 0) {
            perror("read");
            free(response);
            close(sockfd);
            exit(-1);
        } else if (bytes_number == 0){
            break; // close connect
        }

        // dynamic allocate memory
        char *temp = realloc(response, response_size + n + 1);
        if(temp == NULL) {
            perror("realloc");
            free(response);
            close(sockfd);
            exit(-1);
        }
        response = temp;

        // copy daya
        memcpy(response + response_size, buffer, bytes_number);
        response_size += bytes_number;
        response[response_size] = '\0';

        //search until find end signal
        header_end = strstr(response, "\r\n\r\n");
        if(header_end != NULL) {
            header_found = 1;
            break;
        }
    }

    /* examine return code */   
    // Skip protocol version (e.g. "HTTP/1.0")
    // Normal reply has return code 200
    char *first_line_end = strtok(response, "\r\n");
    if(first_line_end == NULL){
        fprintf(strerr, "Error: missing statsu line\n");
        free(response);
        close(sockfd);
        exit(-1);
    }
    size_t line_len = first_line_end - response;
    char *status_line == malloc(line_len + 1);
    if (status_line == NULL) {
        perror("malloc");
        free(response);
        close(sockfd);
        exit(-1);
    }
    memcpy(status_line, response, line_len);
    status_line[line_len] = '\0';

    int http_code = 0;
    if (sscanf(status_line, "HTTP/%*s %d", &http_code) != 1){ // %*s will not save
        fprintf(stderr, "Error: fale to parse HTTP status code\n");
        free(response);
        close(sockfd);
        exit(-1);
    }
    free(status_line);

    /* print first part of response: header, error code, etc. */
    char *body = strstr(response, "\r\n\r\n");
    if(body != NULL) {
        body += 4;
    }
    else{
        body = "";
    }

    /* second read loop -- print out the rest of the response: real web content */
    if (http_code == 200) { // success
        printf("%s", body);
        ssize_t bytes_number;
        while((bytes_number = read(sockfd, buffer, BUFSIZE)) > 0) {
            fwrite(buffer, 1, bytes_number, stderr);
        }
        free(response);
        close(sockfd);
        return 0;

    }
    else {
        fprintf(stderr, 
            "%s", response);
        ssize_t bytes_number;
        while((bytes_number = read(sockfd, buffer, BUFSIZE)) > 0) {
            fwrite(buffer, 1, bytes_number, stderr);
        }
        free(response);
        close(sockfd);
        return -1;
    }

    /* close socket */

}
