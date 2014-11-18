//commented for human reading by Danny (Andrew McDaniel)

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>

extern int errno;

typedef void (*sighandler_t)(int);
static char *my_argv[100], *my_envp[100];
static char *search_path[10];

void handle_signal(int signo)
{
    printf("\n[MY_SHELL ] ");
    fflush(stdout);
}
//takes in a string with spaces and breaks the string into array entries at the spaces, filling an argument array
void fill_argv(char *tmp_argv)
{
    char *foo = tmp_argv; //puts the tmp that was passed in into a new character array
    int index = 0;
    char ret[100]; //makes another character array and sets all the bytes to zero
    bzero(ret, 100);
    while(*foo != '\0') {
        if(index == 10) //if the index hits 10, break out (this sets a hard limit of ten "words" in any command)
            break;

        if(*foo == ' ') { //checks for a space in foo
            if(my_argv[index] == NULL) //checks if my_argv at the current index is open
                my_argv[index] = (char *)malloc(sizeof(char) * strlen(ret) + 1); //if so allocate space for the contents of ret, which are now presumably a full command, option, file, etc
            else {
                bzero(my_argv[index], strlen(my_argv[index])); //otherwise empty out my_argv at that location
            }
            strncpy(my_argv[index], ret, strlen(ret)); //copy ret into my_argv at the current index
            strncat(my_argv[index], "\0", 1); //add a null pointer to the end
            bzero(ret, 100); //empty out ret
            index++; //advance the index
        } else {
            strncat(ret, foo, 1); //if not a space keep adding characters to ret
        }
        foo++; //advance foo so we're checking whatever comes next
        /*printf("foo is %c\n", *foo);*/ //NOT MY COMMENT
    }
    my_argv[index] = (char *)malloc(sizeof(char) * strlen(ret) + 1); // This section covers for when foo reaches a null character,
    strncpy(my_argv[index], ret, strlen(ret));                       // since there would be no space to trigger the rest of the code.
    strncat(my_argv[index], "\0", 1);                                // 
}
//copies the environment variables to a string array
void copy_envp(char **envp)
{
    int index = 0;
    for(;envp[index] != NULL; index++) { //runs until we're out of environment data to copy
        my_envp[index] = (char *)
		malloc(sizeof(char) * (strlen(envp[index]) + 1)); //allocates space to copy in the supplied environment data
        memcpy(my_envp[index], envp[index], strlen(envp[index])); //copies over the data
    }
}

void get_path_string(char **tmp_envp, char *bin_path)
{
    int count = 0;
    char *tmp;
    while(1) {
        tmp = strstr(tmp_envp[count], "PATH");
        if(tmp == NULL) {
            count++;
        } else {
            break;
        }
    }
        strncpy(bin_path, tmp, strlen(tmp));
}

