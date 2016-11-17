#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <stdio.h>
#include <dirent.h>
#include <libgen.h>

int displayCurrentWorkingDirectory(void);
void runCommand(char* command, char* PATH);
void childProcess(char* programName, char* argv[], char* PATH);
void parentProcess(pid_t pid);

int main(void) {
    char Error[256] = "";
    /* Read profile. */
    FILE* file = fopen("./profile", "r");
    
    char HOME[1024] = "";
    char PATH[1024] = "";
    char buffer[256] = "";
    char start[6] = "";
    char pathStart[6] = "PATH=";
    char homeStart[6] = "HOME=";
    if (file == NULL) {
        strcpy(Error, "File profile does not exist.");
    } else {
        /* Assign PATH and HOME from profile. */
        while (fgets(buffer, 256, file)) {
            strncpy(start, buffer, 5);
            if (strcmp(start, pathStart) == 0) {
                strncpy(PATH, buffer+5, (strlen(buffer) - 5));
                PATH[strlen(PATH)-1] = 0;
            } else if (strcmp(start, homeStart) == 0) {
                strncpy(HOME, buffer+5, (strlen(buffer) - 5));
                HOME[strlen(HOME)-1] = 0;
            }
        }
        fclose(file);
    }

    /* if PATH or HOME not assigned. */
    if (PATH[0] == '\0' || HOME[0] == '\0') {
        strcpy(Error, "PATH or HOME is not assigned.");
    }
    if (Error[0] != '\0') {
        printf("There is an error: %s\n", Error);
    } else {
        displayCurrentWorkingDirectory();
        char command[256] = "";
        /* Command, and enter. */
        while (1) {
            fgets(command, 256, stdin);
            command[strlen(command)-1] = 0;
            runCommand(command, PATH);
            /* When program completes, prompt presents again. */
            displayCurrentWorkingDirectory();
        }
    }
    return 0;
}

/* This function get current working directory full path and print in command prompt. */
int displayCurrentWorkingDirectory(void) {
    /* Get current working directory. */
    char cwd[256] = "";
    getcwd(cwd, sizeof(cwd)); /* getcwd returns absolute pathname of current working directory pointer. */
    /* Command prompt. */
    printf(">%s\n", cwd);
    return 0;
}

/* This function run the command user typed. */
void runCommand(char* command, char* PATH) {
    /* Get the program name and arguments from command. */
    char programName[256] = "";
    int indexOfCommand = 0;
    int numOfEmptySpace = 0;
    char* argv[20];
    char argument[256] = "";
    int indexOfArgumentArray;
    int indexOfEachArgument = 0;
    while (command[indexOfCommand] != '\0') {
        /* Still getting the program name. */
        if (numOfEmptySpace == 0) {
            if (command[indexOfCommand] != ' ') {
                programName[indexOfCommand] = command[indexOfCommand];
            } else {
                numOfEmptySpace ++;
                argv[0] = programName;
            }
        } else {
            /* Getting the arguments from command. */
            if (command[indexOfCommand] != ' ') {
                argument[indexOfEachArgument] = command[indexOfCommand];
                indexOfArgumentArray ++;
                indexOfEachArgument ++;
            } else {
                argv[numOfEmptySpace] = argument;
                strcpy(argument, "");
                numOfEmptySpace ++;
                indexOfEachArgument = 0;
            }
        }
        indexOfCommand ++;
    }
    
    /* Run the program by creating child process. */
//    pid_t pid = fork();
//    if (pid == 0) {
        childProcess(programName, argv, PATH);
//    } else {
//        wait(&pid);
        //parentProcess(pid);
//    }
}

/* Child process needs to execute the program. */
void childProcess(char* programName, char* argv[], char* PATH) {
    DIR* dir;
    struct dirent* dp;
    
    int foundPath = 0;
    /* Search through PATH. */
    char correctPath[256] = "";
    int indexOfPaths = 0;
    char programPath[256] = "";
    int index = 0;
    int numberOfC = 0;
    while (PATH[indexOfPaths] != '\0') {
        if (PATH[indexOfPaths] != ':') {
            correctPath[index] = PATH[indexOfPaths];
            index ++;
        } else {
            numberOfC ++;
            /* Check whether program is under this directory. */
            
            /* Try to open the directory. */
            if ((dir = opendir(correctPath)) == NULL) {
//                exit(1);
                printf("debug: directory of PATH not found");
            } else {
                /* Search for program. */
                while ((dp = readdir(dir)) != NULL) {
                    if (strcmp(dp->d_name, programName) == 0) {
                        realpath(dp->d_name, programPath);
                        closedir(dir);
                        foundPath = 1;
                        break;
                    }
                }
            }
            if (foundPath != 0) {
                break;
            }


            strcpy(correctPath, "");
            index = 0;
        }
        indexOfPaths ++;
    }
    /* This takes care of the case when there is no : in PATH. */
    if (numberOfC == 0) {
        if ((dir = opendir(correctPath)) == NULL) {
            //                exit(1);
            printf("debug: directory of PATH not found");
        } else {
            /* Search for program. */
            while ((dp = readdir(dir)) != NULL) {
                if (strcmp(dp->d_name, programName) == 0) {
                    printf("debug: found program.");
                    realpath(dp->d_name, programPath);
                    closedir(dir);
                    break;
                }
            }
            printf("debug: did not find program");
        }
    }
    system("/bin/ls");

//    system(programPath);
//    execv("ls", argv);
//    exit(0);
}

/* Parent process should wait until child process is completed. */ 
void parentProcess(pid_t pid) {
    wait(NULL);
}
