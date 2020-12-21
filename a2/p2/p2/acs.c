/*
 * Univeristy of Victoria
 * Csc 360 Assignemnt 2
 * Kiana Pazdernik
 * V00896924
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>


// Creating a Queue to track the given criteria for the Customer
// A linked list is used for holding the customer info
typedef struct customer {
    int id;
    int priority;
    float arrival_time;
    float service_time;
    float start_time;
    struct clerk *clerk;
    struct customer *next;
} customer;

// Creating a queue for the clerks
typedef struct clerk {
    int id;
    int status;
    struct customer *cust;
} clerk;

// Threads to track thread/mutex/convar
// For however many customers, there is a thread for each customer
// A convar for each queue, and 2 mutex's
customer *bus_queue = NULL;
customer *econ_queue = NULL;
pthread_mutex_t mutex1;
pthread_mutex_t mutex2;
pthread_cond_t convar2;
pthread_cond_t convar1;

// 4 clerk threads
clerk *clerks[4];
// Variables to track length, start and clerks in use
struct timeval start;

// Variables to track the average waiting times
double overall_waiting_time = 0;
double business_waiting_time = 0;
double economy_waiting_time = 0;

// Variables to keep track of each queue length
// Variables to help count the waiting times
int business_count = 0;
int economy_count = 0;
int num_customer = 0;
int bus_qlen = 0;
int econ_qlen = 0;


/*
 * Initializing a new customer, giving the customer the info id,priority, arrival, service, and the next customer
 * Return the created customer
 */ 
customer* create_customer(int id, int priority, int arrival_time, int service_time, customer *next) {
    customer* customer_init = (customer*)malloc(sizeof(customer));
    
    if(customer_init == NULL) {
        printf("Failed to create a customer in Linked List. \n");
        exit(1);
    }
    customer_init->id = id;
    customer_init->priority = priority;
    customer_init->arrival_time = arrival_time;
    customer_init->service_time = service_time;
    customer_init->start_time = 0;
    customer_init->clerk = NULL;
    customer_init->next = next;

    return customer_init;
}

/*
 * Using a LL to enqueue the given customer to the end of the LL
 */ 
customer* enqueue(customer* head, int id, int priority, int arrival_time, int service_time) {
    if(head == NULL) {
        customer *cst = create_customer(id, priority, arrival_time, service_time, NULL);
        return cst;
;
    }
    // Finding the tail
    customer *cursor = head;
    while(cursor->next != NULL){
        cursor = cursor->next;
    }
        
    // Creating the new customer
    customer* new_cust =  create_customer(id, priority, arrival_time, service_time, NULL);
    cursor->next = new_cust;
    return head;
}

/*
 * Remove the given customer from the queue
 */ 
customer* dequeue(customer* head) {
    if(head == NULL){
        return NULL;
    }
        
    // Freeing the head
    customer *front = head;
    if (front == head) {
        head = head->next;
        free(front);
        return head;
    }

    head = head->next;
    front->next = NULL;
    // Checking if this is the last customer to be removed
    if(front == head)
        head = NULL;
    free(front);
    return head;
}


/*
 * Returns the time difference in microseconds between current time and start of serving customer
 */
float getTimeDifference() {
	struct timeval curr_time;
	gettimeofday(&curr_time, NULL);
	long curr_micro = (curr_time.tv_sec * 1000000.0) + curr_time.tv_usec;
	long start_micro = (start.tv_sec * 1000000.0) + start.tv_usec;
    return (float)(curr_micro - start_micro) / (1000000.0);
}




/*
 * Getitng the toal amount of customers from first line
 */ 
int get_customer_count(FILE *infile, char *filename) {
    int temp_count;
    fscanf(infile, "%d", &temp_count);
    
    return temp_count;
}

/*
 * Given a file, placing the customer and the customer info together
 * Placing the customer and info into the econ or business queue given the priority
 */
