#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#define CONNMAX 1000
#define BYTES 1024
#define MAX_CLIENTS 10
#define FILE_PATH "/LinuxMonitor.json"  // not used
#define MSG_LEN 10000
#define SOCK_ADDR "localhost"
#define SOCK_PORT 8080

int clients[MAX_CLIENTS] = {0};


char *ROOT;

void manageConnection(unsigned int fd)
{
    struct sockaddr_in client_addr;
    int addrSize = sizeof(client_addr);

    int incom = accept(fd, (struct sockaddr *)&client_addr, (socklen_t *)&addrSize);
    if (incom < 0)
    {
        perror("Error in accept(): ");
        exit(-1);
    }

    printf("\nNew connection: \nfd = %d \nip = %s:%d\n", incom, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i] ==  0)
        {
            clients[i] = incom;
            printf("Managed as client #%d\n", i);
            break;
        }
    }
}

void disconnectClient(unsigned int fd, unsigned int client_id, struct sockaddr_in client_addr)
{  
    int addrSize = sizeof(client_addr);
    getpeername(fd, (struct sockaddr *)&client_addr,(socklen_t *)&addrSize);
    printf("Client %d disconnected %s:%d \n", client_id, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    close(fd);
    clients[client_id] = 0;
}

int GetFileSize(char *filepath)
{
    FILE* f = fopen(filepath, "r");

    if (!f)
    {   
        printf("Can't open file\n");
        return -1;
    }
    int size = 0;

    while(fgetc(f) != EOF)
    {
        size++;
    }

    fclose(f);
    return size;
}

char* readFile(char *filepath, int size)
{
    FILE* f = fopen(filepath, "r");
    char* buf = calloc(size, sizeof(char));
    char c;
    int i = 0;

    if (!f)
    {   
        printf("Can't open file\n");
        exit(-1);
    }

    for (int i = 0; i < size; i++)
    {
        fread(&c, 1, 1, f);
        buf[i] = c;
    }

    fclose(f);
    return buf;
}

void manageRequest(unsigned int fd, char* filepath)
{    
    int fileSize = GetFileSize(filepath);
    if (fileSize == -1)
    {
        send(fd, "HTTP/1.1 404 Not Found\r\n\n", 25, 0);
    }
    else
    {
    	char *buffer = readFile(filepath, fileSize);

        int total = 0;
        int n;
        send(fd, "HTTP/1.1 200 OK\r\n", 17,0);
        send(fd, "Content-Type: text/html; charset=utf-8\r\n\r\n", 42, 0);
        char *message = strtok(buffer, "\n");
        while(message != NULL)
        {
            n = send(fd, message, strlen(message), 0); 
            if(n == -1) { perror("Error: unable to send file\n"); exit(-1); }
            total += strlen(message);
            message = strtok(NULL, "\n");
        }       
    }
}

void manageClient(unsigned int fd, unsigned int client_id)
{
    char msg[99999], *reqline[3], data_to_send[BYTES], path[99999];
    int rcvd, bytes_read, file;
    memset( (void*)msg, (int)'\0', 99999 );

    struct sockaddr_in client_addr;

    int recvSize = recv(fd, msg, MSG_LEN, 0);
    if (recvSize ==  0)
    {
       disconnectClient(fd, client_id, client_addr);
    }
    else
    {
        printf("%s\n", msg);
        reqline[0] = strtok(msg, " \t\n");
        if (strncmp(reqline[0], "GET\0", 4) == 0)
        {
            reqline[1] = strtok (NULL, " \t");
            if ( strncmp(reqline[1], "/\0", 2) == 0 )
                printf("Error: no file specified\n");
            else
            {                
                strcpy(path, reqline[1]);
                printf("Sending file: %s\n", path);
                manageRequest(fd, path);
            }   
        }
        disconnectClient(fd, client_id, client_addr);
    }
}

int main(void)
{
	
	time_t current, lastTaken;
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0)
    {
        perror("Error in sock(): ");
        return listener;
    }

    char PORT[6];
    ROOT = getenv("PWD");
    strcpy(PORT,"10000");

    fcntl(listener, F_SETFL, O_NONBLOCK);

    struct sockaddr_in client_addr;
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(SOCK_PORT);
    client_addr.sin_addr.s_addr = INADDR_ANY; 

    if (bind(listener, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0)
    {
        perror("Error in bind():");
        return -1;
    }
    printf("Server is up.\nHost %s port %d\n",SOCK_ADDR, SOCK_PORT);

    if (listen(listener, 10) < 0)
    {
        perror("Error in listen():");
        return -1;
    }
    printf("Waiting for the connections\n");

    while(1)
    {     	   	
        fd_set readfds; 
        int max_fd;     
        int active_clients_count; 

        FD_ZERO(&readfds);
        FD_SET(listener, &readfds);
        max_fd = listener;

        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            int fd = clients[i];

            if (fd > 0)
            {
                FD_SET(fd, &readfds);
            }

            max_fd = (fd > max_fd) ? (fd) : (max_fd);
        }

        active_clients_count = select(max_fd + 1, &readfds, NULL, NULL, NULL);

        if (active_clients_count < 0 && (errno !=  EINTR))
        {
            perror("Error in select():");
            return active_clients_count;
        }

        if (FD_ISSET(listener, &readfds))
        {
            manageConnection(listener);
        }

        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            int fd = clients[i];
            if ((fd > 0) && FD_ISSET(fd, &readfds))
            {
                manageClient(fd, i);
            }
        }
    }
    return 0;
}