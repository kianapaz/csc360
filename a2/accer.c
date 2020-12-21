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
    int priority;
    float arrival_time;
    float service_time;
    

} customer;



// Global Vars for the values of tracking the customers
// tracking the customer and queue
customer customer_info[256];

// Stores the customers that are waiting
customer* customer_queue[256];
customer* econ_queue[256];
customer* bus_queue[256];



// Variables to track length, start and clerks in use
struct timeval start;

// Variables to track average waiting times;
double economy_waiting_time = 0;
double business_waiting_time = 0;
double overall_waiting_time = 0;
int ecol = 0;
int busl = 0;


int queue_length = 0;
int econ_len = 0;
int bus_len = 0;
// If clerk is:0, then the clerk is free
// If clerk is:1, then the clerk is busy
int clerk_in_use = 0;

// Threads to track thread/mutex/convar
// For however many customers, there is a thread for each customer
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
 * Compares priority 1 business, 0 economy, 1>0
 * If priority is the same, compare arrival time, if that is the same
 * Compare serving time, choose the smaller one first, 
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
		printf("Failed to sort the customer criteria.\n");
		return 0;
	}
}

/* 
 * Using bubblesort to sort the queue of customers
 */ 
void sorting_queue(customer** queue, int queue_len) {

	int startingIndex;
	if (clerk_in_use) {
		startingIndex = 1;
	} else {
		startingIndex = 0;
	}
	for (int x = startingIndex; x < queue_len; x++) {
		for (int y = startingIndex; y < queue_len-1; y++) {
			if (comparing_customers(queue[y], queue[y+1]) == 1) {
				customer* temp = queue[y+1];
				queue[y+1] = queue[y];
				queue[y] = temp;
			}
		}
	}
}



/*
 * Requesting a clerk for the given customer
 */ 
void clerk_request(customer* next_customer, customer** given_q, int *given_len) {

    // Checking if the mutex can be locked
	if (pthread_mutex_lock(&mutex) != 0) {
		printf("Failed to lock mutex. \n");
		exit(1);
	}

	// Insert the given customer into the queue
    given_q[*given_len] = next_customer;
    given_len++;

    // Sort the queue to ensure the top priority is First
	sorting_queue(given_q, *given_len);

    
	while (given_q[0]->id != next_customer->id) {
		if (pthread_cond_wait(&convar, &mutex) != 0) {
			printf("Failed to wait for convar.\n");
			exit(1);
		}
	}
    // Setting clerk to in use
	clerk_in_use = 1;

	if (pthread_mutex_unlock(&mutex) != 0) {
		printf("Failed to unlock mutex.\n");
		exit(1);
	}

}

/*
 * 
 * The customer is done being served
 * Release the clerk back for customers help
 */ 
void clerk_release(customer* done_customer, customer** given_q, int *given_len) {
	if (pthread_mutex_lock(&mutex) != 0) {
		printf("Failed to lock mutex. \n");
		exit(1);
	}

	if (pthread_cond_broadcast(&convar) != 0) {
		printf("Failed to broadcast. \n");
		exit(1);
	}

    // Removing the customer who is done
    for (int i = 0; i < *given_len-1; i++) {
		*given_q[i] = *given_q[i+1];
	}
	*given_len -= 1;

    // Releasing the clerk - setting to 0 for free
	clerk_in_use = 0;

	if (pthread_mutex_unlock(&mutex) != 0) {
		printf("Failed to unlock mutex. \n");
		exit(1);
	}
}

/*
 * Returns the time difference in microseconds between current time and start of serving customer
*/
float getTimeDifference() {
	struct timeval curr_time;
	gettimeofday(&curr_time, NULL);
	long curr_micro = (curr_time.tv_sec * 10 * 100000) + curr_time.tv_usec;
	long start_micro = (start.tv_sec * 10 * 100000) + start.tv_usec;
    return (float)(curr_micro - start_micro) / (10 * 100000);
}

/*
 * Handling the schedule for customers
 * Given a customer, idle if no customer, then give a clerk a customer to serve
 */
