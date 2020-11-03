#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "unistd.h" //Chdir, getcwd, fork, close, dup2
#include "errno.h"
#include "sys/types.h" //Fork, wait, open
#include "sys/wait.h"  //Wait
#include "sys/stat.h"  //Open
#include "fcntl.h"     //Open
#include "signal.h"    //Sigaction
#include "setjmp.h"    //Jumps

//Defs
typedef void (*sig_handler)(int);

//Prototypes
void shell();
int getCommandLine(char **linePtr);
int runCommand(char *linePtr);
int redirection(char *file, int flags, int mode, int fileNo);
int pipeRedir(int in, int out, char *cmd);
int pipeInit(char *cmd1, char *cmd2);
int sigHandler(sig_handler h, int sigNum);
void myHandler(int signalNum);

//Globals
jmp_buf buf;

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

    //Signal handlers
    if (sigHandler(myHandler, SIGINT) == -1)
    {
        exit(1); //Error 
    }
    if (sigHandler(SIG_IGN, SIGQUIT) == -1)
    {
        exit(1); //Error
    }

    if (sigsetjmp(buf, 1) == 2) //Buf and save sigs
    {
        printf("\n");
    }

    while (getCommandLine(&linePtr) != -1)
    {

        //Comparing linePtr to inputs
        if (strcmp(linePtr, "\n") == 0)
        {
            continue;
        }
        if (strcmp(linePtr, "exit\n") == 0)
        {
            //Exiting
            exit(0); //Terminated by user, successful exit
        }
        else if (strncmp(linePtr, "cd ", 3) == 0)
        {
            char *parse = strtok(linePtr, " \n");
            parse = strtok(NULL, " \n");
            //Takes char *
            //Returns 0 on succes, -1 on error
            if (chdir(parse) == -1)
            {
                fprintf(stderr, "CD Error: %s\n", strerror(errno));
            }
        }
        else if (strcmp(linePtr, "cd\n") == 0)
        {
            char *home = getenv("HOME");
            if (chdir(home) == -1)
            {
                fprintf(stderr, "CD Error: %s\n", strerror(errno));
            }
        }
        else
        {
            char *cmd1 = strtok(linePtr, "|\n"); //First exe
            char *cmd2 = strtok(NULL, "|\n");    //Second exe

            //Parent ignore SIGINT when child is running
            if (sigHandler(SIG_IGN, SIGINT) == -1)
            {
                break;
            }

            if (cmd2 == NULL) //No piping
            {
                //Running the exe with stdin and stdout
                if (pipeRedir(0, 1, cmd1) == -1)
                {
                    break;
                }
            }
            else
            {
                //Redirect piping before running
                if (pipeInit(cmd1, cmd2) == -1)
                {
                    break;
                }
            }
            //Child finished, restoring SIGINT handler
            if (sigHandler(myHandler, SIGINT) == -1)
            {
                break;
            }
        }
    }
    free(linePtr);
    exit(1); //Terminated by error, not successful exit
}

void myHandler(int signalNum)
{
    if (signalNum == SIGINT)
    {
        siglongjmp(buf, 2);
    }
}

