/*
 * CSC 360 A02 Kui Wu
 * Assignment 1
 * September 16, 2020
 * Kiana Pazdernik
 */

/* 
 This assignment is to create 'pman' command in the command line using C */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>

#define MAX_INPUT 128
#define TRUE 1
#define FALSE 0

typedef struct node {
	pid_t pid;
	int running;
	char* process;
	struct node* next;
} node;

node* head = NULL;
int listlen = 0;

int running_process(pid_t pid){
	node* temp = head;
	while(temp != NULL){
		if(temp->pid == pid){
			return TRUE;
		}
		temp = temp->next;
	}
	return FALSE;

}
void readStat(char* filePath, char** fileContents) {
	FILE *fp = fopen(filePath, "r");
	char filestream[1024];
	if (fp != NULL) {
		int i = 0;
		while (fgets(filestream, sizeof(filestream)-1, fp) != NULL) {
			char* token;
			token = strtok(filestream, " ");
			fileContents[i] = token;
			while (token != NULL) {
				fileContents[i] = token;
				token = strtok(NULL, " ");
				i++;
			}
		}
		fclose(fp);
	} else {
		printf("Error: could not read stat file\n");
	}
}
void bg(char** user_process){
	
	pid_t pid = fork();
	if(pid > 0){
		node* temp = (node*)malloc(sizeof(node));
		temp->pid = pid;
		temp->process = user_process[1];
		temp->running = TRUE;
		temp->next = NULL;

		if(head == NULL){
			head = temp;
		}else{
			node* curr = head;
			while(curr->next != NULL){
				curr=curr->next;
			}
			curr->next = temp;
		}

		printf("Started Background Process: %d\n", pid);
		sleep(1);
	}else if(pid == 0){
		char* cmd = user_process[1];
		execvp(cmd, &user_process[1]);
		printf("Failed to Execute Process: %s\n", cmd);
		exit(1);
	}else{
		printf("Failed to Fork.\n");
	}
		
}

//Function to start a given pid
void bgstart(pid_t pid){
	if(!running_process(pid)){
		printf("Error: Invalid pid.\n");
		return;
	}

	int starting_pid = kill(pid, SIGCONT);
	if(!starting_pid){
		sleep(1);
	}else{
		printf("Error: Failed to Execute start.\n");
	}

}
// Function that kills the given pid
void bgkill(pid_t pid){
	if(!running_process(pid)){
		printf("Error: Invalid Pid.\n");
		return;
	}

	int killing_pid = kill(pid, SIGTERM);
	if(!killing_pid){
		sleep(1);
	}else{
		printf("Error: Failed to Execute kill.\n");
	}
	listlen--;
}

void bgstop(pid_t pid){
	if(!running_process(pid)){
		printf("Error: Invalid Pid.\n");
		return;
	}

	int stopping_pid = kill(pid, SIGSTOP);
	if(!stopping_pid){
		sleep(1);
	}else{
		printf("Error: Failed to Execute stop.\n");
	}
}

void bglist(){
	int i = 0;
	node* temp = head;
	while(temp != NULL){
		i++;
		char* stop = "";
		if(!temp->running){
			stop = "(stopped)";
		}
		printf("%d: %s %s\n", temp->pid, temp->process, stop);
		temp = temp->next;
	}

	printf("Total Background Jobs: %d \n", i);

}

void pstat(pid_t pid){

    if(running_process(pid)){
        printf("Pid does not exist: %d.\n", pid);
    }else{
        char statPath[MAX_INPUT];
		char statusPath[MAX_INPUT];
		sprintf(statPath, "/proc/%d/stat", pid);
		sprintf(statusPath, "/proc/%d/status", pid);

		char* statContents[MAX_INPUT];
		readStat(statPath, statContents);

		char statusContents[MAX_INPUT][MAX_INPUT];
		FILE* statusFile = fopen(statusPath, "r");
		if (statusFile != NULL) {
			int i = 0;
			while (fgets(statusContents[i], MAX_INPUT, statusFile) != NULL) {
				i++;
			}
			fclose(statusFile);
		} else {
			printf("Error: could not read status file\n");
			return;
		}

		char* p;
		long unsigned int utime = strtoul(statContents[13], &p, 10) / sysconf(_SC_CLK_TCK);
		long unsigned int stime = strtoul(statContents[14], &p, 10) / sysconf(_SC_CLK_TCK);
		char* voluntary_ctxt_switches = statusContents[39];
		char* nonvoluntary_ctxt_switches = statusContents[40];

		printf("comm:\t%s\n", statContents[1]);
		printf("state:\t%s\n", statContents[2]);
		printf("utime:\t%lu\n", utime);
		printf("stime:\t%lu\n", stime);
		printf("rss:\t%s\n", statContents[24]);
		printf("%s", voluntary_ctxt_switches);
		printf("%s", nonvoluntary_ctxt_switches);
    }
}
int get_prompt(char** prompt){
    char* input = readline("PMan: >");
    if(strcmp(input, "") != 0){
	char* token = strtok(input, " ");
        for(int i = 0; i < MAX_INPUT; i++){
            prompt[i] = token;
            token = strtok(NULL, " ");
        }

        return TRUE;
    }
    return FALSE;
}

