#include "stdio.h"
#include "string.h"
#include "stdlib.h"

void printWorldB(char **arr, int row, int col) //Print the world with boundaries
{
    for (int i = 0; i < row; i++)
    {
        for (int j = 0; j < col; j++)
        {
            printf("%c", arr[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}

void printWorld(char **arr, int row, int col) //Print the world without boundaries
{
    for (int i = 1; i < row + 1; i++)
    {
        for (int j = 1; j < col + 1; j++)
        {
            printf("%c", arr[i][j]);
        }
        printf("\n");
    }
    printf("================================\n");
}

void printNeighborB(int **arr, int row, int col) //Print the neighbors with boundaries
{
    for (int i = 0; i < row; i++)
    {
        for (int j = 0; j < col; j++)
        {
            printf("%d", arr[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}

void clearNeighbor(int **arr, int row, int col) //Restting neighbors count to zero
{
    for (int i = 0; i < row; i++)
    {
        for (int j = 0; j < col; j++)
        {
            arr[i][j] = 0;
        }
    }
}

void life(int row, int col, int gen, char **world, int **neighbors)
{
    int genCount = 0;
    int i, j;

    //Printing the first generation
    printf("Generation: %d\n", genCount);
    printWorld(world, row, col);

    //Iterating the generations
    while (genCount < gen)
    {
        genCount = genCount + 1;
        printf("Generation: %d\n", genCount);
        //Counting Neighbors
        for (i = 1; i < row + 1; i++)
        {
            for (j = 1; j < col + 1; j++)
            {
                if (world[i - 1][j - 1] == '*')
                {
                    neighbors[i][j] = neighbors[i][j] + 1;
                }
                if (world[i - 1][j] == '*')
                {
                    neighbors[i][j] = neighbors[i][j] + 1;
                }
                if (world[i - 1][j + 1] == '*')
                {
                    neighbors[i][j] = neighbors[i][j] + 1;
                }
                if (world[i][j - 1] == '*')
                {
                    neighbors[i][j] = neighbors[i][j] + 1;
                }
                if (world[i][j + 1] == '*')
                {
                    neighbors[i][j] = neighbors[i][j] + 1;
                }
                if (world[i + 1][j - 1] == '*')
                {
                    neighbors[i][j] = neighbors[i][j] + 1;
                }
                if (world[i + 1][j] == '*')
                {
                    neighbors[i][j] = neighbors[i][j] + 1;
                }
                if (world[i + 1][j + 1] == '*')
                {
                    neighbors[i][j] = neighbors[i][j] + 1;
                }
            }
        }
        //Printing neighbors
        //printNeighborB(neighbors, row + 2, col + 2);

        //Update the world
        for (i = 1; i < row + 1; i++)
        {
            for (j = 1; j < col + 1; j++)
            {
                if (world[i][j] == '*')
                {
                    if (neighbors[i][j] < 2)
                    {
                        world[i][j] = '-';
                    }
                    else if (neighbors[i][j] == 2 || neighbors[i][j] == 3)
                    {
                        world[i][j] = '*';
                    }
                    else if (neighbors[i][j] > 3)
                    {
                        world[i][j] = '-';
                    }
                }
                else if (world[i][j] == '-' && neighbors[i][j] == 3)
                {
                    world[i][j] = '*';
                }
            }
        }
        clearNeighbor(neighbors, row + 2, col + 2);
        printWorld(world, row, col);
    }
}

void printFormat()
{
    printf("Incorrect Inputs, please use the following.\n");
    printf("Do not enter 0s for rows/columns/generations.\n");
    printf("life\n");
    printf("life rows columns\n");
    printf("life rows columns fileName\n");
    printf("life rows columns fileName generations\n");
}

int main(int argc, char *argv[])
{
    //life rows columns filename generations
    int row, col, gen;
    char *fileName;
    //Default values
    row = 10;
    col = 10;
    gen = 10;
    fileName = "life.txt";
    //Fetching command line arguments
    if (argc == 3) //life rows columns
    {
        row = atoi(argv[1]);
        col = atoi(argv[2]);
        if (row == 0 || col == 0)
        {
            printFormat();
            return 0;
        }
    }
    else if (argc == 4) //life rows columns filename
    {
        row = atoi(argv[1]);
        col = atoi(argv[2]);
        if (row == 0 || col == 0)
        {
            printFormat();
            return 0;
        }
        fileName = argv[3];
    }
    else if (argc == 5) //life rows columns filename generations
    {
        row = atoi(argv[1]);
        col = atoi(argv[2]);
        gen = atoi(argv[4]);
        if (row == 0 || col == 0 || gen == 0)
        {
            printFormat();
            return 0;
        }

        fileName = argv[3];
    }
    else if (argc >= 6 || argc == 2)
    {
        printFormat();
        return 0;
    }

    printf("row: %d, col: %d, gen: %d, filename: %s\n", row, col, gen, fileName);

    //Opening a file
    FILE *fptr;
    fptr = fopen(fileName, "r");
    if (fptr == NULL) //Error Checking
    {
        fprintf(stderr, "Cannot open file.\n");
        return 0;
    }

    //Allocating a 2d char array on the heap
    char **world;             //Pointer to pointers
    const int rowb = row + 2; //Addubg 2 for top/bottom boundary
    const int colb = col + 2; //Adding 2 for left/right boundary

    world = malloc((rowb) * sizeof(char *));
    if (world == NULL)
    {
        fprintf(stderr, "Memory allocation error.\n");
        return 0;
    }

    int i, j;

    for (i = 0; i < rowb; i++)
    {
        world[i] = malloc((colb) * sizeof(char));
        if (world[i] == NULL)
        {
            fprintf(stderr, "Memory allocation error.\n");
            return 0;
        }
    }

    //Initialzing the world such that every cell is dead.
    for (i = 0; i < rowb; i++)
    {
        for (j = 0; j < colb; j++)
        {
            world[i][j] = '-';
        }
    }

    //Getline(linePtr allocSize filePtr)
    char *linePtr = NULL; //Ptr to a line addr read
    size_t n = 0;         //Allocated space for line
    ssize_t result;       //Number of characters read, -1 on EOF
    //size_t is the size of an allocated block of memory
    //ssize_t is the signed size_t

    int lineLen, rowCount; //Line length and row count
    rowCount = 0;
    while ((result = getline(&linePtr, &n, fptr)) != -1 && rowCount < row) //Extra row cells are ignored
    {
        lineLen = strlen(linePtr) - 1; //Sub 1 for return char
        if (lineLen > col)
        { //Extra col cells are ignored
            lineLen = col;
        }
        //Copying read line into world array
        for (i = 0; i < lineLen; i++)
        {
            if (linePtr[i] == '*')
            {
                world[rowCount + 1][i + 1] = linePtr[i];
            }
        }
        rowCount = rowCount + 1;
    }

    //Printing world to check after reading file updates
    //printf("World with boundary.\n");
    //printWorldB(world, rowb, colb);

    //Printing the world without boundaries
    //printf("World without boundary.\n");
    //printWorld(world, row, col);

    //Initialzing an neighbor array to keep count
    int **neighbors;
    neighbors = malloc((rowb) * sizeof(int *));
    if (neighbors == NULL)
    {
        fprintf(stderr, "Memory allocation error.\n");
        return 0;
    }

    for (i = 0; i < rowb; i++)
    {
        neighbors[i] = malloc((colb) * sizeof(int));
        if (neighbors[i] == NULL)
        {
            fprintf(stderr, "Memory allocation error.\n");
            return 0;
        }
    }

    //Settomg the neighbors to 0
    clearNeighbor(neighbors, rowb, colb);

    //Life starts
    life(row, col, gen, world, neighbors);
    //Life ends

    //Close file
    fclose(fptr);

    //Free up line pointer
    free(linePtr);

    //Free up world
    for (i = 0; i < rowb; i++)
    {
        free(world[i]);
    }
    free(world);

    //Free up neighbor
    for (i = 0; i < rowb; i++)
    {
        free(neighbors[i]);
    }
    free(neighbors);

    return 0;
}
