/*
 * Helper struct queue to help travserse the directories 
 * With the help from https://www.geeksforgeeks.org/queue-set-1introduction-and-array-implementation/
 */ 
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include "queue.h"

/*
 * Creates a queue given the max amount the queue can grow
 * Return the newly created queue
 */ 
struct Queue* createQueue(unsigned int capacity){
    struct Queue* queue = (struct Queue*) malloc(sizeof(struct Queue));
    if(queue == NULL){
        printf("Failed to allocate memory.\n");
		exit(1);
    }
    queue->capacity = capacity;
    queue->front = queue->size = 0;
    queue->rear = capacity-1;
    queue->array = (char*)malloc(queue->capacity * sizeof(char) * MAX_STR);
    if(queue->array == NULL){
        printf("Failed to allocate memory.\n");
		exit(1);
    }
    memset(queue->array, '\0', queue->capacity * sizeof(char) * MAX_STR);
    return queue;
}

/*
 * Expands the given queue, creates a new largre queue
 * Returns the larger queue
 */ 
struct Queue* resizeQueue(struct Queue *queue){
    struct Queue* newqueue = (struct Queue*) malloc(sizeof(struct Queue));
    if(newqueue == NULL){
        printf("Failed to allocate memory.\n");
		exit(1);
    }
    newqueue->capacity = ((queue)->capacity) * 2;
    newqueue->front = newqueue->size = 0;
    newqueue->rear = newqueue->capacity-1;
    newqueue->array = (char*)malloc(newqueue->capacity * sizeof(char) * MAX_STR);
    if(newqueue->array == NULL){
        printf("Failed to allocate memory.\n");
		exit(1);
    }
    memset(newqueue->array, '\0', newqueue->capacity * sizeof(char) * MAX_STR);
    
    char * item;
    while(!isEmpty(queue)){
        item = dequeue(queue);
        enqueue(newqueue, item);
    }
    free(queue);
    return newqueue;
}

/*
 * Checks if the queue is at max capacity
 * Returns 0 if not full, -1 full
 */  
int isFull(struct Queue* queue){
    return (queue->size == queue->capacity);
}

/*
 * Checking if the queue is empty
 * -1 = empty, 0 not empty
 */ 
int isEmpty(struct Queue* queue){
    return (queue->size == 0);
}

/*
 * Enqueue's the given variable to the queue
 * returns the updated queue
 */ 
struct Queue* enqueue(struct Queue* queue, char * item)
{
    if(isFull(queue))
        queue = resizeQueue(queue);
    (queue)->rear = ((queue)->rear + 1) % (queue)->capacity;
    char * p = (queue)->array + ((queue->rear)*MAX_STR);
    memset(p, '\0', MAX_STR);
    strcpy(p,item);
    (queue)->size = (queue)->size+1;
    return queue;
}


/*
 * removes an itme from the queue
 * returns the removed item
 */ 
char * dequeue(struct Queue* queue){
    if (isEmpty(queue)){
        return "";
    }
        
    char * item;
    item = malloc(sizeof(char)*MAX_STR);
    if(item == NULL){
        printf("Failed to allocate memory.\n");
		exit(1);
    }
    char * p = (queue)->array+((queue->front) * MAX_STR);
    memset(item, '\0', MAX_STR);
    strcpy(item,p);
    (queue)->front = ((queue)->front + 1) % (queue)->capacity;
    (queue)->size = (queue)->size - 1;
    return item;
}

/*
 * Checks the front of the queue
 * returns the item at the front of the queue
 */ 
char * front(struct Queue* queue)
{
    if(isEmpty(queue)){
        return "";
    }
        
    char * item;
    item = malloc(sizeof(char) * MAX_STR);
    if(item == NULL){
        printf("Failed to allocate memory.\n");
		exit(1);
    }
    char * p = queue->array + ((queue->front) * MAX_STR);
    memset(item, '\0', MAX_STR);
    strcpy(item, p);
    return item;
}

/*
 *Checks an item at the rear of the queue
 * returns the rear item
 */ 
char * rear(struct Queue* queue){
    if(isEmpty(queue)){
        return "";
    }
        
    char * item;
    item = malloc(sizeof(char) * MAX_STR);
    if(item == NULL){
        printf("Failed to allocate memory.\n");
		exit(1);
    }
    char * p = queue->array + ((queue->rear) * MAX_STR);
    memset(item, '\0', MAX_STR);
    strcpy(item, p);
    return item;
}