int sigHandler(sig_handler h, int sigNum)
{
    //Changes handler for signal
    struct sigaction newAct;
    newAct.sa_handler = h;
    newAct.sa_flags = SA_RESTART;
    if (sigaction(sigNum, &newAct, NULL) == 1)
    {
        fprintf(stderr, "Sigaction Error: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

int pipeInit(char *cmd1, char *cmd2)
{
    int fd[2];
    if (pipe(fd) == -1)
    {
        fprintf(stderr, "Pipe Error: %s\n", strerror(errno));
        return -1;
    };

    if (pipeRedir(0, fd[1], cmd1) == -1) //Child read from stdin, write to fd[1]
    {
        return -1;
    }

    if (close(fd[1]) == -1) //Parent doesnt write to pipe
    {
        fprintf(stderr, "Close Error: %s\n", strerror(errno));
        return -1;
    }

    if (pipeRedir(fd[0], 1, cmd2) == -1) //Child reads from fd[0], writes to stdout
    {
        return -1;
    }

    if (close(fd[0]) == -1) //Close reading pipe
    {
        fprintf(stderr, "Close Error: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

int pipeRedir(int in, int out, char *cmd)
{
    int status;
    int PID = fork();
    if (PID == 0) //Child
    {
        //Returning signals to default handlers
        if (sigHandler(SIG_DFL, SIGINT) == -1)
        {
            return -1;
        }
        if (sigHandler(SIG_DFL, SIGQUIT) == -1)
        {
            return -1;
        }

        if (in != 0) //in -> stdin
        {
            if (dup2(in, 0) == -1)
            {
                fprintf(stderr, "Dup Error: %s\n", strerror(errno));
                return -1;
            }
            if (close(in) == -1) //Close old
            {
                fprintf(stderr, "Close Error: %s\n", strerror(errno));
                return -1;
            }
        }

        if (out != 1) //out -> stdout
        {
            if (dup2(out, 1) == -1)
            {
                fprintf(stderr, "Dup Error: %s\n", strerror(errno));
                return -1;
            }
            if (close(out) == -1) //Close old
            {
                fprintf(stderr, "Close Error: %s\n", strerror(errno));
                return -1;
            }
        }
        if (runCommand(cmd) == -1) //Execution error
        {
            return -1;
        }
    }
    else if (PID == -1) //Error
    {
        fprintf(stderr, "Fork Error: %s\n", strerror(errno));
        return -1;
    }

    if (wait(&status) == -1) //Parent waits
    {
        fprintf(stderr, "Wait Error: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

int runCommand(char *linePtr)
{
    //I/O Redirection
    char *file = NULL;
    int mode, flags, fileNo;

    //Agrv array
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
            //Read file name
            file = strtok(NULL, " \n");
            if (file == NULL || strcmp(file, ">") == 0 || strcmp(file, "<") == 0)
            {
                fprintf(stderr, "Missing output file.\n");
                return -1;
            }
            flags = O_CREAT | O_RDWR | O_TRUNC;
            mode = S_IRUSR | S_IWUSR;
            fileNo = STDOUT_FILENO;
            if (redirection(file, flags, mode, fileNo) == -1)
            {
                return -1;
            }
        }
        else if (strcmp(token, ">>") == 0) //Output append
        {
            file = strtok(NULL, " \n");
            if (file == NULL || strcmp(file, ">") == 0 || strcmp(file, "<") == 0)
            {
                fprintf(stderr, "Missing output file.\n");
                return -1;
            }
            flags = O_CREAT | O_RDWR | O_APPEND;
            mode = S_IRUSR | S_IWUSR;
            fileNo = STDOUT_FILENO;
            if (redirection(file, flags, mode, fileNo) == -1)
            {
                return -1;
            }
        }
        else if (strcmp(token, "2>") == 0) //Error
        {
            file = strtok(NULL, " \n");
            if (file == NULL || strcmp(file, ">") == 0 || strcmp(file, "<") == 0)
            {
                fprintf(stderr, "Missing error output file.\n");
                return -1;
            }
            flags = O_CREAT | O_RDWR | O_TRUNC;
            mode = S_IRUSR | S_IWUSR;
            fileNo = STDERR_FILENO;
            if (redirection(file, flags, mode, fileNo) == -1)
            {
                return -1;
            }
        }
        else if (strcmp(token, "2>>") == 0) //Error append
        {
            file = strtok(NULL, " \n");
            if (file == NULL || strcmp(file, ">") == 0 || strcmp(file, "<") == 0)
            {
                fprintf(stderr, "Missing error output file.\n");
                return -1;
            }
            flags = O_CREAT | O_RDWR | O_APPEND;
            mode = S_IRUSR | S_IWUSR;
            fileNo = STDERR_FILENO;
            if (redirection(file, flags, mode, fileNo) == -1)
            {
                return -1;
            }
        }
        else if (strcmp(token, "<") == 0) //Input
        {
            file = strtok(NULL, " \n");
            if (file == NULL || strcmp(file, ">") == 0 || strcmp(file, "<") == 0)
            {
                fprintf(stderr, "Missing input file.\n");
                return -1;
            }
            flags = O_RDONLY;
            mode = S_IRUSR;
            fileNo = STDIN_FILENO;
            if (redirection(file, flags, mode, fileNo) == -1)
            {
                return -1;
            }
        }
        else
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
    char *ps;
    ps = getenv("PS1");
    if (getcwd(cwd, sizeof(cwd)) == NULL)
    {
        fprintf(stderr, "getcwd Error: %s\n", strerror(errno));
        return -1;
    }

    if (ps != NULL) //If PS exists, use PS
    {
        printf("%s$ ", ps);
    }
    else //Else CWD
    {
        printf("%s$ ", cwd);
    }
    size_t n = 0; //Allocated size
    //Returns -1 on failure or the number of chars read
    return getline(linePtr, &n, stdin);
}

int redirection(char *file, int flags, int mode, int fileNo)
{
    int fd;
    //Opening file, redirecting input to this

    fd = open(file, flags, mode);
    if (fd == -1)
    {
        fprintf(stderr, "File Open Error: %s\n", strerror(errno));
        return -1;
    }

    //I/O Redirection
    if (dup2(fd, fileNo) == -1)
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
    return 0;
}