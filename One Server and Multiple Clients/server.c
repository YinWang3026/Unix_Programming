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

#include "pthread.h"

#define MAXCLIENT 200
#define MAXMESSG 256

//Strucs
typedef struct
{
    int fd;
    int id;
    char name[30];
} client;

typedef struct
{
    client *cpt;
    char message[1024];
} messg;

typedef struct
{
    int readPtr;
    int writePtr;
    int currSize;
    messg *messgArr[MAXMESSG];
} queue;

//Globals
client *clientArr[MAXCLIENT]; //A client pointer arr
pthread_mutex_t cMutex = PTHREAD_MUTEX_INITIALIZER;
queue queuePtr; //A queue and its lock and cond variable
pthread_mutex_t qMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t qCond = PTHREAD_COND_INITIALIZER;

//Prototype
int serverUp(char *, int);
void serverAccept(int);

int addClient(client *);
void rmClient(client *);
void broadcast(char *, client *);
void printClients(client *);
void serverCleanUp();

void *clientHandler(void *arg);
void *messageHandler(void *arg);
void *stdinHandler(void *arg);

int enqueue(messg *);
messg *dequeue();

int main(int argc, char *argv[])
{
    int port = 1234;        //Default port number
    char *ip = "127.0.0.1"; //Default ip
    char *garb = NULL;
    if (argc == 2)
    {
        port = strtol(argv[1], &garb, 10);
        if (strcmp(garb, "\0") != 0)
        {
            fprintf(stderr, "Invalid port.\n");
            return 0;
        }
    }
    else if (argc == 3)
    {
        ip = argv[1];
        port = strtol(argv[2], &garb, 10);
        if (strcmp(garb, "\0") != 0)
        {
            fprintf(stderr, "Invalid port.\n");
            return 0;
        }
    }
    else if (argc > 3)
    {
        fprintf(stderr, "Invalid arguments.\n");
        fprintf(stderr, "Port\n");
        fprintf(stderr, "IP Port\n");
        return 0;
    }
    printf("Server, IP: %s, Port: %d\n", ip, port);

    //Zeroing out the globals
    memset(clientArr, 0, sizeof(clientArr));
    queuePtr.readPtr = 0;
    queuePtr.writePtr = 0;
    queuePtr.currSize = 0;
    memset(queuePtr.messgArr, 0, sizeof(queuePtr.messgArr));

    //Initialzing message thread
    pthread_t mThread;
    pthread_create(&mThread, NULL, &messageHandler, NULL);
    if (pthread_detach(mThread) > 0)
    {
        fprintf(stderr, "Detach error: %s\n", strerror(errno));
        exit(1);
    }

    //Initialzing stdin thread
    pthread_t cThread;
    pthread_create(&cThread, NULL, &stdinHandler, NULL);
    if (pthread_detach(cThread) > 0)
    {
        fprintf(stderr, "Detach error: %s\n", strerror(errno));
        exit(1);
    }

    //Setting up socket
    int socfd = serverUp(ip, port);
    if (socfd == -1) //Error
    {
        return 0;
    }

    //Accepting and handlding requests
    serverAccept(socfd);

    //Closing server socket and clearing heap
    serverCleanUp();

    if (close(socfd) == -1)
    {
        fprintf(stderr, "Socket close error: %s\n", strerror(errno));
    }
    //If main exits, all threads should too
    return 0;
}

int addClient(client *c)
{
    pthread_mutex_lock(&cMutex);
    for (int i = 0; i < MAXCLIENT; i++)
    {
        //Put the client at the first NULL
        if (clientArr[i] == NULL)
        {
            clientArr[i] = c;
            pthread_mutex_unlock(&cMutex);
            return 0;
        }
    }
    pthread_mutex_unlock(&cMutex);
    return -1;
}

void rmClient(client *c)
{
    pthread_mutex_lock(&cMutex);
    for (int i = 0; i < MAXCLIENT; i++)
    {
        //Find the client by ID and set it NULL
        if (clientArr[i] != NULL && clientArr[i]->id == c->id)
        {
            free(clientArr[i]);
            clientArr[i] = NULL;
            break;
        }
    }
    pthread_mutex_unlock(&cMutex);
}

