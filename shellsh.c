#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> // pid_t
#include <sys/wait.h> // for waitpid
#include <unistd.h> // fork
#include <ctype.h>

//int status = 0;

// handler function for SIGINT: ctrl+c
void handle_SIGINT(int signo) {
    char* message = "terminated by signal 2\n";
    write(STDOUT_FILENO, message, 24);
}

// handler for SIGUSR2
void handle_SIGUSR2(int signo) {
    exit(0);
}




int main(){

    struct sigaction SIGINT_action = {0}, SIGUSR2_action = {0};

    // fill in SIGINT_actino struct for blocking SIGINT
    SIGINT_action.sa_handler = handle_SIGINT;
    sigfillset(&SIGINT_action.sa_mask);     // block all catchable signals while handler is running
    SIGINT_action.sa_flags = 0;             // no flags set
    sigaction(SIGINT, &SIGINT_action, NULL);

    // fill in SIGUSR2_action struct for exiting
    SIGUSR2_action.sa_handler = handle_SIGUSR2;
    sigfillset(&SIGUSR2_action.sa_mask);
    SIGUSR2_action.sa_flags = 0;
    sigaction(SIGUSR2, &SIGUSR2_action, NULL);

    int status = 0;   
    while (1) {
        char command_array[2048];
        char *command = &command_array[0];
        int len = 2048;
        printf(":");
        fgets(command, 2048, stdin);

        // 'fgets' keeps the newline at the end; strip '\n' from the end of the command
        command[strcspn(command, "\n")] = 0;

        while(isspace(*command)) {
            command++;
        }

        if (strncmp(command, "#", 1) == 0) {
            continue;
        }

        if (command[0] == '\0') {
            continue;
        }

        if (strcmp(command,"exit") == 0) {
            raise(SIGUSR2);
        }
        
        if (strcmp(command,"status") == 0) {       
            printf("exit value %d\n",status);
            continue;
        }

        // cd to another dir
        if (strncmp(command,"cd ", 3) == 0) {
            // return -1 if error
            status = chdir(command+3);    // ptr arith: command->"cd .." thus command+3->".."
            // &command[3]   
            // printf("ok\n");                 
            // system("pwd");
            // #define PATH_MAX 200
            // char cwd[200];
            // if (getcwd(cwd, sizeof(cwd)) != NULL) {
            //     printf("Current working dir: %s\n", cwd);
            // } 
            continue;
        }

        
        char *args[512];
        char* token;
        char* firstCommand;
        token = strtok(command, " ");
        args[0] = calloc(strlen(token)+1, sizeof(char));
        strcpy(args[0], token);
        firstCommand = calloc(strlen(token)+1, sizeof(char));
        strcpy(firstCommand, token);
        int i = 1;
        while (token != NULL) {
            token = strtok(NULL, " ");
            if (token != NULL) {
                args[i] = calloc(strlen(token)+1, sizeof(char));
                strcpy(args[i], token);
                i++;
            }            
        }
        args[i] = NULL;

        //checkForRedirect(args);
        int writeTo, writeFrom = 0;
        int writeToIndex, writeFromIndex = 0;
        for (int j = 0; j < i; j++) {
            if (strcmp(args[j],">") == 0) {
                writeTo = 1;
                writeToIndex = j;
                //printf("ok\n");
            }
            if (strcmp(args[j],"<") == 0) {
                writeFrom = 1;
                writeFromIndex = j;
                //printf("other ok\n");
            }
        }
        int out, savestdout = 0;
        if (writeTo == 1) {
            out = open(args[writeToIndex+1], O_WRONLY | O_CREAT | O_TRUNC, 0640);
            int savestdout = dup(1);
            printf("%s\t%d\n", args[writeToIndex+1],out);
            // TODO: what if command is miswritten?
            if (out == -1) {
                perror("error  in open()\n");
                continue;
            }
            // int result = dup2(out, 1);
            // if (result = -1) {
            //     perror("error in dup2()\n");
            //     continue;
            // }
        }




        // spawn a child to run exec function
        int child;
        pid_t spawnpid = fork();
        switch (spawnpid) {
        case -1:
            perror("forked up\n");
            exit(1);
            break;
        case 0: ;     
            // child process          
            // list of arguments from command/input prompt
            // char *args[512];
            // char* token;
            // char* firstCommand;
            // token = strtok(command, " ");
            // args[0] = calloc(strlen(token)+1, sizeof(char));
            // strcpy(args[0], token);
            // firstCommand = calloc(strlen(token)+1, sizeof(char));
            // strcpy(firstCommand, token);
            // int i = 1;
            // while (token != NULL) {
            //     token = strtok(NULL, " ");
            //     if (token != NULL) {
            //         args[i] = calloc(strlen(token)+1, sizeof(char));
            //         strcpy(args[i], token);
            //         i++;
            //     }            
            // }
            // args[i] = NULL;

            // //checkForRedirect(args);
            // int writeTo, writeFrom = 0;
            // int writeToIndex, writeFromIndex = 0;
            // for (int j = 0; j < i; j++) {
            //     if (strcmp(args[j],">") == 0) {
            //         writeTo = 1;
            //         writeToIndex = j;
            //         //printf("ok\n");
            //     }
            //     if (strcmp(args[j],"<") == 0) {
            //         writeFrom = 1;
            //         writeFromIndex = j;
            //         //printf("other ok\n");
            //     }
            // }

            if (writeTo == 1) {
                //int out = open(args[writeToIndex+1], O_WRONLY | O_CREAT | O_TRUNC, 0640);
                //printf("%s\t%d\n", args[writeToIndex+1],out);
                // TODO: what if command is miswritten?
                // if (out == -1) {
                //     perror("error  in open()\n");
                //     continue;
                // }
                int result = dup2(out, 1);
                if (result == -1) {
                    perror("error in dup2()\n");
                    exit(-1);
                }
                args[writeToIndex] = 0;
            }

            execvp(args[0], args);
            status = 2;
            exit(status);

        default:
            // in the parent process
            // get status of last child and save as status
            spawnpid = waitpid(spawnpid, &child, 0);
            // chekc if child returned normally
            status = -1;            // just in case there is an issue
            if (WIFEXITED(child)) {
                status = WEXITSTATUS(child);
                printf("in parent: exit value %d\n",status);
            }
            // if (out > 0) {
            //     close(out);
            //     dup2(savestdout, 1);
            //     close(savestdout);
            // }
            //status = WEXITSTATUS(status);       
            //printf("exit value %d\n",exit_status);
        }
    }
    return 0;
}

