#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "unistd.h"
#include "errno.h"

extern char **environ;

void printFormat()
{
    printf("env\n");
    printf("env [-i] [name=value]... [command [args]...]]\n");
    printf("env [name=value]... [command [args]...]]\n");
    printf("'=' is needed for the entry to be registered as name=value\n");
    printf("Else, it is part of executable and args\n");
}

void printEnv(char **arr) //Printing environment
{
    for (int index = 0; arr[index] != NULL; ++index)
    {
        puts(arr[index]);
    }
}

//Struct for making a copy of ENV and size info
typedef struct
{
    int currSize;
    int maxSize;
    char **arr;
} ENVCPY;

int copyEnv(ENVCPY *myEnv)
{
    //Copies the env onto heap, returns 0 for error, 1 for success
    //Allocate space for first char
    int index;

    if (myEnv->arr == NULL)
    {
        fprintf(stderr, "Memory allocation error.\n");
        return 0;
    }

    for (index = 0; environ[index] != NULL; ++index)
    {
        //Allocate space for string
        myEnv->arr[index] = malloc((strlen(environ[index]) + 1) * sizeof(char));
        if (myEnv->arr[index] == NULL)
        {
            fprintf(stderr, "Memory allocation error.\n");
            return 0;
        }
        //Allocate space for next char
        strcpy(myEnv->arr[index], environ[index]);
        myEnv->currSize = myEnv->currSize + 1;
        if (myEnv->currSize == myEnv->maxSize)
        {
            myEnv->maxSize = myEnv->maxSize + 10;
            myEnv->arr = realloc(myEnv->arr, myEnv->maxSize * sizeof(char *));
            if (myEnv->arr == NULL)
            {
                fprintf(stderr, "Memory allocation error.\n");
                return 0;
            }
        }
    }
    //Terminate the arr with NULL
    myEnv->arr[index] = NULL;
    myEnv->currSize = myEnv->currSize + 1;
    return 1;
}

int modifyEnv(char *iname, char *ivalue, ENVCPY *myEnv)
{
    //Returns -1 for error, 1 for added space, 0 for no space added
    int index;
    for (index = 0; myEnv->arr[index] != NULL; ++index)
    {
        //Making a copy of environ[index] into temp for processing
        char *temp = malloc((strlen(ivalue) + strlen(myEnv->arr[index])) * sizeof(char));
        if (temp == NULL)
        {
            fprintf(stderr, "Memory allocation error.\n");
            return -1;
        }
        strcpy(temp, myEnv->arr[index]);
        //Using token to grab the name element from name=value
        if (strcmp(iname, strtok(temp, "=")) == 0)
        {
            //If name match, update the environ name with new value
            strcat(temp, "=\0"); //Concatenating temp + "=" + ivalue
            strcat(temp, ivalue);
            free(myEnv->arr[index]);  //Free the old
            myEnv->arr[index] = temp; //Updating environ to new name=value
            return 0;
        }
    }
    //Input name not in environ, adding it to the end
    char *temp = malloc((strlen(iname) + strlen(ivalue) + 1) * sizeof(char));
    if (temp == NULL)
    {
        fprintf(stderr, "Memory allocation error.\n");
        return -1;
    }
    //Concatenating the string
    strcat(temp, iname);
    strcat(temp, "=\0");
    strcat(temp, ivalue);
    //Attaching it to end of environ and setting new environ NULL
    free(myEnv->arr[index]);      //Free the old
    myEnv->arr[index] = temp;     //Updating environ to new name=value
    myEnv->arr[index + 1] = NULL; //Updating last environ to NULL
    return 1;
}

int argumentProcessing(int argc, char *argv[], int iFlag, ENVCPY *myEnv)
{
    //Use token to fetch the arguments
    char *name, *value;
    //i j are loop index, commandIndex is the index of executable in argv
    int i, modRes;
    for (i = iFlag + 1; i < argc; i++)
    {
        name = strtok(argv[i], "=");
        value = strtok(NULL, "=");
        //printf("name:%s\tvalue:%s\n", name, value);
        if (value != NULL) //A valid name=value
        {
            //Checking if there are at least 2 spaces
            //Might add 2 space in mod, 1 for new environment, 1 for NULL
            if (myEnv->currSize + 2 >= myEnv->maxSize)
            {
                myEnv->maxSize = myEnv->maxSize + 10;
                myEnv->arr = realloc(myEnv->arr, myEnv->maxSize * sizeof(char *));
                if (myEnv->arr == NULL)
                {
                    fprintf(stderr, "Memory allocation error.\n");
                    return -1;
                }
            }
            modRes = modifyEnv(name, value, myEnv);
            if (modRes == -1)
            {
                return -1; //Memory error
            }
            myEnv->currSize = myEnv->currSize + modRes; //Update size
        }
        else //If it doesnt have a value, agrv reached exectuable section
        {
            return i; //command args start at index i
        }
    }
    return -2; //For no exectuable
}

