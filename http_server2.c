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
 
    char * ok_response_f  = "HTTP/1.0 200 OK\r\n"						\
                            "Content-type: text/plain\r\n"				\
                            "Content-length: %d \r\n\r\n";
    
    char * notok_response = "HTTP/1.0 404 FILE NOT FOUND\r\n"			\
                            "Content-type: text/html\r\n\r\n"			\
                            "<html><body bgColor=black text=white>\n"	\
                            "<h2>404 FILE NOT FOUND</h2>\n"				\
                            "</body></html>\n";
    
  (void)notok_response;  // DELETE ME
  (void)ok_response_f;   // DELETE ME

  char buffer[BUFSIZE];
  memset(buffer, 0, sizeof(buffer));
    /* first read loop -- get request and headers*/
    int bytes_read = read(sock, buffer, sizeof(buffer) - 1);
    if (bytes_read <= 0) {
        /* 出错或客户端关闭连接 */
        close(sock);
        return -1;
    }
    buffer[bytes_read] = '\0';
    printf("Received request:\n%s\n", buffer);

    /* parse request to get file name */

    /* Assumption: For this project you only need to handle GET requests and filenames that contain no spaces */
  
    /* open and read the file */
  
    /* send response */
    char body[] = "Hello, world!";
    int body_len = strlen(body);
    char response[BUFSIZE];
    int header_len = snprintf(response, sizeof(response), ok_response_f, body_len);
    if (header_len < 0 || header_len >= (int)sizeof(response)) {
        fprintf(stderr, "Response header too large\n");
        close(sock);
        return -1;
    }

    if (header_len + body_len < (int)sizeof(response)) {
        memcpy(response + header_len, body, body_len);
        response[header_len + body_len] = '\0';
    } else {
        memcpy(response + header_len, body, sizeof(response) - header_len - 1);
        response[sizeof(response) - 1] = '\0';
    }
    
    write(sock, response, strlen(response));

    /* close socket and free space */
    close(sock );
    return 0;
}



int
main(int argc, char ** argv)
{
    int server_port = -1;
    int ret = 0;
    // int sock = -1;
    int listen_sock = -1;


    /* parse command line args */
    if (argc != 2) {
        fprintf(stderr, "usage: http_server2 port\n");
        exit(-1);
    }

    server_port = atoi(argv[1]);

    if (server_port < 1500) {
        fprintf(stderr, "Requested port(%d) must be above 1500\n", server_port);
        exit(-1);
    }
    
    /* initialize and make socket */
    listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock < 0) {
        perror("socket");
        exit(-1);
    }
    // 允许地址复用
    int opt = 1;
    if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(listen_sock);
        exit(-1);
    }

    /* set server address */
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(server_port);

    /* bind listening socket */
    if (bind(listen_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
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
    
    printf("Server listening on port %d\n", server_port);

    // 初始化存放客户端 socket 的数组
    int client_fds[FD_SETSIZE];
    for (int i = 0; i < FD_SETSIZE; i++) {
        client_fds[i] = -1;
    }

    /* connection handling loop: wait to accept connection */

    while (1) {
        fd_set readfds;
        FD_ZERO(&readfds);
        int max_fd = listen_sock;
        
        // 将监听 socket 加入读集合
        FD_SET(listen_sock, &readfds);
        
        // 将所有已连接的客户端 socket 加入读集合
        for (int i = 0; i < FD_SETSIZE; i++) {
            int fd = client_fds[i];
            if (fd > 0) {
                FD_SET(fd, &readfds);
                if (fd > max_fd) {
                    max_fd = fd;
                }
            }
        }
        
        /* do a select() */
        int activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0) {
            perror("select");
            continue;
        }
        
        // 若监听 socket 可读，则表示有新连接到来
        if (FD_ISSET(listen_sock, &readfds)) {
            struct sockaddr_in cli_addr;
            socklen_t cli_len = sizeof(cli_addr);
            int new_sock = accept(listen_sock, (struct sockaddr *) &cli_addr, &cli_len);
            if (new_sock < 0) {
                perror("accept");
            } else {
                // 将新连接加入到 client_fds 数组中
                int added = 0;
                for (int i = 0; i < FD_SETSIZE; i++) {
                    if (client_fds[i] < 0) {
                        client_fds[i] = new_sock;
                        added = 1;
                        break;
                    }
                }
                if (!added) {
                    fprintf(stderr, "Too many connections, rejecting new connection\n");
                    close(new_sock);
                } else {
                    printf("Accepted new connection from %s:%d\n",
                           inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
                }
            }
        }
        
        /* 对所有已建立连接的 socket 检查是否有可读数据 */
        for (int i = 0; i < FD_SETSIZE; i++) {
            int sock = client_fds[i];
            if (sock > 0 && FD_ISSET(sock, &readfds)) {
                ret = handle_connection(sock);
                (void)ret;  // 生产环境可检查返回值
                /* 处理完请求后从数组中删除该 socket */
                client_fds[i] = -1;
            }
        }
    }

    close(listen_sock);
    return 0;
}
