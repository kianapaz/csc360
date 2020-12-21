/*
 * CSC 360 A02
 * Assignment 1
 * October 2, 2020
 * Kiana Pazdernik

 This assignment is to create 'pman' command in the command line using C
 Commands Include:
 bg
 bgstart
 bgstop
 bgkill
 bglist
 pstat
 */

#include <ctype.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_INPUT 128

// Struct for a linked list with pid, given process and if the process is
// currently running
typedef struct node {
    pid_t pid;
    char *process;
    int running;
    struct node *next;
} node;

node *head = NULL;
int listlen = 0;

// Function to check if a given pid exists
int running_process(pid_t pid) {
    node *temp = head;

    while (temp != NULL) {
        if (temp->pid == pid) {
            return 1;
        }
        temp = temp->next;
    }
    return 0;
}

// bg function that starts a given process
void bg(char **user_process) {
    pid_t pid = fork();
    if (pid > 0) {
        node *temp = (node *)malloc(sizeof(node));
        temp->pid = pid;
        temp->process = user_process[1];
        temp->running = 1;
        temp->next = NULL;

        if (head == NULL) {
            head = temp;
        } else {
            node *curr = head;
            while (curr->next != NULL) {
                curr = curr->next;
            }
            curr->next = temp;
        }

        printf("Started Background Process: %d\n", pid);
    } else if (pid == 0) {
        char *cmd = user_process[1];
        execvp(cmd, &user_process[1]);
        printf("Failed to Execute Process: %s\n", cmd);
        exit(1);
    } else {
        printf("Failed to Fork.\n");
    }
}

// Function to start a given pid
void bgstart(pid_t pid) {
    if (!running_process(pid)) {
        printf("Error: Invalid pid.\n");
        return;
    }

    int starting_pid = kill(pid, SIGCONT);
    if (starting_pid) {
        printf("Error: Failed to Execute start.\n");
    }
}

// Function that kills the given pid
void bgkill(pid_t pid) {
    if (!running_process(pid)) {
        printf("Error: Invalid Pid.\n");
        return;
    }

    int killing_pid = kill(pid, SIGTERM);
    if (killing_pid) {
        printf("Error: Failed to Execute kill.\n");
    }
}

// Function that kills given pid
void bgstop(pid_t pid) {
    if (!running_process(pid)) {
        printf("Error: Invalid Pid.\n");
        return;
    }

    int stopping_pid = kill(pid, SIGSTOP);
    if (stopping_pid) {
        printf("Error: Failed to Execute stop.\n");
    }
}

// Function that iterates through the LL and prints the total processes
void bglist() {
    int i = 0;
    node *temp = head;
    while (temp != NULL) {
        i++;
        char *stop = "";
        if (!temp->running) {
            stop = "(stopped)";
        }
        printf("%d: %s %s\n", temp->pid, temp->process, stop);
        temp = temp->next;
    }

    printf("Total Background Jobs: %d \n", i);
}

// Function that takes a pid and gets the resulting info of the pid and location
void pstat(pid_t pid) {
    if (!running_process(pid)) {
        printf("Pid does not exist: %d.\n", pid);
        return;
    }

    char stat_path[MAX_INPUT];
    char status_path[MAX_INPUT];
    char *stat_contents[MAX_INPUT];

    // Finds the stat and status location of the pid
    sprintf(stat_path, "/proc/%d/stat", pid);
    sprintf(status_path, "/proc/%d/status", pid);

    // Opens the stat file
    FILE *stat_file = fopen(stat_path, "r");
    char filestream[1024];
    if (stat_file == NULL) {
        printf("Could not read stat file.\n");
        return;
    }
    int i = 0;
    while (fgets(filestream, sizeof(filestream) - 1, stat_file) != NULL) {
        // Adding the contents to th stat_content file
        char *token;
        token = strtok(filestream, " ");
        stat_contents[i] = token;
        while (token != NULL) {
            stat_contents[i] = token;
            token = strtok(NULL, " ");
            i++;
        }
    }
    fclose(stat_file);

    // After getting the stat info, get the utime, stime, comm, state, rss
    char *utime_c;
    char *stime_c;
    long unsigned int utime =
        strtoul(stat_contents[13], &utime_c, 10) / sysconf(_SC_CLK_TCK);
    long unsigned int stime =
        strtoul(stat_contents[14], &stime_c, 10) / sysconf(_SC_CLK_TCK);

    printf("comm: %s \n", stat_contents[1]);
    printf("state: %s \n", stat_contents[2]);
    printf("utime: %lu \n", utime);
    printf("stime: %lu \n", stime);
    printf("rss: %s \n", stat_contents[24]);

    // Opening the status file
    char status_contents[MAX_INPUT][MAX_INPUT];
    FILE *status_file = fopen(status_path, "r");
    if (status_file == NULL) {
        printf("Could not read status file.\n");
        return;
    }
    int j = 0;
    // Adding the contents to status_contents
    while (fgets(status_contents[j], MAX_INPUT, status_file) != NULL) {
        j++;
    }
    fclose(status_file);

    // Getting the vol/nonvol switches
    char *voluntary_ctxt_switches = status_contents[39];
    char *nonvoluntary_ctxt_switches = status_contents[40];
    printf("%s ", voluntary_ctxt_switches);
    printf("%s ", nonvoluntary_ctxt_switches);
}

