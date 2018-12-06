#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define BACKLOG 25

void parse(char *req, int reg_size);

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:56.0) Gecko/20100101 Firefox/56.0r\n";

int main(int argc, char **argv) {
	int cfd;
	char *hostname, buf[MAXLINE], *port;
	struct sockaddr_in client_addr;

	// Validate port number
	if(argc != 2) {
		printf("Needs a port number\n");
		exit(2);
	}

	port = argv[1];

	int fd = open_listenfd(port);
	if(fd < 0) {
		perror("Error in open_listenfd\n");
		exit(2);
	}

	printf("Listening for a request\n");
	
   
	// Accept connection
	socklen_t client_len = sizeof(client_addr);
	cfd = accept(fd, (struct sockaddr *) &client_addr, &client_len);
	if(cfd < 0) {
		perror("Error accepting connection\n");
		exit(2);
	}

	printf("Accepted a Connection\n");

	// Read the client's input
	bzero(buf, MAXLINE);
	int size = read(cfd, buf, MAXLINE);
	if (size < 0) {
		perror("Error reading client\n");
		exit(2);
	}

	printf("Received request: %s\n", buf);

	// Write to the client
	if(write(cfd, "Request received", 16) < 0) {
		perror("Error writing to client\n");
		exit(2);
	}

	parse(buf, size);

	hostname = NULL;
	// Open connection to a server
	cfd = open_clientfd(hostname, port);
	
    printf("%s\n", user_agent_hdr);

    close(cfd);
    close(fd);
    return 0;
}

/** Parse the client's request, supporting only GET commands **/
void parse(char *req, int reg_size) {
	char uri[MAXLINE], req_type[MAXLINE], http[MAXLINE], path[MAXLINE], leftover[MAXLINE], host[MAXLINE], nohttp[MAXLINE], port[5];

	// Parse the request
	sscanf(req, "%s %s %s", req_type, uri, http);
	if(sscanf(uri, "http://%s", nohttp) != 1)
		sscanf(uri, "%s", nohttp);
	sscanf(nohttp, "%[^/]%s", leftover, path);
	if(sscanf(leftover, "%[^:]:%s", host, port) == 1) {
		port[0] = '8';
		port[1] = '0';
	}

	// printf("uri: %s\n", uri);
	// printf("http: %s\n", http);
	// printf("path: %s\n", path);
	// printf("port: %s\n", port);
	// printf("req_type: %s\n", req_type);

	if(strcmp(req_type, "GET") == 0) {
		
	}

	return;
}