void insert_path_str_to_search(char *path_str) 
{
    int index=0;
    char *tmp = path_str;
    char ret[100];

    while(*tmp != '=')
        tmp++;
    tmp++;

    while(*tmp != '\0') {
        if(*tmp == ':') {
            strncat(ret, "/", 1);
            search_path[index] = 
		(char *) malloc(sizeof(char) * (strlen(ret) + 1));
            strncat(search_path[index], ret, strlen(ret));
            strncat(search_path[index], "\0", 1);
            index++;
            bzero(ret, 100);
        } else {
            strncat(ret, tmp, 1);
        }
        tmp++;
    }
}
//attaches a file path to files
int attach_path(char *cmd)
{
    char ret[100]; //builds a characters array
    int index;
    int fd;
    bzero(ret, 100); //fills the corresponding bytes with zeroes
    for(index=0;search_path[index]!=NULL;index++) { //goes through until search_path is null
        strcpy(ret, search_path[index]); //copies the contents of search_path at the current index into ret (and presumably wipes the previous contents of ret
        strncat(ret, cmd, strlen(cmd)); //attaches the command onto the end of ret after the contents of search_path
        if((fd = open(ret, O_RDONLY)) > 0) { //attempts to open the command as read only (positive integers mean it opened, everything else means an error)
            strncpy(cmd, ret, strlen(ret)); //if the command was opened successfully, change the command to the contents of ret, which appear to be the file path plus the file name
            close(fd); //closes the file
            return 0;
        }
    }
    return 0;
}
//calls the command
void call_execve(char *cmd)
{
    int i;
    printf("cmd is %s\n", cmd);
    if(fork() == 0) { //creates a fork to run the command, so the shell can wait in the background
        i = execve(cmd, my_argv, my_envp); //runs the command, passing in the arguments and environment variables (Note that on success this breaks out of the program completely, hence why the following error doesn't print.)
        printf("errno is %d\n", errno); //prints an error if the command failed
        if(i < 0) {
            printf("%s: %s\n", cmd, "command not found");
            exit(1); //kills the child process after appropriate errors have printed
        }
    } else {
        wait(NULL); //waits for the child process to finish (by either crashing or closing normally or failing to run) before resuming running the shell
    }
}
//empties out the my_argv array
void free_argv()
{
    int index;
    for(index=0;my_argv[index]!=NULL;index++) { //runs through my_argv
        bzero(my_argv[index], strlen(my_argv[index])+1); //empties out each entry
        my_argv[index] = NULL; //sets it to NULL
        free(my_argv[index]); //releases the allocated memory
    }
}

int main(int argc, char *argv[], char *envp[])
{
    char c;
    int i, fd;
    char *tmp = (char *)malloc(sizeof(char) * 100);
    char *path_str = (char *)malloc(sizeof(char) * 256);
    char *cmd = (char *)malloc(sizeof(char) * 100);
    
    signal(SIGINT, SIG_IGN);
    signal(SIGINT, handle_signal);

    copy_envp(envp);

    get_path_string(my_envp, path_str);   
    insert_path_str_to_search(path_str);

    if(fork() == 0) {
        execve("/usr/bin/clear", argv, my_envp);
        exit(1);
    } else {
        wait(NULL);
    }
    printf("[MY_SHELL ] ");
    fflush(stdout);

    while(c != EOF) {
        c = getchar();
        switch(c) { //initiates a switch statement that checks the input characters and runs code when enter is pressed
            case '\n': if(tmp[0] == '\0') { //if the input is empty, just print out the prompt again
                       printf("[MY_SHELL ] ");
                   } else { //otherwise do all this stuff
                       fill_argv(tmp); //loads the contents of tmp into my_argv
                       strncpy(cmd, my_argv[0], strlen(my_argv[0])); //copy in the command
                       strncat(cmd, "\0", 1); //add a null character to the end
                       if(index(cmd, '/') == NULL) { //if the command does NOT contain a / (and thus doesnt have an associated path)
                           if(attach_path(cmd) == 0) { //attaches the appropriate path to the command name
                               call_execve(cmd); //run call_execve
                           } else {
                               printf("%s: command not found\n", cmd); //print an error if the command could not be run
                           }
                       } else { //if the command DOES contain a / (and thus presumably has a path attached)
                           if((fd = open(cmd, O_RDONLY)) > 0) { //checks if the command can be opened and then closes it
                               close(fd);
                               call_execve(cmd); //run call_execve
                           } else {
                               printf("%s: command not found\n", cmd);
                           }
                       }
                       free_argv(); //empties out my_argv
                       printf("[MY_SHELL ] "); //print the prompt
                       bzero(cmd, 100); //after everything is said and done make cmd empty again
                   }
                   bzero(tmp, 100); //also make tmp empty
                   break;
            default: strncat(tmp, &c, 1); //if the input was not the enter key, add what was typed to tmp
                 break;
        }
    }
    free(tmp);
    free(path_str);
    for(i=0;my_envp[i]!=NULL;i++)
        free(my_envp[i]);
    for(i=0;i<10;i++)
        free(search_path[i]);
    printf("\n");
    return 0;
}
