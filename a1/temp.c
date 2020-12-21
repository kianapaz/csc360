/*
	University of Victoria
	Fall 2016
	CSC 360 Assignment 1
	Michael Reiter
	V00831568
*/

#include <unistd.h>            // fork(), execvp()
#include <string.h>            // strcmp()
#include <ctype.h>             // isdigit()
#include <stdio.h>             // printf()
#include <stdlib.h>            // malloc()
#include <sys/types.h>         // pid_t
#include <sys/wait.h>          // waitpid()
#include <signal.h>            // kill(), SIGTERM, SIGSTOP, SIGCONT
#include <readline/readline.h> // readline

/* ---------- Typedefs ---------- */

typedef struct node_t {
	pid_t pid;
	int isRunning;
	char* process;
	struct node_t* next;
} node_t;

/* ---------- Constants and global variables ---------- */

#define TRUE 1
#define FALSE 0
#define MAX_INPUT_SIZE 128
#define COMMANDS_LENGTH 6

char* VALID_COMMANDS[] = {
	"bg",
	"bgkill",
	"bgstop",
	"bgstart",
	"bglist",
	"pstat"
};

node_t* processListHead = NULL;

/* ---------- General Helper functions ---------- */

/*
	s: a string
	returns TRUE if the string is a valid integer, FALSE otherwise
*/
int isNumber(char* s) {
	int i;
	for (i = 0; i < strlen(s); i++) {
		if (!isdigit(s[i])) {
			return FALSE;
		}
	}
	return TRUE;
}

/*
	pid: a process id (e.g. 123)
	returns TRUE if the pid is valid, FALSE otherwise
*/
int isExistingProcess(pid_t pid) {
	node_t* iterator = processListHead;
	while (iterator != NULL) {
		if (iterator->pid == pid) {
			return TRUE;
		}
		iterator = iterator->next;
	}
	return FALSE;
}

/*
	command: a command (e.g. "bg", "bgkill", etc.)
	returns an integer mapping to that command or -1 if the command is invalid
*/
int commandToInt(char* command) {
	int i;
	for (i = 0; i < COMMANDS_LENGTH; i++) {
		if (strcmp(command, VALID_COMMANDS[i]) == 0) {
			return i;
		}
	}
	return -1;
}

/*
	filePath: the path the stat file (e.g. '/proc/123/stat')
	fileContents: an array of strings to which to write the read file contents
*/
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

/* ---------- Linked List functions ---------- */

/*
	pid: a process id (e.g. 123)
	adds a node with a given pid to the list of processes
*/
void addProcessToList(pid_t pid, char* process) {
	node_t* n = (node_t*)malloc(sizeof(node_t));
	n->pid = pid;
	n->process = process;
	n->isRunning = TRUE;
	n->next = NULL;

	if (processListHead == NULL) {
		processListHead = n;
	} else {
		node_t* iterator = processListHead;
		while (iterator->next != NULL) {
			iterator = iterator->next;
		}
		iterator->next = n;
	}
}

