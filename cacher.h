#include "csapp.h"

#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

// Content node, where content will be stored
struct node {
	char uri[MAXLINE];
    char content[MAX_OBJECT_SIZE];
	int content_size;
	int time;
};

int cache_find(char *uri);
int cache_add(char *data, int size, char *uri);
void cache_init();
void cache_destroy();

int total_size;
int global_time;

struct node *(list[5]);

pthread_mutex_t mutex;