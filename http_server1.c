/*
 * CS 1652 Project 1 
 * (c) Jack Lange, 2020
 * (c) Amy Babay, 2022
 * (c) <Student names here>
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

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>


#define BUFSIZE 1024
#define FILENAMESIZE 100


static int 
handle_connection(int sock) 
{

    char * ok_response_f  = "HTTP/1.0 200 OK\r\n"        \
        					"Content-type: text/plain\r\n"                  \
        					"Content-length: %d \r\n\r\n";
 
    char * notok_response = "HTTP/1.0 404 FILE NOT FOUND\r\n"   \
        					"Content-type: text/html\r\n\r\n"                       \
        					"<html><body bgColor=black text=white>\n"               \
        					"<h2>404 FILE NOT FOUND</h2>\n"
        					"</body></html>\n";
    
	// (void)notok_response;  // DELETE ME
	// (void)ok_response_f;   // DELETE ME

    char buffer[BUFSIZE];
    int bytes_read;

    /* first read loop -- get request and headers */
    bytes_read = read(sock, buffer, BUFSIZE - 1);
    if (bytes_read <= 0) {
        close(sock);
        return -1;
    }
    buffer[bytes_read] = '\0';

    /* parse request to get file name */
    /* Assumption: this is a GET request and filename contains no spaces*/
    char method[16], uri[256], version[16];
    if (sscanf(buffer, "%15s %255s %15s", method, uri, version) != 3) {
        close(sock);
        return -1;
    }
    if (strcmp(method, "GET") != 0) {
        /* Only GET method is supported */
        close(sock);
        return -1;
    }
    char filename[FILENAMESIZE];
    if (uri[0] == '/')
        snprintf(filename, FILENAMESIZE, "%s", uri + 1);
    else
        snprintf(filename, FILENAMESIZE, "%s", uri);

    if (strlen(filename) == 0) {
        /* default to index.html if no filename provided */
        strncpy(filename, "index.html", FILENAMESIZE);
        filename[FILENAMESIZE - 1] = '\0';
    }

    /* open and read the file */
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        write(sock, notok_response, strlen(notok_response));
        close(sock);
        return -1;
    }
    fseek(fp, 0, SEEK_END);
    long filesize = ftell(fp);
    rewind(fp);
    char header[BUFSIZE];
    int header_len = snprintf(header, BUFSIZE, ok_response_f, filesize);
    write(sock, header, header_len);
	
	/* send response */
    int n;
    while ((n = fread(buffer, 1, BUFSIZE, fp)) > 0) {
        if (write(sock, buffer, n) < 0) {
            break;
        }
    }

    /* close socket and free pointers */

    fclose(fp);
    close(sock);
    return 0;
}


int 
main(int argc, char ** argv)
{
    int server_port = -1;
    int ret         =  0;
    int sock        = -1;

    /* parse command line args */
    if (argc != 2) {
        fprintf(stderr, "usage: http_server1 port\n");
        exit(-1);
    }

    server_port = atoi(argv[1]);

    if (server_port < 1500) {
        fprintf(stderr, "INVALID PORT NUMBER: %d; can't be < 1500\n", server_port);
        exit(-1);
    }

    /* initialize and make socket */
    listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock < 0) {
        perror("socket");
        exit(-1);
    }

    /* set server address */
    int opt = 1;
    if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(listen_sock);
        exit(-1);
    }
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(server_port);

    /* bind listening socket */
    if (bind(listen_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(listen_sock);
        exit(-1);
    }

    /* start listening */
    if (listen(listen_sock, 10) < 0) {
        perror("listen");
        close(listen_sock);
        exit(-1);
    }

    printf("HTTP server listening on port %d\n", server_port);

    /* connection handling loop: wait to accept connection */
    while (1) {

        sock = accept(listen_sock, (struct sockaddr *)&client_addr, &client_addr_len);
        if (sock < 0) {
            perror("accept");
            continue;
        }

        /* handle connections */
        ret = handle_connection(sock);

		// (void)ret; // DELETE ME
    }

    close(listen_sock);
    return 0;
}