/*
	pid: a process id (e.g. 123)
	removes a node with a given pid from the list of processes
*/
void removeProcessFromList(pid_t pid) {
	if (!isExistingProcess(pid)) {
		return;
	}
	node_t* iterator1 = processListHead;
	node_t* iterator2 = NULL;

	while (iterator1 != NULL) {
		if (iterator1->pid == pid) {
			if (iterator1 == processListHead) {
				processListHead = processListHead->next;
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

/*
	pid: a process id (e.g. 123)
	returns a node with a given pid to the list of processes
*/
node_t* getNodeFromList(pid_t pid) {
	node_t* iterator = processListHead;
	while (iterator != NULL) {
		if (iterator->pid == pid) {
			return iterator;
		}
		iterator = iterator->next;
	}
	return NULL;
}

/* ---------- Commands ---------- */

/*
	userInput: a pointer to an array of tokenized user input strings
	creates a new child process and executes userInput[1] command
*/
void bg(char** userInput) {
	pid_t pid = fork();
	if (pid == 0) {    // child
		char* command = userInput[1];
		execvp(command, &userInput[1]);
		printf("Error: failed to execute command %s\n", command);
		exit(1);
	} else if (pid > 0) {		// parent
		printf("Started background process %d\n", pid);
		addProcessToList(pid, userInput[1]);
		sleep(1);
	} else {
		printf("Error: failed to fork\n");
	}
}

/*
	pid: a process id
	sends the TERM signal to a process pid to terminate it
*/
void bgkill(pid_t pid) {
	if (!isExistingProcess(pid)) {
		printf("Error: invalid pid\n");
		return;
	}
	int error = kill(pid, SIGTERM);
	if (!error) {
		sleep(1);
	} else {
		printf("Error: failed to execute bgkill\n");
	}
}

/*
	pid: a process id
	sends the STOP signal to a process pid to temporarily stop it
*/
void bgstop(pid_t pid) {
	if (!isExistingProcess(pid)) {
		printf("Error: invalid pid\n");
		return;
	}
	int error = kill(pid, SIGSTOP);
	if (!error) {
		sleep(1);
	} else {
		printf("Error: failed to execute bgstop\n");
	}
}

/*
	pid: a process id
	sends the CONT signal to a stopped process pid to restart it
*/
void bgstart(pid_t pid) {
	if (!isExistingProcess(pid)) {
		printf("Error: invalid pid\n");
		return;
	}
	int error = kill(pid, SIGCONT);
	if (!error) {
		sleep(1);
	} else {
		printf("Error: failed to execute bgstart\n");
	}
}

/*
	displays a list of all programs currently executing in the background
*/
void bglist() {
	int count = 0;
	node_t* iterator = processListHead;

	while (iterator != NULL) {
		count++;
		char* stopped = "";
		if (!iterator->isRunning) {
			stopped = "(stopped)";
		}
		printf("%d:\t %s %s\n", iterator->pid, iterator->process, stopped);
		iterator = iterator->next;
	}
	printf("Total background jobs:\t%d\n", count);
}

/*
	pid: a process id
	lists information relevant to a process pid
*/
void pstat(pid_t pid) {
	if (isExistingProcess(pid)) {
		char statPath[MAX_INPUT_SIZE];
		char statusPath[MAX_INPUT_SIZE];
		sprintf(statPath, "/proc/%d/stat", pid);
		sprintf(statusPath, "/proc/%d/status", pid);

		char* statContents[MAX_INPUT_SIZE];
		readStat(statPath, statContents);

		char statusContents[MAX_INPUT_SIZE][MAX_INPUT_SIZE];
		FILE* statusFile = fopen(statusPath, "r");
		if (statusFile != NULL) {
			int i = 0;
			while (fgets(statusContents[i], MAX_INPUT_SIZE, statusFile) != NULL) {
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
	} else {
		printf("Error: Process %d does not exist.\n", pid);
	}
}

/* ---------- Main helper functions ---------- */

/*
	input: a pointer to an array of strings to contain the tokenized user input
	tokenizes user input and stores it in input
	returns TRUE on success, FALSE on error
*/
int getUserInput(char** userInput) {
	char* rawInput = readline("PMan: > ");
	if (strcmp(rawInput, "") == 0) {
		return FALSE;
	}
	char* token = strtok(rawInput, " ");
	int i;
	for (i = 0; i < MAX_INPUT_SIZE; i++) {
		userInput[i] = token;
		token = strtok(NULL, " ");
	}
	return TRUE;
}

/*
	input: a pointer to an array of strings containing the tokenized user input
	executes commands from input
*/
void executeUserInput(char** userInput) {
	int commandInt = commandToInt(userInput[0]);

	switch (commandInt) {
		case 0: {
			if (userInput[1] == NULL) {
				printf("Error: invalid command to background\n");
				return;
			}
			bg(userInput);
			break;
		}
		case 1: {
			if (userInput[1] == NULL || !isNumber(userInput[1])) {
				printf("Error: invalid pid\n");
				return;
			}
			pid_t pid = atoi(userInput[1]);
			if (pid != 0) {
				bgkill(pid);
			}
			break;
		}
		case 2: {
			if (userInput[1] == NULL || !isNumber(userInput[1])) {
				printf("Error: invalid pid\n");
				return;
			}
			pid_t pid = atoi(userInput[1]);
			bgstop(pid);
			break;
		}
		case 3: {
			if (userInput[1] == NULL || !isNumber(userInput[1])) {
				printf("Error: invalid pid\n");
				return;
			}
			pid_t pid = atoi(userInput[1]);
			bgstart(pid);
			break;
		}
		case 4:
			bglist();
			break;
		case 5: {
			if (userInput[1] == NULL || !isNumber(userInput[1])) {
				printf("Error: invalid pid\n");
				return;
			}
			pid_t pid = atoi(userInput[1]);
			pstat(pid);
			break;
		}
		default:
			printf("PMan: > %s:\tcommand not found\n", userInput[0]);
			break;
	}
}

/*
	updates process list running statuses
*/
void updateProcessStatuses() {
	pid_t pid;
	int	status;
	while (TRUE) {
		pid = waitpid(-1, &status, WCONTINUED | WNOHANG | WUNTRACED);
		if (pid > 0) {
			if (WIFSTOPPED(status)) {
				printf("Background process %d was stopped.\n", pid);
				node_t* n = getNodeFromList(pid);
				n->isRunning = FALSE;
			}
			if (WIFCONTINUED(status)) {
				printf("Background process %d was started.\n", pid);
				node_t* n = getNodeFromList(pid);
				n->isRunning = TRUE;
			}
			if (WIFSIGNALED(status)) {
				printf("Background process %d was killed.\n", pid);
				removeProcessFromList(pid);
			}
			if (WIFEXITED(status)) {
				printf("Background process %d terminated.\n", pid);
				removeProcessFromList(pid);
			}
		} else {
			break;
		}
	}
}

/* ---------- Main ---------- */

/*
	An interactive prompt that can run processes in the background on Unix
*/
int main() {
	while (TRUE) {
		char* userInput[MAX_INPUT_SIZE];
		int success = getUserInput(userInput);
		updateProcessStatuses();
		if (success) {
			executeUserInput(userInput);
		}
		updateProcessStatuses();
	}

	return 0;
}