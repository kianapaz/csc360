/*
 * Univeristy of Victoria
 * Csc 360 Assignemnt 2
 * Kiana Pazdernik
 * V00896924
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

// Creating a Queue to track the given criteria for the Customer
typedef struct customer {
    int id;
    float arrival_time;
    float service_time;
    int priority;

} customer;

// Global Vars for the values of tracking the customers
// tracking the customer and queue
customer customers[256];
customer* queue[256];

// Variables to track length, start and clerks in use
struct timeval start;
int queue_length = 0;
int clerk_in_use = 0;

// Threads to track thread/mutex/convar
pthread_t threads[256];
pthread_mutex_t mutex;
pthread_cond_t convar;

/*
 * Given the input file, read the contents of customer
 */
int reading_customerfile(char* filepath, char filecontents[1024][1024]) {
	FILE* infile = fopen(filepath, "r");

    if (infile == NULL) {
        return 1;
    } else {
        int i = 0;
        while (fgets(filecontents[i], 1024, infile)) {
            i++;
        }
        fclose(infile);
        return 0;
    }
}

/*
 * Comparing the inputted cust1 to cust2 and finding the top priority
 */
int comparing_customers(customer* customer1, customer* customer2) {
	
    if (customer1->priority > customer2->priority) {
		return 1;
	}else if (customer1->arrival_time > customer2->arrival_time) {
		return 1;
	}else if (customer1->service_time > customer2->service_time) {
		return 1;
	}else if (customer1->id > customer2->id) {
		return 1;
	}else if (customer1->priority < customer2->priority) {
		return -1;
	}else if (customer1->arrival_time < customer2->arrival_time) {
		return -1;
	}else if (customer1->service_time < customer2->service_time) {
		return -1;
	}else if (customer1->id < customer2->id) {
		return -1;
	}else {
		// Should never get here
		printf("Failed to sort the customer criteria.\n");
		return 0;
	}
}

/*
	Sorts queue in place using Bubblesort
	(sometimes it's worth sacrificing running time to save programmer time)
*/
void sorting_queue() {

	int startingIndex;
	if (clerk_in_use) {
		startingIndex = 1;
	} else {
		startingIndex = 0;
	}
	for (int x = startingIndex; x < queue_length; x++) {
		for (int y = startingIndex; y < queue_length-1; y++) {
			if (comparing_customers(queue[y], queue[y+1]) == 1) {
				customer* temp = queue[y+1];
				queue[y+1] = queue[y];
				queue[y] = temp;
			}
		}
	}
}

/*
	f: a flow* to insert
	Inserts f into queue
*/
void insert_to_queue(customer* temp) {
	queue[queue_length] = temp;
	queue_length += 1;
}

/*
	Removes the least recently inserted flow from the queue
*/
void remove_queue() {
	int x = 0;
	while (x < queue_length-1) {
		queue[x] = queue[x+1];
		x += 1;
	}
	queue_length -= 1;
}

/*
	f: a flow
	Acquires the transmission clerk
*/
void clerk_request(customer* f) {
	if (pthread_mutex_lock(&mutex) != 0) {
		printf("Error: failed to lock mutex\n");
		exit(1);
	}

	insert_to_queue(f);
	sorting_queue();

	if (f->id != queue[0]->id) {
		printf("Customer %2d waits for the finish of flow %2d. \n", f->id, queue[0]->id);
	}
	while (queue[0]->id != f->id) {
		if (pthread_cond_wait(&convar, &mutex) != 0) {
			printf("Error: failed to wait for convar\n");
			exit(1);
		}
	}
	clerk_in_use = 1;

	if (pthread_mutex_unlock(&mutex) != 0) {
		printf("Error: failed to unlock mutex\n");
		exit(1);
	}
}

/*
	f: a flow
	Releases the transmission clerk
*/
void clerk_release(customer* temp) {
	if (pthread_mutex_lock(&mutex) != 0) {
		printf("Error: failed to lock mutex\n");
		exit(1);
	}

	if (pthread_cond_broadcast(&convar) != 0) {
		printf("Error: failed to broadcast\n");
		exit(1);
	}

	remove_queue();
	clerk_in_use = 0;

	if (pthread_mutex_unlock(&mutex) != 0) {
		printf("Error: failed to unlock mutex\n");
		exit(1);
	}
}