void* scheduler(void* customer_item) {
	customer* curr_customer = (customer*)customer_item;
    
	// Wait for arrival of a customer (converted from deciseconds to microseconds)
	usleep(curr_customer->arrival_time * 100000);
	
    //printf("A customer arrives: customer ID %2d. arrival time (%.2f), clerk time (%.1f), priority (%2d). \n", f->id, getTimeDifference(), f->service_time * 0.1, f->priority);
    printf("A customer arrives: customer ID %2d. \n", curr_customer->id);

    int q;
	if(curr_customer->priority == 0){
        clerk_request(curr_customer, econ_queue, &econ_len);
        q = 0;
        printf("A customer enters a queue: the queue ID %1d, and length of the queue %2d. \n", q, econ_len);
    }else{
        clerk_request(curr_customer, bus_queue, &bus_len);
        q = 1;
        printf("A customer enters a queue: the queue ID %1d, and length of the queue %2d. \n", q, bus_len);
    }
    
    
    
    float start_time = getTimeDifference();
    printf("A clerk starts serving a customer: start time %.2f, the customer ID %2d, the clerk ID %1d. \n", start_time, curr_customer->id, 1);
	// Sleep for transmission time (converted from deciseconds to microseconds)
    usleep(curr_customer->service_time * 100000);

    float end_time = getTimeDifference();
    printf("A clerk finishes serving a customer: end time %.2f, the customer ID %2d, the clerk ID %1d. \n", end_time, curr_customer->id, 1);

    
    // Adding up the time it took to help customers
    overall_waiting_time += (end_time - start_time);

    // Releasing the clerk
    if(curr_customer->priority == 0){
        clerk_release(curr_customer, econ_queue, &econ_len);
        economy_waiting_time += (end_time - start_time);
        ecol++;

    }else if (curr_customer->priority == 1){
        clerk_release(curr_customer, bus_queue, &bus_len);
        business_waiting_time += (end_time - start_time);
        busl++;

    }

	pthread_exit(NULL);

    return NULL;
}


/*
 * Given a file, placing the customer and the customer info together
 * Placing the customer and info into an object
 */
void parsing(char fileContents[1024][1024], int num_customer) {
	
	for (int i = 1; i <= num_customer; i++) {
		
        // Formatting the customer file into the queue
        int k = 0;
        while (fileContents[i][k] != '\0') {
            if (fileContents[i][k] == ':') {
                fileContents[i][k] = ',';
            }
            k++;
        }

		int j = 0;
		int customer_queue[4];
        int econ_queue[4] = {0,0,0,0};
        int bus_queue[4] = {0,0,0,0};
		char* token = strtok(fileContents[i], ",");
		while (token != NULL) {
			customer_queue[j] = atoi(token);
			token = strtok(NULL, ",");
			j++;
		}

        // Inserting the customer correct info
        // id, priority, arrival_time, service_time
		customer curr_customer = {
			customer_queue[0],  
			customer_queue[1],  
			customer_queue[2],  
			customer_queue[3]   
		};

		customer_info[i-1] = curr_customer;

        // Inserting the customer info into the correct queue's
        if(customer_queue[1] == 0){
            customer curr_econ = {
                econ_queue[0],
                econ_queue[1],
                econ_queue[2],
                econ_queue[3]
            };
            customer_info[i] = curr_econ;
        }else{
            customer curr_bus = {
                bus_queue[0],
                bus_queue[1],
                bus_queue[2],
                bus_queue[3]
            };
            customer_info[i] = curr_bus;
        }


	}
}


/*
 *
 * Initializing variables and getting the customers from a file
 * With the help from: 'A2-hint.c' given by CSC370-Fall2020 - Author Kui Wu
 */ 
int main(int argc, char* argv[]) {
    // Checking for the input customer file
	if (argc < 2) {
		printf("To run acs.c: ./acs customer.txt \n");
		exit(1);
	}

	// Check if there is an error reading the customers file, or if file does not exist
	char fileContents[1024][1024];
	if (reading_customerfile(argv[1], fileContents) != 0) {
		printf("Failed to read customers file. The file may not exist. Run again ./acs customer.txt \n");
		exit(1);
	}

	// Parse input into array of customers
	int num_customer = atoi(fileContents[0]);
	parsing(fileContents, num_customer);

	// Initialize mutex and convar
	if (pthread_mutex_init(&mutex, NULL) != 0) {
		printf("Error: failed to initialize mutex\n");
		exit(1);
	}
	if (pthread_cond_init(&convar, NULL) != 0) {
		printf("Error: failed to initialize conditional variable\n");
		exit(1);
	}

	
	gettimeofday(&start, NULL);

	// Creating a thread for each customer that arrives
	for (int i = 0; i < num_customer; i++) {
		if (pthread_create(&threads[i], NULL, scheduler, (void*)&customer_info[i]) != 0){
			printf("Failed to create pthread for customers.\n");
			exit(1);
		}
	}


	// Waiting for all threads to terminate
	for (int i = 0; i < num_customer; i++) {
		if (pthread_join(threads[i], NULL) != 0) {
			printf("Failed to join pthread.\n");
			exit(1);
		}
	}


    printf("The average waiting time for all customers in the system is: %.2f seconds. \n", overall_waiting_time/(ecol+busl));
    printf("The average waiting time for all business-class customers is: %.2f seconds. \n", economy_waiting_time/ecol);
    printf("The average waiting time for all economy-class customers is: %.2f seconds. \n", business_waiting_time/busl);


	// Destroy mutex and convar in case it wasn't probably terminated
	if (pthread_mutex_destroy(&mutex) != 0) {
		printf("Failed to destroy mutex.\n");
		exit(1);
	}
	if (pthread_cond_destroy(&convar) != 0) {
		printf("Failed to destroy convar.\n");
		exit(1);
	}
	pthread_exit(NULL);

	return 0;
}


