#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "unistd.h"

#include "dirent.h" //For directory
#include "sys/types.h"
#include "sys/stat.h" //For i node
#include "math.h"

long double du(char *pathName)
{
    //Returns -1 on errors, else returns size of dir

    struct dirent *direntp; //Directory info
    struct stat statBuf;    //Inode
    long double sum = 0;    //Total size of this directory
    long double result = 0; //Size of nested directory
    char *newPathName;      //New path name

    DIR *dirp = opendir(pathName); //Open directory
    if (dirp == NULL)
    {
        fprintf(stderr, "Cannot open directory\n");
        return -1;
    }

    while ((direntp = readdir(dirp)) != NULL) //Reading directory
    {
        //Ignoring .. and .
        if (strcmp(direntp->d_name, "..") != 0 && strcmp(direntp->d_name, ".") != 0)
        {
            //Forming new path with each obj in the current directory
            newPathName = malloc((strlen(pathName) + strlen(direntp->d_name) + 2) * sizeof(char));
            //+2 For incoming '/' and '\0'
            if (newPathName == NULL)
            {
                fprintf(stderr, "Memory Allocation error\n");
                return -1;
            }
            newPathName = strcpy(newPathName, pathName);
            newPathName = strcat(newPathName, "/");
            newPathName = strcat(newPathName, direntp->d_name);

            //Retriving i node info on new path
            if (lstat(newPathName, &statBuf) == -1)
            {
                fprintf(stderr, "lstat error\n");
                return -1;
            };

            //For debugging
            //printf("Dirname: %s\tblks: %ld\tsz: %ld\tblksz: %ld\n", direntp->d_name, statBuf.st_blocks, statBuf.st_size, statBuf.st_blksize);
            //printf("new path name: %s\n", newPathName);

            //Adding each obj blks to sum
            sum = sum + statBuf.st_blocks;

            //If any if the obj is a directory, call du() on it to find its size
            if (S_ISDIR(statBuf.st_mode))
            {
                //Calculating directory size
                result = du(newPathName);
                if (result == -1)
                {
                    fprintf(stderr, "Du Error\n");
                    return -1;
                }
                //Adding dir size to total
                sum = sum + result;
                //Size of current directory is itself + whats in it
                //Dividing by 2 and ceiling to convert from 512 byte blocks to 1024 byte blocks
                result = ceil((result + statBuf.st_blocks) / 2);
                printf("%ld\t%s\n", (long)result, newPathName);
            }
            //Freeing resources
            free(newPathName);
        }
    }
    closedir(dirp); //Close directory
    return sum;
}

int duDriver(char *pathName)
{
    //Returns -1 on failure, 0 on success
    long double sum = 0;
    struct stat statBuf;
    //Retriving i node info on input path
    if (lstat(pathName, &statBuf) == -1)
    {
        fprintf(stderr, "lstat error\n");
        return -1;
    };
    //If it is a directory, find its size recursively
    if (S_ISDIR(statBuf.st_mode))
    {
        sum = du(pathName);
        if (sum == -1)
        {
            fprintf(stderr, "Du Error\n");
            return -1;
        }
        sum = ceil((sum + statBuf.st_blocks) / 2);
    }
    //If it is not a directory, just return its size
    else
    {
        sum = ceil(statBuf.st_blocks / (long double)2);
    }
    printf("%ld\t%s\n", (long)sum, pathName);

    return 0;
}

int main(int argc, char *argv[])
{
    //Does not support flags
    //No arguments, du on current directory
    char *pathName;
    if (argc == 1)
    {
        pathName = ".";
    }
    //A given directory argv[1]
    else if (argc == 2)
    {
        pathName = argv[1];
    }
    else
    {
        fprintf(stderr, "Input error, try:\ndu\ndu pathName\n");
        return 0;
    }

    if (duDriver(pathName) == -1)
    {
        fprintf(stderr, "Du Error\n");
        fprintf(stderr, "Input error, try:\ndu\ndu pathName\n");
        return 0;
    }
    return 0;
}