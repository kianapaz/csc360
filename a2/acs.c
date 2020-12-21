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

// Creating a queue for the clerks
typedef struct clerk {
    int id;
    int in_use;
} clerk;

// Global Vars for the values of tracking the customers
// tracking the customer and queue
customer customer_info[256];
clerk clerk_info[256];
// Stores the customers that are waiting
customer* customer_queue[256];
customer* econ_queue[256];
customer* bus_queue[256];
clerk* clerks_queue[256];


// Variables to track length, start and clerks in use
struct timeval start;

// Variables to track average waiting times;
double economy_waiting_time = 0;
double business_waiting_time = 0;
double overall_waiting_time = 0;
int ecol = 0;
int busl = 0;

int business = 0;
int economy = 0;

int queue_length = 0;
int econ_len = 0;
int bus_len = 0;

int queue_status;
int winner_selected = 0;
int queue_id = 0;

// If clerk is:0, then the clerk is free
// If clerk is:1, then the clerk is busy
int clerk_in_use = 0;

// Threads to track thread/mutex/convar
// For however many customers, there is a thread for each customer
pthread_t customer_threads[256];
pthread_t clerk_threads[256];
pthread_mutex_t mutex1;
pthread_mutex_t mutex2;
pthread_cond_t convar1;
pthread_cond_t convar2;


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
 * Returns the time difference in microseconds between current time and start of serving customer
*/
float getTimeDifference() {
	struct timeval curr_time;
	gettimeofday(&curr_time, NULL);
	long curr_micro = (curr_time.tv_sec * 10 * 100000) + curr_time.tv_usec;
	long start_micro = (start.tv_sec * 10 * 100000) + start.tv_usec;
    return (float)(curr_micro - start_micro) / (10 * 100000);
}


void* customer_work(customer* curr_customer, customer** queue, int *len, pthread_mutex_t mutex, pthread_cond_t convar){

    
    // Wait for arrival of a customer (converted from deciseconds to microseconds)
	usleep(curr_customer->arrival_time * 100000);
    printf("A customer arrives: customer ID %2d. \n", curr_customer->id);

    
    if (pthread_mutex_lock(&mutex) != 0) {
        printf("Failed to lock mutex. \n");
        exit(1);
    }

    
    printf("A customer enters a queue: the queue ID %1d, and length of the queue %2d. \n", queue_id, *len);
    // Insert the given customer into the queue
    queue[*len] = curr_customer;
    len++;
    
    // Sort the queue to ensure the top priority is First
	sorting_queue(queue, *len);
    
    float start_time = getTimeDifference();


    while (queue[0]->id != curr_customer->id) {
		if (pthread_cond_wait(&convar, &mutex) != 0) {
			printf("Failed to wait for convar.\n");
			exit(1);
		}

        // Removing the customer who is done
        for (int i = 0; i < *len-1; i++) {
            queue[i] = queue[i+1];
        }
        *len -= 1;
        winner_selected = 1;
        
	}

    if (pthread_mutex_unlock(&mutex) != 0) {
        printf("Failed to unlock mutex. \n");
        exit(1);
    }

    //printf("queue status in customers %d \n", queue_status);
    int clerk_use = queue_status;
    printf("A clerk starts serving a customer: start time %.2f, the customer ID %2d, the clerk ID %1d. \n", start_time, curr_customer->id, clerk_use);
    // Sleep for transmission time (converted from deciseconds to microseconds)
    usleep(curr_customer->service_time * 100000);
    
    //queue_status = 0;

    float end_time = getTimeDifference();
    printf("A clerk finishes serving a customer: end time %.2f, the customer ID %2d, the clerk ID %1d. \n", end_time, curr_customer->id, clerk_use);

    
    // Adding up the time it took to help customers
    overall_waiting_time += (end_time - start_time);

    if(curr_customer->priority == 0){
        economy_waiting_time += (end_time - start_time);
        ecol++;
        memcpy(queue, econ_queue, sizeof(*queue));
        econ_len = *len;

    }else if (curr_customer->priority == 1){
        business_waiting_time += (end_time - start_time);
        busl++;
        memcpy(queue, bus_queue, sizeof(*queue));
        bus_len = *len;

    }

    
    
    

    //pthread_exit(NULL);

    //return NULL;
    return 0;
}
/*
 * Handling the schedule for customers
 * Given a customer, idle if no customer, then give a clerk a customer to serve
 */
void* customer_entry(void* customer_item) {

    customer* curr_customer = (customer*)customer_item;

    int *i;
    i = &queue_id;

    printf("QUEUE Id %d and CLERK ID %d\n", queue_id, queue_status);
    if(curr_customer->priority == 1){
        
        *i = 1;
        //queue_id = 1;
        customer_work(curr_customer, bus_queue, &bus_len, mutex1, convar1);
        
    } else if(curr_customer->priority == 0){
        *i =0;
        //queue_id = 0;
        customer_work(curr_customer, econ_queue, &econ_len, mutex2, convar2);
        
    }
    if (pthread_cond_signal(&convar1) != 0) {
        printf("Failed to signal convar. IN ENTRY\n");
        exit(1);
    }
    pthread_exit(NULL);

    return NULL;

  
}