void parsing(FILE *infile, customer *customers[], int customer_count) {
    char *format = "%d:%d,%d,%d";
   
    // Scanning the file for the customer info
    customer *curr;
    for(int i = 0; i < customer_count; i++) {
        int id, priority, arrival, service;
        fscanf(infile, format, &id, &priority, &arrival, &service);
        
        // Creating a customer thread with the given info
        curr = create_customer(id, priority, arrival, service, NULL);
        if (curr->priority == 1) {
            bus_queue = enqueue(bus_queue, curr->id, curr->priority, curr->arrival_time, curr->service_time);
            business_count++;
        }
        else if (curr->priority == 0) {
            econ_queue = enqueue(econ_queue, curr->id, curr->priority, curr->arrival_time, curr->service_time);
            economy_count++;
        }
        customers[i] = curr;
    }
}


/*
 * Helper function to check whenever a clerk has been freed from service and can help a new customer
 */ 
void clerk_worker(customer *curr_customer, clerk *curr_clerk, float start_time){
    
    // Getting the current time to track the waiting time
    int queue_id;
    //float current_waiting_time;
    if (curr_customer->priority == 1) {
        // While the customer is not being helped
        while (curr_customer->id != queue_id) {
           
            // Wait until signaled
            if (pthread_cond_wait(&convar1, &mutex1) != 0) {
                printf("Failed to wait for convar.\n");
                exit(1);
            }

            // Set the id to the current queue id, and add te waiting time
            if (pthread_mutex_lock(&mutex2) != 0) {
                printf("Failed to lock mutex. \n");
                exit(1);
            }
            queue_id = bus_queue->id;
            if (pthread_mutex_unlock(&mutex2) != 0) {
                printf("Failed to unlock mutex. IN ENTRY\n");
                exit(1);
            }
            //current_waiting_time = getTimeDifference();
        }
        
        

    } else if (curr_customer->priority == 0) {
        // While the customer is not being helped
        while (curr_customer->id != queue_id){

            // Wait until signaled
            if (pthread_cond_wait(&convar2, &mutex1) != 0) {
                printf("Failed to wait for convar.\n");
                exit(1);
            }

            if (pthread_mutex_lock(&mutex2) != 0) {
                printf("Failed to lock mutex. \n");
                exit(1);
            }
            // Set the id to the current queue id, and add te waiting time
            queue_id = econ_queue->id;
            if (pthread_mutex_unlock(&mutex2) != 0) {
                printf("Failed to unlock mutex. IN ENTRY\n");
                exit(1);
            }
            //current_waiting_time = getTimeDifference();
        }
        

    }
    

    // Checking the clerks for their status whether they're idle or busy
    for (int i = 0; i < 4; i++) {
        curr_clerk = clerks[i];
        if (curr_clerk->status == 0) {
            break;
        }
    }

    // Found an idle clerk
    curr_clerk->cust = curr_customer;
    // Setitng clerk to busy
    curr_clerk->status = 1;
    curr_customer->clerk = curr_clerk;

    printf("A clerk starts serving a customer: start time %.2f, the customer ID %2d, the clerk ID %1d. \n", getTimeDifference(), curr_customer->id, curr_clerk->id);
    
    // Unlocking the mutex1 from the clerk_request function
    if (pthread_mutex_unlock(&mutex1) != 0) {
        printf("Failed to unlock mutex. \n");
        exit(1);
    }

    // Locking the mutex again to remove the customer from queue since it's being helped
    if (pthread_mutex_lock(&mutex2) != 0) {
        printf("Failed to lock mutex. \n");
        exit(1);
    }
    if (curr_customer->priority == 1) {
        bus_queue = dequeue(bus_queue);
        bus_qlen--;
    } else if (curr_customer->priority == 0) {
        econ_queue = dequeue(econ_queue);
        econ_qlen--;
    }


    if (pthread_mutex_unlock(&mutex2) != 0) {
        printf("Failed to unlock mutex. IN ENTRY\n");
        exit(1);
    }
}

/*
 * Clerk function to shedule which customer is highest priority
 * and serving that customer
 */ 
