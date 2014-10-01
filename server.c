#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <pthread.h>


void *connection_handler(void *);


int main(int argc, char *argv[])
{
    int socket_desc, new_socket, c, *new_sock;
    struct sockaddr_in server, client;
    char *message;


    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1)
    {
        printf("could not create socket\n");
    }
    

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( 8888);

    if ( bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        puts("bind failed");
        return 1;
    }
    puts("bind done");

    
    listen(socket_desc, 3);

    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);
    while ( (new_socket = accept(socket_desc, (struct sockaddr*)&client, (socklen_t*)&c)) > 0 )
    {
        puts("Connection accepted");

        message = "Hello Client, I have received your connection. And now I will assign you a handler.\n";
        write(new_socket, message, strlen(message));
        
        pthread_t sniffer_thread;
        new_sock = malloc(1);
        *new_sock = new_socket;

        if ( pthread_create( &sniffer_thread, NULL, connection_handler, (void*)new_sock) < 0)
        {
            perror("could not create thread");
            return 1;
        }
        
        puts("handler assigned");
    }
    
    if (new_socket < 0)
    {
        perror("accept failed");
        return 1;
    }

    shutdown(socket_desc, SHUT_RDWR);

    return 0;
}


void *connection_handler(void *socket_desc)
{
    int sock = *(int *)socket_desc;
    int read_size = 0;
    char *message, client_message[2000];
    struct timeval tv;

    message = "Greetings! I am your connection handler\n";
    write(sock, message, strlen(message));
    
    message = "Now type something and i shall repeat what you type\n";
    write(sock, message, strlen(message));

    // while( (read_size = recv(sock, client_message, 2000, 0)) > 0)
    // {
    //     write(sock, client_message, strlen(client_message));
    // }

    tv.tv_sec = 120;
    tv.tv_usec = 0;
    

    while (1)
    {
        fd_set rds;
        FD_SET(sock, &rds);
        
        int activity = select( sock+1, &rds, NULL, NULL, &tv);
        if (activity < 0)
        {
            perror("select: error");
            break;
        }
        else if (activity == 0)
        {
            printf("timeout occurred! No data for 2 mins.\n");
            continue;
        }
        
        if (FD_ISSET(sock, &rds))
        {
            printf("There was some read set sorta activity on socket %d\n", sock);
            read_size = recv(sock, client_message, 2000, 0);
            if (read_size > 0)
            {
                write(sock, client_message, strlen(client_message));
            }
            else
            {
                puts("read size 0 or error");
                break;
            }
        }
               
    }

    if (read_size == 0)
    {
        puts("Client disconnected");
        fflush(stdout);
    }
    else if(read_size == -1)
    {
        perror("recv failed");
    }

    free(socket_desc);
    
    return 0;
    
}
