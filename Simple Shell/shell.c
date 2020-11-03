#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "unistd.h" //Chdir, getcwd, fork, close, dup2
#include "errno.h"
#include "sys/types.h" //Fork, wait, open
#include "sys/wait.h"  //Wait
#include "sys/stat.h"  //Open
#include "fcntl.h"     //Open

//Prototypes
void shell();
int getCommandLine(char **linePtr);
int runCommand(char *linePtr);

int main(int argc, char *argv[])
{
    //Only accepts ./shell
    if (argc == 1)
    {
        shell();
    }
    else
    {
        printf("Too many arguments, only exe.\n");
    }
    return 0;
}

void shell()
{
    char *linePtr = NULL;
    char *parse = NULL;
    int PID;
    int status;

    while (getCommandLine(&linePtr) != -1)
    {
        //Comparing linePtr to inputs
        if (strncmp(linePtr, "\n", 1) == 0)
        {
            continue;
        }
        if (strncmp(linePtr, "exit", 4) == 0)
        {
            //Exiting
            exit(0); //Terminated by user, successful exit
        }
        else if (strncmp(linePtr, "cd", 2) == 0)
        {
            parse = strtok(linePtr, " \n");
            parse = strtok(NULL, " \n");
            //Takes char *
            //Returns 0 on succes, -1 on error
            if (chdir(parse) == -1)
            {
                fprintf(stderr, "CD Error: %s\n", strerror(errno));
            }
        }
        else
        {
            PID = fork();
            if (PID == 0) //Child
            {
                if (runCommand(linePtr) == -1) //Execution error
                {
                    break;
                }
            }
            else if (PID == -1) //Error
            {
                fprintf(stderr, "Fork Error: %s\n", strerror(errno));
                break;
            }

            if (wait(&status) == -1) //Parent waits
            {
                fprintf(stderr, "Wait Error: %s\n", strerror(errno));
                break;
            }
        }
    }
    free(linePtr);
    exit(1); //Terminated by error, not successful exit
}