char **generateArgs(int numArgs, int argc, int commandIndex, char *argv[])
{
    //Size for executable and its args, plus one for NULL
    char **args = malloc((numArgs) * sizeof(char *));
    if (args == NULL)
    {
        fprintf(stderr, "Memory allocation error.\n");
        return NULL;
    }
    //Copying argv to args, commandIndex used as offset
    for (int i = commandIndex; i < argc; i++)
    {
        args[i - commandIndex] = malloc((strlen(argv[i]) + 1) * sizeof(char));
        if (args[i - commandIndex] == NULL)
        {
            fprintf(stderr, "Memory allocation error.\n");
            return NULL;
        }
        strcpy(args[i - commandIndex], argv[i]);
    }

    //Last element has to be NULL and calling execvp
    args[numArgs - 1] = NULL;

    return args;
}

int main(int argc, char *argv[])
{
    //env [name=value] [commands]
    //Modify the environment with new name=value
    //env -i [name=value] [commands]
    //Replace the environment with new name=value

    //Just the executable, then print environ
    if (argc == 1)
    {
        printEnv(environ);
        return 0;
    }

    //Second argument is "-i"
    int iFlag = 0;
    if (argv[1][0] == '-' && argv[1][1] == 'i' && argv[1][2] == '\0')
    {
        iFlag = 1;
    }
    ENVCPY myEnv; //Environment struct
    myEnv.currSize = 0;
    myEnv.maxSize = 10;
    myEnv.arr = malloc(myEnv.maxSize * sizeof(char *));
    // -i is set, ditch the environ
    if (iFlag)
    {
        for (int i = 0; i < myEnv.maxSize; i++)
        {
            myEnv.arr[i] = NULL;
        }
    }
    // -i is not set, copy the environ
    else
    {
        if (copyEnv(&myEnv) == 0)
        {
            fprintf(stderr, "Memory allocation error.\n");
            return 0;
        }
    }

    //printf("=========Original Environ=========\n");
    //printEnv(myEnv.arr);

    //Parsing arguments, modifying the environment
    //commandIndex is index of the executable

    int commandIndex = argumentProcessing(argc, argv, iFlag, &myEnv);
    //If commandIndex is -1, there is an error or no executable provided
    if (commandIndex == -1)
    {
        fprintf(stderr, "Error in argument processing.\n");
        return 0;
    }
    //No executable provided, will print the modified env only
    else if (commandIndex == -2)
    {
        printEnv(myEnv.arr);
        return 0;
    }

    //printf("=========Modified Environ=========\n");
    //printEnv(myEnv.arr);

    //Execute the command with the new environment
    int numArgs = argc - commandIndex + 1;
    //Making the argument array
    char **args = generateArgs(numArgs, argc, commandIndex, argv);
    if (args == NULL)
    {
        fprintf(stderr, "Memory allocation error.\n");
        return 0;
    }

    //Print args
    /*for (int i = 0; i < numArgs; i++)
    {
        printf("%s ", args[i]);
    }*/

    //Redircted the environ to my modified copy
    environ = myEnv.arr;

    //EXECVP
    if (execvp(argv[commandIndex], args) == -1)
    {
        printf("EXECVP failed\n%s\n", strerror(errno));
        printFormat();
    }

    //=======Below only execute if execvp returns -1==========
    //Freeing up the allocated memory for environ
    for (int i = 0; i < myEnv.maxSize; i++)
    {
        free(myEnv.arr[i]);
    }
    free(myEnv.arr);

    //Free up the executable array and its args
    for (int i = 0; i < numArgs; i++)
    {
        free(args[i]);
    }
    free(args);

    return 0;
}