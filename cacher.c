#include "cacher.h"

// Create the cache
void cache_init() {
	head = malloc(sizeof(struct node));
	struct node *curr = head;
	int i;
	for(i = 0; i < 5; i++) {
		curr->content_size = -1;
		curr->time = -1;

		if(i != 4)
			curr->next = malloc(sizeof(struct node));
		else
			curr->next = NULL;
		curr = curr->next;
	}

	global_time = 0;
	total_size = 0;
}

// Destroy the cache, free all memory
void cache_destroy() {
	total_size = 0;
	global_time = 0;

	struct node* tmp;
	while (head != NULL) {
		tmp = head;
		head = head->next;
		free(tmp);
	}
}

// Find and return the node from the given uri, NULL if the uri has not been cached yet
struct node *cache_find(char *uri) {
	struct node *next = head;
	while(next != NULL) {
		// Found uri in cache, so update time then return node
		if(!strcmp(uri, next->uri)) {
			next->time = ++global_time;
			return next;
		}
		next = next->next;
	}

	return NULL;
}

int cache_add(char *data, int size, char *uri) {
	pthread_mutex_lock(&mutex);
	if(cache_find(uri) != NULL) {
		printf("THIS SHOULD NEVER HAPPENDSDFSDF\n\n");
		exit(2);
	}
	struct node new;
	new.time = ++global_time;
	strcpy(new.uri, uri);
	strcpy(new.content, data);
	new.content_size = size;
	new.next = NULL;

	if((total_size + size) > 1049000) {
		printf("not enough space in cache to add this website... \n");
	}

	struct node *next = head;
	struct node *nextnext = head->next;
	struct node *prev = NULL;
	struct node *prevprev = NULL;
	while(next != NULL && next->time != -1) {
		prevprev = prev;
		prev = next;
		next = next->next;
		nextnext = nextnext->next;
	}

	// Full list, so remove and reset node pointers
	if(next == NULL && prev != NULL && prev->time != -1) {
		cache_remove(new);
		next = head, prev = NULL, prevprev = NULL;
		while(next != NULL && next->time != -1) {
			prevprev = prev;
			prev = next;
			next = next->next;
		}
	}

	if(prev == NULL) {
		perror("THIS SHOULDNT HAPPEN!\n");
		exit(2);
	}

	// Otherwise add the new node to the cache
	else {
		// Tail of list
		if(next == NULL)
			prevprev->next = &new;

		// Normal case, not the tail, cache has a place so check size then add.
		else {
			new.next = nextnext;
			prev->next = &new;
		}
	}
	pthread_mutex_unlock(&mutex);
	return 0;
}

// Remove the last used node
void cache_remove() {
	pthread_mutex_lock(&mutex);
	struct node *next = head;
	struct node *last_used = head;
	int last_time = head->time;
	while(next != NULL) {
		// If a node has been used last, then set to be removed
		if(last_time > (next->time)) {
			last_time = next->time;
			last_used = next;
		}
		next = next->next;
	}
	// Remove the last used node
	total_size -= last_used->content_size;
	last_used->content_size = -1;
	last_used->time = -1;
	pthread_mutex_unlock(&mutex);
	return;
}