// Function to check if any flags has been raised for
// continuing/stopping/terminating processes
void process_check() {
    pid_t pid;
    int check;
    while (1) {
        pid = waitpid(-1, &check, WCONTINUED | WNOHANG | WUNTRACED);
        if (!running_process(pid)) {
            break;
        }
        if (pid <= 0) {
            break;
        }
        if (WIFSTOPPED(check)) {
            printf("Background process %d was stopped.\n", pid);
            node *pid_to_stop = NULL;
            node *temp = head;
            while (temp != NULL) {
                if (temp->pid == pid) {
                    pid_to_stop = temp;
                }
                temp = temp->next;
            }

            pid_to_stop->running = 0;
        }
        if (WIFCONTINUED(check)) {
            printf("Background process %d was started.\n", pid);
            // listlen++;
            node *pid_to_continue = NULL;
            node *temp = head;
            while (temp != NULL) {
                if (temp->pid == pid) {
                    pid_to_continue = temp;
                }
                temp = temp->next;
            }

            pid_to_continue->running = 1;
        }

        if (WIFEXITED(check)) {
            printf("Background process %d terminated.\n", pid);
            node *temp = head;
            node *prev = NULL;

            while (temp != NULL) {
                if (temp != head) {
                    prev->next = temp->next;
                } else {
                    head = head->next;
                }
                free(temp);
                break;
            }
            prev = temp;
            temp = temp->next;
            listlen--;
        }
        if (WIFSIGNALED(check)) {
            printf("Background process %d was killed.\n", pid);
            node *temp = head;
            node *curr = NULL;

            while (temp != NULL) {
                if (temp != head) {
                    curr->next = temp->next;
                } else {
                    head = head->next;
                }
                free(temp);
                break;
            }
            curr = temp;
            temp = temp->next;
            listlen--;
        }
    }
}

// Helper function to check if the input is a pid (a digit)
int pid_number(char *pid) {
    for (int i = 0; i < strlen(pid); i++) {
        if (!isdigit(pid[i])) {
            return 0;
        }
    }
    return 1;
}

int main() {
    while (1) {
        char *input[MAX_INPUT];
        int inputted = 0;

        // Getting User Prompt from Terminal
        char *user_prompt = readline("PMan: > ");

        // Ensuring command is not NULL
        if (strcmp(user_prompt, "") != 0) {
            char *token = strtok(user_prompt, " ");
            for (int i = 0; i < MAX_INPUT; i++) {
                input[i] = token;
                token = strtok(NULL, " ");
            }
            inputted = 1;
        }

        // Checking the processes if there are any flags
        process_check();

        // If an input has been given, find which command has been given
        if (inputted) {
            if (strcmp("bg", input[0]) == 0) {
                if (input[1] == NULL) {
                    printf("Error: Invalid Command.\n");
                    continue;
                }
                bg(input);
            } else if (strcmp("bgkill", input[0]) == 0) {
                if (input[1] == NULL || !pid_number(input[1])) {
                    printf("Error: Invalid Pid.\n");
                    continue;
                }

                pid_t pid = atoi(input[1]);
                bgkill(pid);
            } else if (strcmp("bgstop", input[0]) == 0) {
                if (input[1] == NULL || !pid_number(input[1])) {
                    printf("Error: Invalid Pid.\n");
                    continue;
                }

                pid_t pid = atoi(input[1]);
                bgstop(pid);
            } else if (strcmp("bgstart", input[0]) == 0) {
                if (input[1] == NULL || !pid_number(input[1])) {
                    printf("Error: Invalid Pid.\n");
                    continue;
                }

                pid_t pid = atoi(input[1]);
                bgstart(pid);
            } else if (strcmp("bglist", input[0]) == 0) {
                bglist();
            } else if (strcmp("pstat", input[0]) == 0) {
                if (input[1] == NULL || !pid_number(input[1])) {
                    printf("Error: Invalid Pid.\n");
                    continue;
                }

                pid_t pid = atoi(input[1]);
                pstat(pid);
            } else {
                printf("PMan: > %s Command not Found\n", input[0]);
            }
        }
        // Checks the process flags again
        process_check();
    }
    return 0;
}