void broadcast(char *m, client *c)
{
    //Send to all but client C
    char message[128];
    memset(message, 0, sizeof(message));
    strcpy(message, m);
    pthread_mutex_lock(&cMutex);
    for (int i = 0; i < MAXCLIENT; i++)
    {
        if (clientArr[i] != NULL)
        {
            if (clientArr[i]->id != c->id)
            {
                if (write(clientArr[i]->fd, message, sizeof(message)) == -1)
                {
                    fprintf(stderr, "Broadcast write error to client id '%d': %s\n", clientArr[i]->id, strerror(errno));
                }
            }
        }
    }
    pthread_mutex_unlock(&cMutex);
}

void printClients(client *c)
{
    pthread_mutex_lock(&cMutex);
    char buffer[MAXCLIENT * 31 + 1];
    sprintf(buffer, "===Current Clients===\n");
    if (write(c->fd, buffer, sizeof(buffer)) == -1)
    {
        fprintf(stderr, "printClients write error: %s\n", strerror(errno));
    }
    sleep(1); //Slowing down the writes
    memset(buffer, 0, sizeof(buffer));
    //Add all names to a buffer to send
    for (int i = 0; i < MAXCLIENT; i++)
    {
        if (clientArr[i] != NULL)
        {
            if (clientArr[i]->id != c->id)
            {
                strcat(buffer, clientArr[i]->name);
                strcat(buffer, " ");
            }
        }
    }
    if (write(c->fd, buffer, sizeof(buffer)) == -1)
    {
        fprintf(stderr, "printClients write error: %s\n", strerror(errno));
    }
    sleep(1);
    sprintf(buffer, "\n=====================\n");
    if (write(c->fd, buffer, sizeof(buffer)) == -1)
    {
        fprintf(stderr, "printClients write error: %s\n", strerror(errno));
    }
    pthread_mutex_unlock(&cMutex);
}

void serverCleanUp()
{
    int i;
    for (i = 0; i < MAXCLIENT; i++)
    {
        if (clientArr[i] != NULL)
        {
            free(clientArr[i]);
        }
    }
    for (i = 0; i < MAXMESSG; i++)
    {
        if (queuePtr.messgArr[i] != NULL)
        {
            free(queuePtr.messgArr[i]);
        }
    }
}

void *clientHandler(void *arg)
{
    //Input a client pointer
    char buffer[1024];
    int result;
    messg *mpt;
    client *c = (client *)arg;

    //Fetch client name
    if (read(c->fd, c->name, sizeof(c->name)) == -1)
    {
        fprintf(stderr, "Read name error: %s\n", strerror(errno));
        free(c);
        return NULL;
    }
    //Add client to list
    if (addClient(c) == -1) //Full
    {
        fprintf(stderr, "Server full, add client failed.\n");
        sprintf(buffer, "--FULL--\n");
        if (write(c->fd, buffer, sizeof(buffer)) == -1)
        {
            fprintf(stderr, "Write error: %s\n", strerror(errno));
        }
        free(c);
        return NULL;
    }

    //Message new client the current users
    printClients(c);

    //Message other clients a client joined
    printf("%s has joined the server.\n", c->name);
    sprintf(buffer, "%s has joined the server.\n", c->name);
    broadcast(buffer, c);

    for (;;)
    {
        //Recieve messages from the clients
        memset(buffer, 0, sizeof(buffer));
        result = read(c->fd, buffer, sizeof(buffer));
        if (result == -1)
        {
            fprintf(stderr, "Read error: %s\n", strerror(errno));
            break;
        }
        else if (result == 0)
        {
            break;
        }
        if (strcmp(buffer, "\0") == 0) //Ignoring null char
        {
            continue;
        }

        //Add to the message queue
        mpt = (messg *)malloc(sizeof(messg));
        if (mpt == NULL)
        {
            fprintf(stderr, "Messg malloc error: %s\n", strerror(errno));
            exit(1); //Fatal
        }
        mpt->cpt = c;
        memset(mpt->message, 0, sizeof(mpt->message));
        strcpy(mpt->message, buffer);
        if (enqueue(mpt) == -1)
        {
            memset(buffer, 0, sizeof(buffer));
            strcpy(buffer, "Server: Message failed to send.\n");
            if (write(c->fd, buffer, sizeof(buffer)) == -1)
            {
                fprintf(stderr, "Client handler write error: %s\n", strerror(errno));
            }
        }
    }

    //Message other clients a client left
    printf("%s has left the server.\n", c->name);
    sprintf(buffer, "%s has left the server.\n", c->name);
    broadcast(buffer, c);

    //Closing connection
    if (close(c->fd) == -1)
    {
        fprintf(stderr, "Connection close error: %s\n", strerror(errno));
        exit(1);
    }
    //Removing from list
    rmClient(c);

    return NULL;
}

