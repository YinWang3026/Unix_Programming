#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "unistd.h"
#include "errno.h"

#include "sys/socket.h" //socket, AF_INET, SOCK_STREAM, bind, listen, accept
#include "sys/types.h"
#include "netinet/in.h" //servaddr, INADDR_ANY, htons
#include "arpa/inet.h"  //inet_ntop
#include "sys/select.h" //pselect

//1. Socket
//2. Bind
//3. Listen
//4. Accept
//5. Read from client
//6. Write to client
//7. Close connection
//8. Close socket

//Prototype
int serverUp(char *, int);
void serverAccept(char *, int);
void serverRW(char *, int);

int main(int argc, char *argv[])
{
    int port = 1234;        //Default port number
    char *ip = "127.0.0.1"; //Default ip
    char *name = "Server";  //Default name
    char *s = NULL;
    if (argc == 2)
    {
        name = argv[1];
    }
    else if (argc == 3)
    {
        name = argv[1];
        port = strtol(argv[2], &s, 10);
        if (strcmp(s, "\0") != 0)
        {
            fprintf(stderr, "Invalid port.\n");
            return 0;
        }
    }
    else if (argc == 4)
    {
        name = argv[1];
        ip = argv[2];
        port = strtol(argv[3], &s, 10);
        if (strcmp(s, "\0") != 0)
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
    printf("Name: %s, IP: %s, Port: %d\n", name, ip, port);

    //Setting up socket
    int socfd = serverUp(ip, port);
    if (socfd == -1) //Error
    {
        return 0;
    }

    //Accepting and handlding requests
    //If return from this, done with connection regardless of reason
    serverAccept(name, socfd);

    //Closing server socket
    if (close(socfd) == -1)
    {
        fprintf(stderr, "Socket close error: %s\n", strerror(errno));
    }
    return 0;
}

int serverUp(char *ip, int port)
{
    //Socket
    int socfd = socket(AF_INET, SOCK_STREAM, 0); //TCP/IP
    if (socfd == -1)
    {
        fprintf(stderr, "Socket creation error: %s\n", strerror(errno));
        return -1;
    }
    //Bind
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
    if (bind(socfd, (struct sockaddr *)&servAddr, sizeof(servAddr)) == -1)
    {
        fprintf(stderr, "Bind error: %s\n", strerror(errno));
        return -1;
    }
    //Listen - passive mode
    int queueSize = 10;
    if (listen(socfd, queueSize) == -1) //socFD, queue size
    {
        fprintf(stderr, "Listen error: %s\n", strerror(errno));
        return -1;
    }
    return socfd;
}

void serverAccept(char *name, int socfd)
{
    int connfd; //FD for incoming connections
    struct sockaddr_in clientAddr;
    memset(&clientAddr, 0, sizeof(clientAddr));
    socklen_t sockLen = sizeof(clientAddr);

    for (;;)
    {
        //Accept connection
        //Returns connection FD, takes addr and sizeof(addr) if peer info is wanted
        connfd = accept(socfd, (struct sockaddr *)&clientAddr, &sockLen);
        if (connfd == -1)
        {
            fprintf(stderr, "Accept error: %s\n", strerror(errno));
            continue; //Move on to next connection
        }

        //Printing incoming ip:port
        char printable[sockLen];
        if (inet_ntop(AF_INET, &clientAddr.sin_addr, printable, sockLen) == NULL)
        {
            //af,src,dst,socklen
            fprintf(stderr, "NTOP error: %s\n", strerror(errno));
            //Cant convert ip is okay, not terminal error
        }
        printf("Got a connction from: %s : %d\n", printable, ntohs(clientAddr.sin_port));

        //Read and write
        //If returns, moving on to next connection
        serverRW(name, connfd);

        //Close connection
        if (close(connfd) == -1)
        {
            fprintf(stderr, "Connection close error: %s\n", strerror(errno));
            exit(1);
            //Can't close could be terminal
        }
    }
}

void serverRW(char *name, int connfd)
{
    char buffer[1024]; //Only reads 1024 chars a time
    char wBuffer[strlen(name) + 5 + 1024];
    int sret, result;

    fd_set readfds;
    struct timeval timeout;

    for (;;)
    {
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
        FD_ZERO(&readfds);        //Need to do this before select everytime
        FD_SET(0, &readfds);      //Stdin
        FD_SET(connfd, &readfds); //Connection

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
                //Read stdin to write buffer
                if (read(0, buffer, sizeof(buffer)) == -1)
                {
                    fprintf(stderr, "Read error: %s\n", strerror(errno));
                    return;
                }
                if (strcmp(buffer, "--exit--\n") == 0) //Command to d/c from server
                {
                    return;
                }
                memset(wBuffer, 0, sizeof(wBuffer));
                strcpy(wBuffer, name);
                strcat(wBuffer, ": ");
                strcat(wBuffer, buffer);
                //Write buffer to connfd
                if (write(connfd, wBuffer, sizeof(wBuffer)) == -1)
                {
                    fprintf(stderr, "Write error: %s\n", strerror(errno));
                    return;
                }
                printf("%s", wBuffer);
                FD_CLR(0, &readfds);
            }
            else if (FD_ISSET(connfd, &readfds)) //Check FD connfd is ready
            {
                memset(buffer, 0, sizeof(buffer));
                //Read from connection/client
                result = read(connfd, buffer, sizeof(buffer));
                if (result == -1)
                {
                    fprintf(stderr, "Read error: %s\n", strerror(errno));
                    return;
                }
                else if (result == 0)
                {
                    fprintf(stderr, "Client terminated.\n");
                    return;
                }
                printf("%s", buffer);
                FD_CLR(connfd, &readfds);
            }
        }
    }
}