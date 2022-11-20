#include <strings.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "http.h"

#define MAXLINE 4096
#define SA struct sockaddr
#define BACKLOG 10
#define BUFFER_SIZE 512

// ############ Resources ############
// resource definition
typedef struct Resource {
    char *path;
    char *data;
    struct Resource * next;
} Resource;
// resource initialisation
Resource *initializeResource() {
    Resource * head = NULL;
    head = (Resource *) malloc(sizeof(Resource));

    return head;
}
// find a resource in linked list
bool findResource(Resource *head, char *path, char *data) {
    Resource *current = head;

    while (current != NULL) {
        if(current->path != NULL || current->data != NULL)
            if (strcmp(current->path, path) == 0 && strcmp(current->data, data) == 0)
                return true;

        current = current->next;
    }

    return false;
}
// add a resource in linked list
void addResource(Resource *head, char *path, char *data) {
    Resource *current = head;

    if (current->data == NULL || current->path == NULL) {
        head->data = data;
        head->path = path;
        head->next = NULL;
        return;
    }

    while (current->next != NULL)
        current = current->next;

    current->next = (Resource*) malloc(sizeof(Resource));
    current->next->data = data;
    current->next->path = path;
    current->next->next = NULL;
}
// delete a resource by value from a linked list
Resource *deleteResource(Resource *head, char *path, char *data) {
    Resource *current = head;
    Resource *prev = NULL;

    if (head == NULL)
        return NULL;


    while (strcmp(current->path, path) != 0) {

        if (current->next == NULL) {
            return NULL;
        } else {
            prev = current;
            current = current->next;
        }
    }

    if (current == head) {
        head = head->next;
    } else {
        prev->next = current->next;
    }

    return current;
}
// free resource linked list
void freeRescource(Resource *head) {

    Resource * tmp;

    while (head != NULL)
    {
        tmp = head;
        head = head->next;
        free(tmp);
    }

}
// ####################################

// ############ HTTP Response Codes ############
// 404 -> Not Found
Response* Resp_NotFound(Response * resp) {
    resp->reason = "Not found";
    resp->status = 404;
    resp->payload = NULL;
    return resp;
}
// 200 -> Ok
Response* Resp_Ok(Response * resp) {
    resp->reason = "Ok";
    resp->status = 200;
    return resp;
}
// 400 -> Bad Request
Response* Resp_BadRequest(Response * resp) {
    resp->status = 400;
    resp->reason = "Bad Request";
    resp->payload = NULL;
    return resp;
}
// 201 -> Created
Response* Resp_Created(Response * resp) {
    resp->status = 201;
    resp->reason = "Created";
    resp->payload = NULL;
    return resp;
}
// 403 -> Forbidden
Response* Resp_Forbidden(Response * resp) {
    resp->status = 403;
    resp->reason = "Forbidden";
    resp->payload = NULL;
    return resp;
}
// 501 -> Other Request
Response* Resp_OtherRequest(Response * resp) {
    resp->status = 501;
    resp->reason = "Other Request";
    resp->payload = NULL;
    return resp;
}
// 200 -> Exists already -> TODO: Needs to change
Response* Resp_ExistsAlready(Response * resp) {
    resp->status = 200;
    resp->reason = "Exists Already";
    resp->payload = NULL;
    return resp;
}
// ############################################

// ############# HTTP METHODS ##################
// method == GET
void GetMethod(Request *request, Response *response) {
    Resp_Ok(response);

    if (strcmp(request->uri, "static/foo") == 0)
        response->payload = "Foo";

    else if (strcmp(request->uri, "static/bar") == 0)
        response->payload = "Bar";

    else if (strcmp(request->uri, "static/baz") == 0)
        response->payload = "Baz";

    else
        Resp_NotFound(response);

}
// method == PUT
void PutMethod(Request *request, Response *response, Resource *head) {
    if (strncmp(request->uri, "dynamic/", 8) == 0) {

        if (findResource(head, request->uri, response->payload) == true) {
            // exists already
            Resp_Ok(response);
        } else {
            // doesn't exist yet
            addResource(head, request->uri, response->payload);

            Resp_Created(response);
        }
    } else {
        // forbidden 403
        Resp_Forbidden(response);
    }
}
// method == DELETE
void DeleteMethod(Request *request, Response *response, Resource *head) {
    if (strncmp(request->uri, "dynamic/", 8) == 0) {

        if (findResource(head, request->uri, response->payload) == true) {
            // exists
            deleteResource(head, request->uri, response->payload);
            Resp_Ok(response);
        } else {
            // doesn't exist
            Resp_NotFound(response);
        }
    }
}
// ############################################