void clerk_request(customer *curr_customer, float start_time) {
    
    
    //float current_waiting_time;
    if (pthread_mutex_lock(&mutex1) != 0) {
        printf("Failed to lock mutex. \n");
        exit(1);
    }
    clerk *curr_clerk;
    
    // For all the clerks
    for (int i = 0; i < 4; i++) {
        curr_clerk = clerks[i];

        // Check if a given clerk is busy, if not
        if (curr_clerk->status == 0) {
            
            // Set the clerk to busy, get the next customer to sever and set the customer's clerk to the current clerk
            curr_clerk->status = 1;
            curr_clerk->cust = curr_customer;
            curr_customer->clerk = curr_clerk;
            printf("A clerk starts serving a customer: start time %.2f, the customer ID %2d, the clerk ID %1d. \n", getTimeDifference(), curr_customer->id, curr_clerk->id);
            
            // Checking which queue the customer belongs to
            if (curr_customer->priority == 1) {
                
                // Locking the mutex to remove the current customer from the queue
                if (pthread_mutex_lock(&mutex2) != 0) {
                    printf("Failed to lock mutex. \n");
                    exit(1);
                }

                // Rmeoving the current customer
                bus_queue = dequeue(bus_queue);
                bus_qlen--;
                //current_waiting_time = getTimeDifference();
                
                
                if (pthread_mutex_unlock(&mutex2) != 0) {
                    printf("Failed to unlock mutex. \n");
                    exit(1);
                }

                
                
            } else if (curr_customer->priority == 0) {
                if (pthread_mutex_lock(&mutex2) != 0) {
                    printf("Failed to lock mutex. \n");
                    exit(1);
                }
                econ_queue = dequeue(econ_queue);
                econ_qlen--;
                //current_waiting_time = getTimeDifference();
                
                
                if (pthread_mutex_unlock(&mutex2) != 0) {
                    printf("Failed to unlock mutex. \n");
                    exit(1);
                }
                
            }
            
            if (pthread_mutex_unlock(&mutex1) != 0) {
                printf("Failed to unlock mutex. \n");
                exit(1);
            }
            // Returns, where the customer entered to wait until the end of service time to complete
            return;
        }
    }
    // The clerks are all busy, calling the worker function to wait for a clerk to become free
    clerk_worker(curr_customer, curr_clerk, start_time);
}

/* 
 * Customer entry begins the service time by finding an according clerk
 */ 
void* customer_entry(void* customer_item) {
    // Get the current customer
    customer *curr_customer = (customer*) customer_item;
    
    float start_time = getTimeDifference();

    
    // Request a clerk to serve the customer
    clerk_request(curr_customer, start_time);
    // Wait until the customer has taken the service time
    usleep(curr_customer->service_time * 100000);

    float end_time = getTimeDifference();

    // Getting the waiting times by the current customer and continuousl add
    overall_waiting_time += end_time - curr_customer->start_time;
    // Checking which priority and adding to according priority time
    if(curr_customer->priority == 0){
        business_waiting_time += end_time - curr_customer->start_time;
    }else{
        economy_waiting_time += end_time - curr_customer->start_time;
    }
    printf("A clerk finishes serving a customer: end time %.2f, the customer ID %2d, the clerk ID %1d. \n", end_time, curr_customer->id, curr_customer->clerk->id);
    
    
    if ( pthread_mutex_lock(&mutex1) != 0) {
        printf("Failed to unlock mutex. \n");
        exit(1);
    }

    
    // Setting the clerk back to idle
    curr_customer->clerk->status = 0;
    // Singaling that the customer has been served and is done
    customer *cursor = bus_queue;
    int c = 0;
    while(cursor != NULL) {
        c++;
        cursor = cursor->next;
    }
    
    if (c > 0){
        
        if (pthread_cond_signal(&convar1) != 0) {
            printf("Failed to signal convar. \n");
            exit(1);
        }
    }else{
        
        if (pthread_cond_signal(&convar2) != 0) {
            printf("Failed to signal convar. \n");
            exit(1);
        }
    }
    if (pthread_mutex_unlock(&mutex1) != 0) {
        printf("Failed to unlock mutex. \n");
        exit(1);
    }

    return 0;
}

/*
 * Given the customers, create a thread for each customer
 */ 
