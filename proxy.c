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

void parse(int cfd);
int foward_request(rio_t *rp);
int foward_response(rio_t *rp, int cfd);

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:56.0) Gecko/20100101 Firefox/56.0\r\n";
static const char *prox_conn = "Proxy-Connection: close\r\n";
static const char *conn = "Connection: close\r\n";

int main(int argc, char **argv) {
	char *port;
	
	// Validate port number
	if(argc != 2) {
		printf("Needs a port number\n");
		exit(2);
	}

	port = argv[1];

	// Proxy file descriptor
	int fd = open_listenfd(port);
	if(fd < 0) {
		perror("Not a valid port number\n");
		exit(2);
	}

	Signal(SIGPIPE, SIG_IGN);

	// Accept connections
	socklen_t client_len;
	struct sockaddr_in client_addr;
	int cfd;
	while(1) {
		client_len = sizeof(client_addr);
		printf("Listening for a connection\n");
		cfd = accept(fd, (struct sockaddr *) &client_addr, &client_len);
		parse(cfd);
		close(cfd);
		printf("Transaction completed!\n");
	}
	
	close(fd);
	return 0;
}

// Parse the request from the client
void parse(int cfd) {
	printf("\n");
	// Precondition
	if(cfd < 0) {
		perror("Error accepting connection from client\n");
		return;
	}

	rio_t rp;
	// Handle requests from client
	int sfd;
	rio_readinitb(&rp, cfd);
	if((sfd = foward_request(&rp)) < 0)
		return;
	
	// Handle response from server
	rio_readinitb(&rp, sfd);
	if(foward_response(&rp, cfd) < 0)
		return;

	close(sfd);
	return;
}

// Foward the request data from client's rio_t to the server (sfd), which is returned
int foward_request(rio_t *rp) {
	char uri[MAXLINE], req_type[MAXLINE], http[MAXLINE], leftover[MAXLINE], nohttp[MAXLINE], line[MAXLINE], path[MAXLINE],
		port[MAXLINE], host[MAXLINE], request[MAX_OBJECT_SIZE];

	if(rio_readlineb(rp, line, MAXLINE) < 0){
		perror("error reading request line in parse_request");
		return -1;
	}

	// Parse the request
	sscanf(line, "%s %s %s", req_type, uri, http);

	if(strcmp(req_type, "GET") != 0) {
		perror("only GET is supported\n");
		return -1;
	}

	// Loop only executes once, it eliminates need for GOTO. 
	// localhost case
	if(uri[0] == '/') {
		// Set default port, might be changed later
		strcpy(port, "80");
		strcpy(path, uri);
	}
	
	else {
		if(sscanf(uri, "http://%s", nohttp) != 1)
			sscanf(uri, "%s", nohttp);
		sscanf(nohttp, "%[^/]%s", leftover, path);
		if(sscanf(leftover, "%[^:]:%s", host, port) == 1) {
			port[0] = '8';
			port[1] = '0';
		}
	}
	
	strcpy(request, "");
	sprintf(request, "%s %s HTTP/1.0\r\n", req_type, path);

	// Host is not defined, so read headers to obtain the host.
	// Read request headers
	int flag = 0;
	while(strcmp(line, "\r\n")) {
		if(rio_readlineb(rp, line, MAXLINE) < 0) {
			perror("error reading a request line from client");
			return -1;
		}
		if(!strcmp(line, "\r\n"))
			break;
		// If the request contains a custom host, then set our host flag
		if(strstr(line, "Host:")) {
			if(strstr(line, "Host: localhost")) {
				sscanf(line, "Host: localhost:%s", port);
				strcpy(host, "localhost");
			}
			
			flag = 1;
		}
		// If the line is not a custom header
		if(!strstr(line, "User-Agent:") && !strstr(line, "Proxy-Connection:") && !strstr(line, "Connection:"))
			strcat(request, line);
	}

	// Client did not send a custom host, so use default
	if(!flag) {
		sprintf(line, "Host: %s\r\n", host);
		strcat(request, line);
	}

	// Add the custom headers
	strcat(request, user_agent_hdr);
	strcat(request, prox_conn);
	strcat(request, conn);
	strcat(request, "\r\n");

	printf("host: %s\n", host);
	printf("port: %s\n", port);
	printf("path: %s\n", path);


	printf("%s", request);

	// Open the server connection
	int sfd = open_clientfd(host, port);
	if(sfd < 0) {
		perror("error opening up connection to server\n");
		return -1;
	}

	printf("server connected\n");

	if(rio_writen(sfd, request, strlen(request)) < 0){
		perror("error sending the header to server");
		return -1;
	}
	printf("Request forwarded\n");
	return sfd;
}

// Foward the response data from rp (a server) to cfd (the client)
int foward_response(rio_t *rp, int cfd) {
	char line[MAXLINE], response[MAXLINE], content_line[MAX_OBJECT_SIZE], content[MAX_OBJECT_SIZE * 10];

	strcpy(response, "");

	// Recieve response header from server
	int line_size = 0, response_size = 0, content_size = 0;
	while((line_size = rio_readlineb(rp, line, MAXLINE)) != 0) {
		strcat(response, line);
		response_size += line_size;

		// Additional loop guard, after the line was added
		if(!strcmp(line, "\r\n"))
			break;
	}

	// Write reponse header to client
	if(rio_writen(cfd, response, response_size) < 0) {
		perror("error writing response header to client");
		return -1;
	}

	strcpy(content, "");

	// Receive content from server
	// Need to use a different method of obtaining the content because it may be binary
	while((line_size = rio_readnb(rp, content_line, MAX_OBJECT_SIZE)) != 0) {
		memcpy(content + content_size, content_line, line_size);
		content_size += line_size;
	}

	// Write reponse content to client
	if(rio_writen(cfd, content, content_size) < 0) {
		perror("error writing response content to client");
		return -1;
	}

	printf("Response forwarded\n");
	return 0;
}