int runCommand(char *linePtr)
{
    //I/O Redirection
    int fd = 0;
    char *file = NULL;

    //Agrv array
    int argSig = 0;
    int argvSize = 0;
    int maxSize = 25;
    char **argv = malloc(maxSize * sizeof(char *));
    if (argv == NULL)
    {
        fprintf(stderr, "Malloc Error: %s\n", strerror(errno));
        return -1;
    }
    argv[0] = NULL; //Initalizing first entry to NULL Ptr

    //Tokens
    char *token = NULL;
    token = strtok(linePtr, " \n");
    while (token != NULL)
    {
        if (strcmp(token, ">") == 0) //Output
        {
            argSig = 1; //No more arguments for executable
            //Read file name
            file = strtok(NULL, " \n");
            if (file == NULL || strcmp(file, ">") == 0 || strcmp(file, "<") == 0)
            {
                fprintf(stderr, "Missing output file.\n");
                return -1;
            }
            //Opening file, redirecting output to this
            fd = open(file, O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR); //Mode = R/W for users
            if (fd == -1)
            {
                fprintf(stderr, "File Open Error: %s\n", strerror(errno));
                return -1;
            }
            //I/O Redirection
            if (dup2(fd, STDOUT_FILENO) == -1)
            {
                fprintf(stderr, "Dup2 Error: %s\n", strerror(errno));
                return -1;
            }
            //Closing old FD
            if (close(fd) == -1)
            {
                fprintf(stderr, "Close Error: %s\n", strerror(errno));
                return -1;
            }
        }
        else if (strcmp(token, ">>") == 0) //Output append
        {
            argSig = 1;
            //Read file name
            file = strtok(NULL, " \n");
            if (file == NULL || strcmp(file, ">") == 0 || strcmp(file, "<") == 0)
            {
                fprintf(stderr, "Missing output file.\n");
                return -1;
            }
            //Opening file, redirecting output to this
            fd = open(file, O_CREAT | O_RDWR | O_APPEND, S_IRUSR | S_IWUSR); //Mode = R/W for users
            if (fd == -1)
            {
                fprintf(stderr, "File Open Error: %s\n", strerror(errno));
                return -1;
            }
            //I/O Redirection
            if (dup2(fd, STDOUT_FILENO) == -1)
            {
                fprintf(stderr, "Dup2 Error: %s\n", strerror(errno));
                return -1;
            }
            //Closing old FD
            if (close(fd) == -1)
            {
                fprintf(stderr, "Close Error: %s\n", strerror(errno));
                return -1;
            }
        }
        else if (strcmp(token, "2>") == 0) //Error
        {
            argSig = 1;
            //Read file name
            file = strtok(NULL, " \n");
            if (file == NULL || strcmp(file, ">") == 0 || strcmp(file, "<") == 0)
            {
                fprintf(stderr, "Missing error output file.\n");
                return -1;
            }
            //Opening file, redirecting output to this
            fd = open(file, O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
            if (fd == -1)
            {
                fprintf(stderr, "File Open Error: %s\n", strerror(errno));
                return -1;
            }
            //I/O Redirection
            if (dup2(fd, STDERR_FILENO) == -1)
            {
                fprintf(stderr, "Dup2 Error: %s\n", strerror(errno));
                return -1;
            }
            //Closing old FD
            if (close(fd) == -1)
            {
                fprintf(stderr, "Close Error: %s\n", strerror(errno));
                return -1;
            }
        }
        else if (strcmp(token, "2>>") == 0) //Error append
        {
            argSig = 1;
            //Read file name
            file = strtok(NULL, " \n");
            if (file == NULL || strcmp(file, ">") == 0 || strcmp(file, "<") == 0)
            {
                fprintf(stderr, "Missing error output file.\n");
                return -1;
            }
            //Opening file, redirecting output to this
            fd = open(file, O_CREAT | O_RDWR | O_APPEND, S_IRUSR | S_IWUSR);
            if (fd == -1)
            {
                fprintf(stderr, "File Open Error: %s\n", strerror(errno));
                return -1;
            }
            //I/O Redirection
            if (dup2(fd, STDERR_FILENO) == -1)
            {
                fprintf(stderr, "Dup2 Error: %s\n", strerror(errno));
                return -1;
            }
            //Closing old FD
            if (close(fd) == -1)
            {
                fprintf(stderr, "Close Error: %s\n", strerror(errno));
                return -1;
            }
        }
        else if (strcmp(token, "<") == 0) //Input
        {
            argSig = 1;
            file = strtok(NULL, " \n");
            if (file == NULL || strcmp(file, ">") == 0 || strcmp(file, "<") == 0)
            {
                fprintf(stderr, "Missing input file.\n");
                return -1;
            }
            //Opening file, redirecting input to this
            fd = open(file, O_RDONLY); //Mode = Read only
            if (fd == -1)
            {
                fprintf(stderr, "File Open Error: %s\n", strerror(errno));
                return -1;
            }
            //I/O Redirection
            if (dup2(fd, STDIN_FILENO) == -1)
            {
                fprintf(stderr, "Dup2 Error: %s\n", strerror(errno));
                return -1;
            }
            //Closing old FD
            if (close(fd) == -1)
            {
                fprintf(stderr, "Close Error: %s\n", strerror(errno));
                return -1;
            }
        }
        if (argSig == 0)
        {
            //Copying arguments to argv
            argv[argvSize] = malloc((strlen(token) + 1) * sizeof(char));
            if (argv[argvSize] == NULL)
            {
                fprintf(stderr, "Malloc Error: %s\n", strerror(errno));
                return -1;
            }
            strcpy(argv[argvSize], token);
            argvSize = argvSize + 1;
            argv[argvSize] = NULL;
            if (argvSize >= maxSize - 2) //Realloc, and making sure there is room for NULL Ptr
            {
                maxSize = maxSize + 25;
                argv = realloc(argv, maxSize * sizeof(char *));
                if (argv == NULL)
                {
                    fprintf(stderr, "Realloc Error: %s\n", strerror(errno));
                    return -1;
                }
            }
        }
        token = strtok(NULL, " \n");
    }

    //EXECVP
    if (execvp(argv[0], argv) == -1)
    {
        fprintf(stderr, "EXECVP failed: %s\n", strerror(errno));
    }

    //Free up resource
    for (int i = 0; i < argvSize; i++)
    {
        free(argv[i]);
    }
    free(argv);
    return -1;
}

int getCommandLine(char **linePtr)
{
    //Fetching CWD for PS
    char cwd[256];
    if (getcwd(cwd, sizeof(cwd)) == NULL)
    {
        fprintf(stderr, "CWD Error: %s\n", strerror(errno));
        return -1;
    }
    printf("%s$ ", cwd);
    size_t n = 0; //Allocated size
    //Returns -1 on failure or the number of chars read
    return getline(linePtr, &n, stdin);
}