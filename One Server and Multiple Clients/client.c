#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "unistd.h"
#include "errno.h"

#include "sys/socket.h" //socket, AF_INET, SOCK_STREAM, bind, listen, accept
#include "sys/types.h"
#include "netinet/in.h" //servaddr, INADDR_ANY, htons
#include "arpa/inet.h"  //inet_ntop, inet_pton

//1. Socket
//2. Connect
//3. Write to server
//4. Read from server
//5. Close socket

//Prototype
int clientUp(char *, int);
void clientRW(char *, int);

int main(int argc, char *argv[])
{
    int port = 1234;        //Default port number
    char *ip = "127.0.0.1"; //Default ip
    char *name = "Client";  //Default name
    char *garb = NULL;
    if (argc == 2)
    {
        name = argv[1];
    }
    else if (argc == 3)
    {
        name = argv[1];
        port = strtol(argv[2], &garb, 10);
        if (strcmp(garb, "\0") != 0)
        {
            fprintf(stderr, "Invalid port.\n");
            return 0;
        }
    }
    else if (argc == 4)
    {
        name = argv[1];
        ip = argv[2];
        port = strtol(argv[3], &garb, 10);
        if (strcmp(garb, "\0") != 0)
        {
            fprintf(stderr, "Invalid port.\n");
            return 0;
        }
    }
    else if (argc > 4)
    {
        fprintf(stderr, "Invalid arguments.\n");
        fprintf(stderr, "Name\n");
        fprintf(stderr, "Name Port\n");
        fprintf(stderr, "Name IP Port\n");
        return 0;
    }

    if (strlen(name) > 30)
    {
        fprintf(stderr, "Name too long. Max lenth = 30 chars.\n");
        return 0;
    }
    printf("Name: %s, IP: %s, Port: %d\n", name, ip, port);

    //Setting up socket
    int socfd = clientUp(ip, port);
    if (socfd == -1)
    {
        return 0;
    }

    //If return from this, done with connection regardless of reason
    clientRW(name, socfd);

    //Closing server socket
    if (close(socfd) == -1)
    {
        fprintf(stderr, "Socket close error: %s\n", strerror(errno));
    }
    return 0;
}

int clientUp(char *ip, int port)
{
    //Socket
    int socfd = socket(AF_INET, SOCK_STREAM, 0); //TCP/IP
    if (socfd == -1)
    {
        fprintf(stderr, "Socket creation error: %s\n", strerror(errno));
        return -1;
    }

    //Connect
    struct sockaddr_in servAddr;                         //IPV4 Socket address
    memset(&servAddr, 0, sizeof(servAddr));              //Zeroing the struct
    servAddr.sin_family = AF_INET;                       //IPV4
    servAddr.sin_port = htons(port);                     //My port
    if (inet_pton(AF_INET, ip, &servAddr.sin_addr) <= 0) //Text to binary, src, dst
    {
        fprintf(stderr, "PTON creation error: %s\n", strerror(errno));
        return -1;
    }

    //Server fd, sockaddr *, sockLen
    if (connect(socfd, (struct sockaddr *)&servAddr, sizeof(servAddr)) == -1)
    {
        fprintf(stderr, "Connection error: %s\n", strerror(errno));
        return -1;
    }
    return socfd;
}

void clientRW(char *name, int socfd)
{
    char buffer[1024]; //Only reads 1024 chars a time
    int sret, result;

    fd_set readfds;
    struct timeval timeout;
    
    memset(buffer, 0, sizeof(buffer));
    strcpy(buffer, name);
    if (write(socfd, buffer, sizeof(buffer)) == -1)
    {
        fprintf(stderr, "Write error: %s\n", strerror(errno));
        return;
    }

    for (;;)
    {
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
        FD_ZERO(&readfds);       //Need to do this before select everytime
        FD_SET(0, &readfds);     //Stdin
        FD_SET(socfd, &readfds); //Connection

        sret = select(10, &readfds, NULL, NULL, &timeout); //Monitor 0 - 10 Fds
        if (sret == -1)
        {
            fprintf(stderr, "Select error: %s\n", strerror(errno));
            return;
        }
        else if (sret > 0) //1 Or more FDS ready
        {
            if (FD_ISSET(0, &readfds)) //Check FD 0 is ready
            {
                memset(buffer, 0, sizeof(buffer));
                //Read stdin to buffer
                if (read(0, buffer, sizeof(buffer)) == -1)
                {
                    fprintf(stderr, "Read error: %s\n", strerror(errno));
                    return;
                }
                if (strcmp(buffer, "--exit--\n") == 0) //Command to d/c from server
                {
                    return;
                }
                //Buffer to socfd connection
                if (write(socfd, buffer, sizeof(buffer)) == -1)
                {
                    fprintf(stderr, "Write error: %s\n", strerror(errno));
                    return;
                }
                printf("%s: %s", name, buffer);
                FD_CLR(0, &readfds);
            }
            else if (FD_ISSET(socfd, &readfds)) //Check FD socfd is ready
            {
                memset(buffer, 0, sizeof(buffer));
                //Read from connection/client
                result = read(socfd, buffer, sizeof(buffer));
                if (result == -1)
                {
                    fprintf(stderr, "Read error: %s\n", strerror(errno));
                    return;
                }
                else if (result == 0)
                {
                    fprintf(stderr, "Server terminated.\n");
                    return;
                }
                if (strcmp(buffer, "--FULL--\n") == 0)
                {
                    fprintf(stderr, "Server full.\n");
                    return;
                }
                printf("%s", buffer);
                FD_CLR(socfd, &readfds);
            }
        }
    }
}