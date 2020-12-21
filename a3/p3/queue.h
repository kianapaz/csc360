
/*
 * Creating the struct queue
 */ 
#define MAX_STR 60
struct Queue{
    int front, rear, size;
    unsigned int capacity;
    char *array;
};

/*
 * Basic Queue Functions
 */ 
struct Queue* createQueue(unsigned capacity);
struct Queue* resizeQueue(struct Queue* queue);
int isFull(struct Queue* queue);
int isEmpty(struct Queue* queue);
struct Queue* enqueue(struct Queue* queue, char * item);
char * dequeue(struct Queue* queue);
char * front(struct Queue* queue);
char * rear(struct Queue* queue);