int customer_thread(customer *customers[], int cust_count, pthread_t threads[]) {
    customer *curr_customer;

    // For the amount of customers
    for (int i = 0; i < cust_count; i++) {

        curr_customer = customers[i];
        // Wait until they arrive
        usleep(curr_customer->arrival_time * 100000);
        printf("A customer arrives: customer ID %2d. \n", curr_customer->id);
        curr_customer->start_time = getTimeDifference();

        // Add the customer to their given queue
        if (curr_customer->priority == 1) {
            if (pthread_mutex_lock(&mutex2) != 0) {
                printf("Failed to lock mutex. \n");
                exit(1);
            }
            bus_qlen++;
            printf("A customer enters a queue: the queue ID %1d, and length of the queue %2d. \n", curr_customer->priority, bus_qlen);
            if (pthread_mutex_unlock(&mutex2) != 0) {
                printf("Failed to unlock mutex. \n");
                exit(1);
            }
        } else if (curr_customer->priority == 0) {

            if (pthread_mutex_lock(&mutex2) != 0) {
                printf("Failed to lock mutex. \n");
                exit(1);
            }
            econ_qlen++;
            printf("A customer enters a queue: the queue ID %1d, and length of the queue %2d. \n", curr_customer->priority, econ_qlen);
            if (pthread_mutex_unlock(&mutex2) != 0) {
                printf("Failed to unlock mutex. IN ENTRY\n");
                exit(1);
            }
        }

        // Ensuring the customer thread was created, the customer then goes to customer_entry
        if (pthread_create(&threads[i], NULL, customer_entry, curr_customer) != 0){
			printf("Failed to create pthread for customers.\n");
			exit(1);
		}

    }

    return 0;
}



int main(int argc, char *argv[]) {
    

    // Checking for the input customer file
	if (argc < 2) {
		printf("To run acs.c: ./acs customer.txt \n");
		exit(1);
	}

    
    // Initialize mutex and convar
    if (pthread_mutex_init(&mutex1, NULL) != 0) {
		printf("Failed to initialize mutex1. \n");
		exit(1);
	}
    if (pthread_mutex_init(&mutex2, NULL) != 0) {
		printf("Failed to initialize mutex2. \n");
		exit(1);
	}
	if (pthread_cond_init(&convar2, NULL) != 0) {
		printf("Failed to initialize convar2. \n");
		exit(1);
	}
    if (pthread_cond_init(&convar1, NULL) != 0) {
		printf("Failed to initialize convar1. \n");
		exit(1);
	}


    // Opening the file only once in order to use fscanf
    FILE *infile;
    infile = fopen(argv[1], "r");
    // Checking if file exists
    if (infile == NULL) {
        printf("Failed to open given file. Check if file exists. \n");
        exit(1);
    }

    // Parses the customer info into customer threads
    num_customer = get_customer_count(infile, argv[1]);
    customer *customer_arr[num_customer];
    parsing(infile, customer_arr, num_customer);
    fclose(infile);
    
    gettimeofday(&start, NULL);

    // Creating a clerk thread
    for(int i = 0; i < 4; i++) {
        // Creating a clerk LL, insert the clerk into the clerks
        clerk* clerk_init = (clerk*)malloc(sizeof(clerk));
        if(clerk_init == NULL) {
            printf("Failed to create new clerk.\n");
            exit(1);
        }
        clerk_init->id = i + 1;
        clerk_init->cust = NULL;
        clerk_init->status = 0;
        clerks[i] = clerk_init;
    }

    // Creating threads for each customer, then initializing the threads calling customer thread
    pthread_t threads[256];
    customer_thread(customer_arr, num_customer, threads);
    

    // Waiting for all threads to terminate
	for (int i = 0; i < num_customer; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
			printf("Failed to join pthread.\n");
			exit(1);
		}
	}

    // Destroy mutex and convar in case it wasn't probably terminated
    if (pthread_mutex_destroy(&mutex1) != 0) {
		printf("Failed to destroy mutex1. \n");
		exit(1);
	}
    if (pthread_mutex_destroy(&mutex2) != 0) {
		printf("Failed to destroy mutex2. \n");
		exit(1);
	}
	if (pthread_cond_destroy(&convar2) != 0) {
		printf("Failed to destroy convar2. \n");
		exit(1);
	}
    if (pthread_cond_destroy(&convar1) != 0) {
		printf("Failed to destroy convar1. \n");
		exit(1);
	}
    
    
    // Printing the waiting times for the customers
    printf("The average waiting time for all customers in the system is: %.2f seconds. \n", (float)(overall_waiting_time) / num_customer);
    printf("The average waiting time for all business-class customers is: %.2f seconds. \n",(float) business_waiting_time / business_count);
    printf("The average waiting time for all economy-class customers is: %.2f seconds. \n", (float)economy_waiting_time / economy_count);
    
    pthread_exit(NULL);

    return 0;
}



