#include "cacher.h"

void cache_init() {
	*list = malloc(5*sizeof(struct node));
	total_size = 0;
	global_time = 0;
}

// Returns the index in the list of cache objects of the requests uri
//	-1 if the site has not been cached yet
int cache_find(char *uri) {
	pthread_mutex_lock(&mutex);
	int i;
	for(i = 0; i < 5; i++) {
		if(list[i] != NULL) {
			if(!strcmp(list[i]->uri,uri)) {
				list[i]->time = ++global_time;
				return i;
			}
		}
	}
	return -1;
	pthread_mutex_unlock(&mutex);
}

// Returns the index that data was added at.
int cache_add(char *data, int size, char *uri) {
	int i;
	if((i = cache_find(uri)) >= 0)
		return i;

	else if(size+total_size > MAX_CACHE_SIZE){
		printf("cache not big enough to store this content! size: %d\n", size);
		return -1;
	}

	pthread_mutex_lock(&mutex);
	printf("we made it boss\n");
	// Nothing in list yet, so add as first element
	if(list[0] == NULL) {
		list[0] = malloc(sizeof(struct node));
		list[0]->time = ++global_time;
		list[0]->content_size = size;
		strcpy(list[0]->uri, uri);
		strcpy(list[0]->content, data);
		total_size += size;
	}

	// Find the last recently used index
	int last_index = 0;
	for(i = 0; i < 5; i++) {
		// Keep trying to find last used index
		if(list[i] != NULL) {
			if(list[i]->time > list[last_index]->time)
				last_index = i;
		}
		// Cache is not full yet, so insert at i
		else {
			last_index = i;
			break;
		}
	}

	total_size += size;

	// Empty slot in cache!
	if(list[last_index] == NULL)
		list[last_index] = malloc(sizeof(struct node));

	else
		total_size -= list[last_index]->content_size;

	// Add the new cached object
	list[last_index]->time = ++global_time;
	list[last_index]->content_size = size;
	strcpy(list[last_index]->uri, uri);
	strcpy(list[last_index]->content, data);
	pthread_mutex_unlock(&mutex);
	return last_index;
}

void cache_destroy() {
	int i;
	for(i = 0; i < 5; i++) {
		if(list[i] != NULL) {
			free(list[i]);
		}
	}
	free(list);
}