void *messageHandler(void *arg)
{
    messg *mpt;
    char msg[1064];
    for (;;)
    {
        mpt = dequeue();
        sprintf(msg, "%s: %s", mpt->cpt->name, mpt->message);
        broadcast(msg, mpt->cpt);
        free(mpt);
    }
    return NULL;
}

int enqueue(messg *m)
{
    //0 for success, 1 for failure
    pthread_mutex_lock(&qMutex);
    if (queuePtr.currSize == MAXMESSG)
    {
        pthread_mutex_unlock(&qMutex);
        return -1;
    }
    if (queuePtr.currSize == 0)
    {
        queuePtr.writePtr = 0;
        queuePtr.readPtr = 0;
    }
    queuePtr.messgArr[queuePtr.writePtr] = m;
    queuePtr.writePtr = queuePtr.writePtr + 1;
    queuePtr.currSize = queuePtr.currSize + 1;
    pthread_mutex_unlock(&qMutex);
    pthread_cond_signal(&qCond);
    return 0;
}

messg *dequeue()
{
    //messg obj for success, NULL for failure
    pthread_mutex_lock(&qMutex);
    while (queuePtr.currSize == 0)
    {
        pthread_cond_wait(&qCond, &qMutex);
    }
    messg *m = queuePtr.messgArr[queuePtr.readPtr];
    queuePtr.messgArr[queuePtr.readPtr] = NULL;
    queuePtr.readPtr = queuePtr.readPtr + 1;
    queuePtr.currSize = queuePtr.currSize - 1;
    pthread_mutex_unlock(&qMutex);
    return m;
}

void *stdinHandler(void *arg)
{
    char buffer[32];
    for (;;)
    {
        //Reading from stdin
        memset(buffer, 0, sizeof(buffer));
        if (read(0, buffer, sizeof(buffer)) == -1)
        {
            fprintf(stderr, "Read error: %s\n", strerror(errno));
            exit(1); //Fatal
        }
        if (strcmp(buffer, "--exit--\n") == 0) //Command end server
        {
            exit(1);
        }
    }
    return NULL;
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

void serverAccept(int socfd)
{
    int connfd;  //FD for incoming connections
    int cid = 0; //Giving each client an ID
    struct sockaddr_in clientAddr;
    memset(&clientAddr, 0, sizeof(clientAddr));
    socklen_t sockLen = sizeof(clientAddr);
    pthread_t aThread; //Thread for handling clients

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

        //Create the client obj
        client *c = (client *)malloc(sizeof(client));
        if (c == NULL)
        {
            fprintf(stderr, "Client malloc error: %s\n", strerror(errno));
            exit(1); //Fatal
        }
        c->id = cid;
        c->fd = connfd;

        cid++;
        //Spawn thread
        pthread_create(&aThread, NULL, &clientHandler, (void *)c);
        if (pthread_detach(aThread) > 0)
        {
            fprintf(stderr, "Detach error: %s\n", strerror(errno));
            exit(1); //Fatal
        }
    }
}