struct Response *Response_Handler(Request *request, Resource *headResource) {

    struct Response *response = malloc(sizeof(struct Response));

    response->version = request->version;

    // incorrect request
    if (request->flags == ERROR)
        Resp_BadRequest(response);

    else if (request->method == GET)
        GetMethod(request, response);

    else if (request->method == PUT)
        PutMethod (request, response, headResource);

    else if (request->method == DELETE)
        DeleteMethod(request, response, headResource);
    else
        Resp_OtherRequest(response);


    return response;
}

// sigaction handling
void sigchld_handler(int s) {
    int saved_errno = errno;
    while(waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}

// get sockaddr_in, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char** argv) {

    int listen_fd, conn_fd; // listen on listen_fd, new connection on conn_fd
    struct addrinfo hints; // used for initialising addresses
    struct addrinfo *servinfo; // servinfo will point to the results
    struct addrinfo *p; // loop elements

    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa; // sigaction
    int yes = 1;
    char s[INET6_ADDRSTRLEN];
    int rv; // status
    pid_t pid;


    // initialize addresses
    memset(&hints, 0, sizeof hints); // make sure the struct is empty
    hints.ai_family = AF_UNSPEC; // don't care IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    hints.ai_flags = AI_PASSIVE; // fill in my IP for me

    if ((rv = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
        // error-checking
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all results and bind first possible
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((listen_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(listen_fd, p->ai_addr, p->ai_addrlen) == -1) {     // binding
            close(listen_fd);
            perror("server: bind");
            continue;
        }

        break;
    }

    // free list
    freeaddrinfo(servinfo);

    // error-checking
    if (p == NULL) {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    // listen on socket
    if (listen(listen_fd, BACKLOG) == -1) {  // listening
        perror("listen");
        exit(1);
    }

    // sigaction handling
    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;   // restart system on signal return

    // error-checking
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("server: waiting for connections on port %s ...\n", argv[1]);

    // initialize linked list for resources
    Resource *headResource = initializeResource();

    // main accept loop
    for (;;) {
        sin_size = sizeof their_addr;

        // accepting connections
        conn_fd = accept(listen_fd, (struct sockaddr *)&their_addr, &sin_size);

        // error-check connected socket
        if (conn_fd == -1) {
            perror("accept");
            continue;
        }

        // presentation format address to network format address (string -> addr)
        inet_ntop(their_addr.ss_family, get_in_addr((SA *)&their_addr), s, sizeof s);

        printf("server: got connection from %s\n", s);

        // fork() for multithreading
        if (!fork()) {

            // close listening socket
            close(listen_fd);

            // malloc receive buffer
            char *recvbuff = (char *) malloc(BUFFER_SIZE * sizeof(char));
            // read from connected socket -> write to recvbuff
            long n = recv(conn_fd, recvbuff, BUFFER_SIZE, 0);

            // request not empty
            if (n != 0) {
                // next pointer after request
                char *nextPtr;

                // deserialize request
                Request *request = deserializeRequest(recvbuff, &nextPtr);

                // handle requests with corresponding responses
                Response *response = Response_Handler(request, headResource);

                // serialize response
                char *str = serializeResponse(response);

                // send response
                if (send(conn_fd, str, strlen(str), 0) == -1)
                    perror("send");
            }

            // free receive buffer
            free(recvbuff);

            exit(EXIT_SUCCESS);
        }
        // close connected socket
        close(conn_fd);
    }

    freeRescource(headResource);
    return (EXIT_SUCCESS);
}