/*
	Returns the time difference in microseconds between now and start
*/
float getTimeDifference() {
	struct timeval nowTime;
	gettimeofday(&nowTime, NULL);
	long nowMicroseconds = (nowTime.tv_sec * 10 * 100000) + nowTime.tv_usec;
	long startMicroseconds = (start.tv_sec * 10 * 100000) + start.tv_usec;
	return (float)(nowMicroseconds - startMicroseconds) / (10 * 100000);
}

/*
	customer_item: a flow struct to schedule
	Entry point for each thread created. Handles flow scheduling.
*/
void* threading(void* customer_item) {
	customer* f = (customer*)customer_item;

	// Wait for arrival (converted from deciseconds to microseconds)
	usleep(f->arrival_time * 100000);
	printf("Customer %2d arrives: arrival time (%.2f), transmission time (%.1f), priority (%2d). \n", f->id, getTimeDifference(), f->service_time * 0.1, f->priority);

	clerk_request(f);

	// Sleep for transmission time (converted from deciseconds to microseconds)
	printf("Customer %2d starts its transmission at time %.2f. \n", f->id, getTimeDifference());
	usleep(f->service_time * 100000);
	printf("Customer %2d finishes its transmission at time %.1f. \n", f->id, getTimeDifference());

	clerk_release(f);

	pthread_exit(NULL);
}


/*
	fileContents: an array of strings containing user input
	num_customer: the number of flows
	Parses input into flow objects
*/
void parsing(char fileContents[1024][1024], int num_customer) {
	
	for (int i = 1; i <= num_customer; i++) {
		
        int k = 0;
        while (fileContents[i][k] != '\0') {
            if (fileContents[i][k] == ':') {
                fileContents[i][k] = ',';
            }
            k++;
        }

		int j = 0;
		int customer_queue[4];
		char* token = strtok(fileContents[i], ",");
		while (token != NULL) {
			customer_queue[j] = atoi(token);
			token = strtok(NULL, ",");
			j++;
		}

		customer f = {
			customer_queue[0],  // id
			customer_queue[1],  // arrival_time
			customer_queue[2],  // service_time
			customer_queue[3]   // priority
		};

		customers[i-1] = f;
	}
}

/* ---------- Main ---------- */

/*
	A router simulator that concurrently schedules the transmission of flows
*/
int main(int argc, char* argv[]) {
	if (argc < 2) {
		printf("Error: use as follows MFS <flows text file>\n");
		exit(1);
	}

	// Read flows text file
	char fileContents[1024][1024];
	if (reading_customerfile(argv[1], fileContents) != 0) {
		printf("Error: Failed to read flows file\n");
		exit(1);
	}

	// Parse input into array of flow*
	int num_customer = atoi(fileContents[0]);
	parsing(fileContents, num_customer);

	// Initialize mutex and conditional variable
	if (pthread_mutex_init(&mutex, NULL) != 0) {
		printf("Error: failed to initialize mutex\n");
		exit(1);
	}
	if (pthread_cond_init(&convar, NULL) != 0) {
		printf("Error: failed to initialize conditional variable\n");
		exit(1);
	}

	// Create threads in a joinable state
	pthread_attr_t attr;
	if (pthread_attr_init(&attr) != 0) {
		printf("Error: failed to initialize attr\n");
		exit(1);
	}
	if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE) != 0) {
		printf("Error: failed to set detachstate\n");
		exit(1);
	}

	gettimeofday(&start, NULL);

	// Create a thread for each flow
	for (int i = 0; i < num_customer; i++) {
		if (pthread_create(&threads[i], &attr, threading, (void*)&customers[i]) != 0){
			printf("Error: failed to create pthread.\n");
			exit(1);
		}
	}

	// Wait for all threads to terminate
	for (int i = 0; i < num_customer; i++) {
		if (pthread_join(threads[i], NULL) != 0) {
			printf("Error: failed to join pthread.\n");
			exit(1);
		}
	}

	// Destroy mutex and conditional variable
	if (pthread_attr_destroy(&attr) != 0) {
		printf("Error: failed to destroy attr\n");
		exit(1);
	}
	if (pthread_mutex_destroy(&mutex) != 0) {
		printf("Error: failed to destroy mutex\n");
		exit(1);
	}
	if (pthread_cond_destroy(&convar) != 0) {
		printf("Error: failed to destroy convar\n");
		exit(1);
	}
	pthread_exit(NULL);

	return 0;
}


