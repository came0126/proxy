#include "csapp.h"

#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* linked list node */
struct node {
	char uri[MAXLINE];
    char content[MAX_OBJECT_SIZE];
	int content_size;
	int time;
	struct node *next; 
};

struct node *cache_find(char *uri);
int cache_add(char *data, int size, char *uri);
void cache_remove();
void cache_init();
void cache_destroy();

struct node *head;
int total_size;
int global_time;

pthread_mutex_t mutex;