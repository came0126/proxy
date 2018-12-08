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

// struct node {
// 	struct node *next;
// 	char key[50];
// 	char val[MAXLINE];
// };

int parse(int cfd);
// char *get_node(char key[], struct node *head);
// void add_node(char key[], char val[], struct node *head);
// void free_list(struct node *node);
// int send_request(int sfd, struct node *head);

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:56.0) Gecko/20100101 Firefox/56.0\r\n";

int main(int argc, char **argv) {
	signal(SIGPIPE, SIG_IGN);

	int cfd;
	char buf[MAXLINE], *port;
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



	//struct node *head = malloc(sizeof(struct node));
	int sfd = parse(cfd);

	// Lines added to header linked list. Now send the request to the server
	close(sfd);
	close(cfd);
	close(fd);
	return 0;
}

/** Parse the request from the client, and return the server's fd supporting only GET commands **/
int parse(int cfd) {
	char uri[MAXLINE], req_type[MAXLINE], http[MAXLINE], path[MAXLINE], leftover[MAXLINE], host[MAXLINE], nohttp[MAXLINE], port[5], line[MAXLINE];

	rio_t rp;
	Rio_readinitb(&rp, cfd);
	Rio_readlineb(&rp, line, MAXLINE);
	printf("\nRequest:\n");
	// Parse the request
	sscanf(line, "%s %s %s", req_type, uri, http);
	if(sscanf(uri, "http://%s", nohttp) != 1)
		sscanf(uri, "%s", nohttp);
	sscanf(nohttp, "%[^/]%s", leftover, path);
	if(sscanf(leftover, "%[^:]:%s", host, port) == 1) {
		port[0] = '8';
		port[1] = '0';
	}

	if(strcmp(req_type, "GET") != 0) {
		printf("ONLY GET is supported\n");
		exit(2);
	}
	
	// Open the server connection
	int sfd = open_clientfd(host, port);
	if(sfd < 0) {
		perror("Error with open_clientfd\n");
		exit(2);
	}

	// Forward request header to server
	char parsed_line[MAXLINE];
	sprintf(parsed_line, "%s %s HTTP/1.0\r\n", req_type, path);
	printf("%s", parsed_line);
	if(rio_writen(sfd, parsed_line, strlen(parsed_line)) < 0) {
		perror("cannot write request header to server");
		exit(2);
	}

	// Read and send the request headers
	Rio_readlineb(&rp, line, MAXLINE);
	while(strcmp("\r\n", line)) {
		// Special line cases
		char tmp[MAXLINE], tmp2[MAXLINE];
		char *str_prox = "Proxy-Connection: close\r\n";
		char *str_conn = "Connection: close\r\n";
		sscanf(line, "%s: %s", tmp, tmp2);
		if(!strcmp(tmp, "Proxy-Connection:"))
			sprintf(line, "%s", str_prox);
		else if(!strcmp(tmp, "Connection"))
			sprintf(line, "%s", str_conn);
		else if(!strcmp(tmp, "User-Agent:"))
			sprintf(line, "%s", user_agent_hdr);

		printf("%s", line);

		// Send header lines one by one
		if(rio_writen(sfd, line, strlen(line)) < 0) {
			perror("Error with sending the request header\n");
			exit(2);
		}

		Rio_readlineb(&rp, line, MAXLINE);
	}
        if(rio_writen(sfd, line, strlen(line)) < 0) {
        	perror("Error with sending the request header\n");
                exit(2);
        }	

	/** NEED TO RECEIVE HEADERS FROM SERVER **/
	printf("Response:\n");
	// Receive response from server
	Rio_readinitb(&rp, sfd);
	while(1) {		
		if((rio_readlineb(&rp, line, MAXLINE)) < 0) {
                        perror("Error writing response header to client");
                        exit(2);
                }

		printf("%s", line);

		if(rio_writen(cfd, line, strlen(line)) < 0) {
			perror("Error writing response header to client");
                	exit(2);
		}
		if(strcmp(line, "\r\n"))
                        break;
	}

	// Receive content from server
	char content[MAX_OBJECT_SIZE];
	int line_size = 0;
	int cont_size = 0;
	while((line_size = rio_readnb(&rp, line, MAX_OBJECT_SIZE)) != 0) {
		memcpy(content + cont_size, line, line_size * sizeof(char));
		cont_size += line_size;
	}

        if(rio_writen(cfd, content, cont_size) < 0) {
                perror("Error writing response content to client");
                exit(2);
        }       

	return sfd;
}

// char *get_node(char key[], struct node *head) {
// 	if(strcmp(head->key, key))
// 		return head->val;

// 	struct node *next = head->next;
// 	while(next != NULL) {
// 		if(strcmp(next->key, key))
// 			return head->val;
// 		next = next->next;
// 	}

// 	return NULL;
// }

// // Add a node to the linkedlist, assumed that head is not null
// void add_node(char key[], char val[], struct node *head) {
// 	if(head->next == NULL) {
// 		strcpy(head->val, val);
// 		strcpy(head->key, key);
// 		return;
// 	}
// 	struct node *next = head->next;
// 	struct node *curr = head;
// 	while(next != NULL) {
// 		if(strcmp(curr->key, key))
// 			return;
// 		next = next->next;
// 	}

// 	curr->next = malloc(sizeof(struct node));
// 	strcpy(curr->next->val, val);
// 	strcpy(curr->next->key, key);
// 	return;
// }

// void free_list(struct node *node) {
// 	if(node == NULL)
// 		return;
// 	free_list(node->next);
// 	free(node->val);
// 	free(node->key);
// 	free(node);
// 	return;
// }

// /** Send a request header to the server fd */
// int send_request(int sfd, struct node *head) {
// 	//sprintf(header, "GET %s HTTP/1.0\r\nHost: %s\r\n%s\r\nConnection: close\r\nProxy-Connection: close\r\n\r\n", path, host, user_agent_hdr);

// 	struct node *next = head;
// 	char line[MAXLINE];
// 	while(next != NULL) {
// 		sprintf(line, "%s: %s", next->key, next->val);
// 		printf("printtttting lineee%s\n", line);
// 		// Send the header to the server
// 		if(rio_writen(sfd, line, strlen(line)) < 0) {
// 			perror("Error with send\n");
// 			exit(2);
// 		}
// 		next = next->next;
// 	}

// 	close(sfd);
// 	return 0;
// }