void removeProcessFromList(pid_t pid) {
	if (!running_process(pid)) {
		return;
	}
	node* iterator1 = head;
	node* iterator2 = NULL;

	while (iterator1 != NULL) {
		if (iterator1->pid == pid) {
			if (iterator1 == head) {
				head = head->next;
			} else {
				iterator2->next = iterator1->next;
			}
			free(iterator1);
			return;
		}
		iterator2 = iterator1;
		iterator1 = iterator1->next;
	}
}

void process_check(){
    pid_t pid;
    int check;
    while(1){
        pid = waitpid(-1, &check, WCONTINUED | WNOHANG | WUNTRACED);
        if(!running_process(pid)){
                break;
        }
            if (pid > 0) {
                if (WIFSTOPPED(check)) {
                    printf("Background process %d was stopped.\n", pid);
                    node* pid_to_stop = NULL;
                    node* temp = head;
                    while(temp != NULL){
                        if(temp->pid == pid){
                            pid_to_stop = temp;
                        }
                        temp = temp->next;
                    }

                    pid_to_stop->running = FALSE;
                }
                if (WIFCONTINUED(check)) {
                    printf("Background process %d was started.\n", pid);
                    node* pid_to_continue = NULL;
                    node* temp = head;
                    while(temp != NULL){
                        if(temp->pid == pid){
                            pid_to_continue = temp;
                        }
                        temp = temp->next;
                    }
                    pid_to_continue->running = TRUE;
                }   

                if (WIFEXITED(check)) {
                    node* temp = head;
                    node* prev = NULL;

                    while(temp != NULL){
                        if(temp != head){
                            prev->next = temp->next;
                        }else{
                            head = head->next;
                        }
                        free(temp);
                        break;
                    }
                    prev = temp;
                    temp = temp->next;
                    listlen--;
                    printf("Background process %d terminated.\n", pid);
                    
                }
                if (WIFSIGNALED(check)) {
                    node* temp = head;
                    node* curr = NULL; 

                    while(temp != NULL){
                        if(temp != head){
                            curr->next = temp->next;
                        }else{
                            head = head->next;
                        }
                        free(temp);
                        break;
                    }
                    curr = temp;
                    temp = temp->next;
                    listlen--;
                    printf("Background process %d was killed.\n", pid);
                    
            }

        }else {
            break;
        }
    }

}
int number(char* s){
    for(int i = 0; i < strlen(s); i++){
        if(!isdigit(s[i])){
            return FALSE;
        }
    }
    return TRUE;
}


int main(){
    while(1){
        char* input[MAX_INPUT];
        int inputted = get_prompt(input);

        process_check();
	if(inputted){
	    if(strcmp("bg", input[0]) == 0){
                if(input[1] == NULL){
                    printf("Error: Invalid Command.\n");
                    continue;
		}
                bg(input);
            }else if(strcmp("bgkill", input[0]) == 0){
                if(input[1] == NULL || !number(input[1])){
                    printf("Error: Invalid Pid.\n");
                    continue;
		}

                pid_t pid = atoi(input[1]);
                bgkill(pid);
            }else if(strcmp("bgstop", input[0]) == 0){
                if(input[1] == NULL || !number(input[1])){
                    printf("Error: Invalid Pid.\n");
                    continue;
		}
            
                pid_t pid = atoi(input[1]);
                bgstop(pid);
            }else if(strcmp("bgstart", input[0]) == 0){
                if(input[1] == NULL || !number(input[1])){
                    printf("Error: Invalid Pid.\n");
                    continue;
		}
            
                pid_t pid = atoi(input[1]);
                bgstart(pid);
            }else if(strcmp("bglist", input[0]) == 0){
                bglist();
            }else if(strcmp("pstat", input[0]) == 0){
                if(input[1] == NULL || !number(input[1])){
                    printf("Error: Invalid Pid.\n");
               	    continue;;
		}
            
                pid_t pid = atoi(input[1]);
                pstat(pid);
            }else{
                printf("PMan: > %s Command not Found\n", input[0]);
            }
        
    	}
        process_check();

    }
    return 0;
}