void* clerk_entry(void* clerk_num){
    
    while(1){
        // Clerk is currently idle
        clerk* curr_clerk = (clerk*)clerk_num;

        printf("The queue ID %d\n\n", queue_id);

        if(queue_id == 1){
            if (pthread_mutex_lock(&mutex1) != 0) {
                printf("Failed to lock mutex. IN ENTRY\n");
                exit(1);
            }
        
            // Insert the Queue status
            queue_status = curr_clerk->id;
            printf("ACTUAL clerk ID %d\n\n", curr_clerk->id);
            
            if (pthread_cond_broadcast(&convar1) != 0) {
                printf("Failed to broadcast. IN ENTRY\n");
                exit(1);
            }

            // Winning queue (next priority person selected
            winner_selected = queue_id;
        
            if (pthread_mutex_unlock(&mutex1) != 0) {
                printf("Failed to unlock mutex. IN ENTRY\n");
                exit(1);
            }
            
            if (pthread_cond_wait(&convar1, &mutex1) != 0) {
                printf("Failed to wait convar. IN ENTRY\n");
                exit(1);
            }

        } else if(queue_id == 0){
            if (pthread_mutex_lock(&mutex2) != 0) {
                printf("Failed to lock mutex. IN ENTRY\n");
                exit(1);
            }
        
            // Insert the Queue status
            queue_status = curr_clerk->id;
            printf("STATUS QUEUE FUCKKKKKKK %d\n\n", queue_status);
            if (pthread_cond_broadcast(&convar2) != 0) {
                printf("Failed to broadcast. IN ENTRY\n");
                exit(1);
            }

            // Winning queue (next priority person selected
            winner_selected = queue_id;
        
            if (pthread_mutex_unlock(&mutex2) != 0) {
                printf("Failed to unlock mutex. IN ENTRY\n");
                exit(1);
            }
            
            if (pthread_cond_wait(&convar2, &mutex2) != 0) {
                printf("Failed to wait convar. IN ENTRY\n");
                exit(1);
            }
            
            
        }
            
       
    }
    //printf("AM I STUCK IN clerks oh someone please please help me \n");
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
            economy++;
        }else{
            customer curr_bus = {
                bus_queue[0],
                bus_queue[1],
                bus_queue[2],
                bus_queue[3]
            };
            customer_info[i] = curr_bus;
            business++;
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
		printf("Failed to read customers file. Check if file exists.\n");
		exit(1);
	}

	// Parse input into array of customers
	int num_customer = atoi(fileContents[0]);
	parsing(fileContents, num_customer);

	// Initialize mutex and convar
	if (pthread_mutex_init(&mutex1, NULL) != 0) {
		printf("Error: failed to initialize mutex. \n");
		exit(1);
	}
    if (pthread_mutex_init(&mutex2, NULL) != 0) {
		printf("Error: failed to initialize mutex. \n");
		exit(1);
	}
	if (pthread_cond_init(&convar1, NULL) != 0) {
		printf("Error: failed to initialize conditional variable. \n");
		exit(1);
	}
    if (pthread_cond_init(&convar2, NULL) != 0) {
		printf("Error: failed to initialize conditional variable. \n");
		exit(1);
	}

	
	gettimeofday(&start, NULL);

	// Creating a thread for each customer that arrives
	for (int i = 0; i < num_customer; i++) {
		if (pthread_create(&customer_threads[i],  NULL, customer_entry, (void*)&customer_info[i]) != 0){
			printf("Failed to create pthread for customers.\n");
			exit(1);
		}
	}

    for(int i = 1; i < 5; i++){
        int clerk_queue[2];
        clerk curr_clerk = {
			clerk_queue[0] = i, 
			clerk_queue[1] = 0 
		};
        clerk_info[i-1] = curr_clerk;
    }
    // Creating a clerk thread
    for(int i = 0; i < 4; i++){
        if (pthread_create(&clerk_threads[i],  NULL, clerk_entry, (void*)&clerk_info[i]) != 0){
			printf("Failed to create pthread for clerks. \n");
			exit(1);
		}
        
    }


	// Waiting for all threads to terminate
	for (int i = 0; i < num_customer; i++) {
		if (pthread_join(customer_threads[i], NULL) != 0) {
			printf("Failed to join pthread.\n");
			exit(1);
		}
	}

    for (int i = 0; i < 4; i++) {
		if (pthread_join(clerk_threads[i], NULL) != 0) {
			printf("Failed to join pthread.\n");
			exit(1);
		}
	}

    

	// Destroy mutex and convar in case it wasn't probably terminated
    if (pthread_mutex_destroy(&mutex1) != 0) {
		printf("Failed to destroy mutex.\n");
		exit(1);
	}
    if (pthread_mutex_destroy(&mutex2) != 0) {
		printf("Failed to destroy mutex.\n");
		exit(1);
	}
	if (pthread_cond_destroy(&convar1) != 0) {
		printf("Failed to destroy convar.\n");
		exit(1);
	}
    if (pthread_cond_destroy(&convar2) != 0) {
		printf("Failed to destroy convar.\n");
		exit(1);
	}

    printf("ECOL %d ECON %d BUSL %d BUSSINNESS %d \n", ecol, economy, busl, business);
    printf("The average waiting time for all customers in the system is: %.2f seconds. \n", overall_waiting_time/(ecol+busl));
    printf("The average waiting time for all business-class customers is: %.2f seconds. \n", economy_waiting_time/ecol);
    printf("The average waiting time for all economy-class customers is: %.2f seconds. \n", business_waiting_time/busl);
	//pthread_exit(NULL);

	return 0;
}


