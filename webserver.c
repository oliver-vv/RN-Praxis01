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

#define MAXLINE 4096
#define SA struct sockaddr
#define BACKLOG 10
#define BUFFER_SIZE 512

enum HTTP_Methods {
    GET,
    POST,
    PUT,
    HEAD,
    PATCH,
    DELETE,
    CONNECT,
    OPTIONS,
    TRACE
};

struct HTTP_Req {
    int method;
    char *uri;
    float version;
    char *body;
    char *header;
    bool valid;
};

struct HTTP_Resp {
    int status;
    char *body;
    float version;
    char *reason;
};

typedef struct Resource {
    char *path;
    char *data;
    struct Resource * next;
} Resource;

Resource *initializeResource() {
    Resource * head = NULL;
    head = (Resource *) malloc(sizeof(Resource));

    return head;
}

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

void freeRescource(Resource *head) {

    Resource * tmp;

    while (head != NULL)
    {
        tmp = head;
        head = head->next;
        free(tmp);
    }

}

int choose_method(char *method) {
    if (strcmp(method, "GET") == 0)
        return GET;
    if (strcmp(method, "POST") == 0)
        return POST;
    if (strcmp(method, "PUT") == 0)
        return PUT;
    if (strcmp(method, "HEAD") == 0)
        return HEAD;
    if (strcmp(method, "PATCH") == 0)
        return PATCH;
    if (strcmp(method, "DELETE") == 0)
        return DELETE;
    if (strcmp(method, "CONNECT") == 0)
        return CONNECT;
    if (strcmp(method, "OPTIONS") == 0)
        return OPTIONS;
    if (strcmp(method, "TRACE") == 0)
        return TRACE;
    return -1;
}

struct HTTP_Req* HTTP_Request_Handler(char *request_str) {

    struct HTTP_Req *request = malloc(sizeof(struct HTTP_Req));

    // find body trough \n \n -> replace second \n with & symbol
    for (int i = 0; i < strlen(request_str) - 2; i++) {
        if (request_str[i] == '\n' && request_str[i + 1] == '\n') {
            request_str[i + 1] = '|';
        }
    }

    // GET / HTTP/1.1 \r\n
    // HEADER \r\n
    // |
    // BODY

    // split request_str into separate variables
    char *request_ln = strtok(request_str, "\n");

    char *header_str = strtok(NULL, "|");
    request->header = header_str;

    char *body_str = strtok(NULL, "|");
    request->body = body_str;

    // select correct method
    char *method_str = strtok(request_ln, " ");
    request->method = choose_method(method_str);

    char *uri_str = strtok(NULL, " ");
    request->uri = uri_str;

    // extract version
    char *version_str = strtok(NULL, " ");
    version_str = strtok(version_str, "/");
    version_str = strtok(NULL, "/");

    // convert version from *char to float
    request->version = (float) atof(version_str);


    // check validity
    // TODO: Check Content-Lenght Header
    if (choose_method(method_str) == -1 || version_str == NULL || uri_str == NULL) {
        request->valid = false;
    } else {
        request->valid = true;
    }

    if (strstr(request->header, "content-length") != NULL) {
        // header contains content-length
        if (body_str == NULL)
            request->valid = false;
    } else {
        if (body_str != NULL)
            request->valid = false;
    }

    return request;
}

struct HTTP_Resp *HTTP_Response_Handler(struct HTTP_Req *request, int conn_fd, Resource *headResource) {

    struct HTTP_Resp *response = malloc(sizeof(struct HTTP_Resp));


    bool hasPayload = false;

    // check if header includes content-length
    if (strstr(request->header, "content-length") != NULL) {
        // header contains content-length
        // send payload
        hasPayload = true;
    }

    response->version = request->version;

    // check if valid request
    if (request->valid == false) {
        // 400 incorrect
        response->status = 400;
        response->reason = "Bad Request";
        response->body = NULL;
    }
    // response handling
    else if (request->method == GET) {

        response->reason = "Ok";
        response->status = 200;

        if (strcmp(request->uri, "static/foo") == 0)
            response->body = "Foo";
        else if (strcmp(request->uri, "static/bar") == 0)
            response->body = "Bar";
        else if (strcmp(request->uri, "static/baz") == 0)
            response->body = "Baz";
        else {
            // 404
            response->body = NULL;
            response->status = 404;
            response->reason = "Not found";
        }

    }

    else if (request->method == PUT) {
        if (strncmp(request->uri, "dynamic/", 8) == 0) {

            if (findResource(headResource, request->uri, request->body) == true) {
                // exists already
                response->status = 200;
                response->reason = "Exists already";
                response->body = NULL;
            } else {
                // doesn't exist yet
                addResource(headResource, request->uri, request->body);

                response->status = 201;
                response->reason = "Created";
                response->body = NULL;
            }
        } else {
            // forbidden 403
            response->status = 403;
            response->reason = "Forbidden";
            response->body = NULL;
        }

    }

    else if (request->method == DELETE) {
        if (strncmp(request->uri, "dynamic/", 8) == 0) {

            if (findResource(headResource, request->uri, request->body) == true) {
                // exists
                deleteResource(headResource, request->uri, request->body);
                response->status = 200;
                response->reason = "Ok";
                response->body = NULL;
            } else {
                // doesn't exist
                response->body = NULL;
                response->status = 404;
                response->reason = "Not found";
            }

        }
    }

    else {
        // 501 everything else
        response->version = request->version;
        response->status = 501;
        response->reason = "Other Request";
    }


    // send response
    char buffer[512];
    if (response->body != NULL) // with body
        snprintf(buffer, sizeof(buffer), "HTTP/%f %d %s\n\n%s", response->version, response->status, response->reason, response->body);
    else    //without body
        snprintf(buffer, sizeof(buffer), "HTTP/%f %d %s", response->version, response->status, response->reason);

    int len, bytes_sent;
    len = strlen(buffer);

    if (send(conn_fd, buffer, len, 0) == -1)
        perror("send");



    return response;
}

// get sockaddr_in, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// https://www.ibm.com/docs/en/zos/2.1.0?topic=functions-sigaction-examine-change-signal-action
void sigchld_handler(int s) {
    int saved_errno = errno;
    while(waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}

void sigact(struct sigaction *sa) {  // sigaction
    (*sa).sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&(*sa).sa_mask);
    (*sa).sa_flags = SA_RESTART;   // restart system on signal return

    // error-checking
    if (sigaction(SIGCHLD, sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
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

    uint8_t buff[MAXLINE + 1];  // output buffer

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

    for (p = servinfo; p != NULL; p = p->ai_next) {     // loop through all results and bind first possible
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

    freeaddrinfo(servinfo);  // free the linked-list

    if (p == NULL) {    // error-checking
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(listen_fd, BACKLOG) == -1) {  // listening
        perror("listen");
        exit(1);
    }

    sigact(&sa);

    printf("server: waiting for connections on port %s ...\n", argv[1]);

    // initialize linked list
    Resource *headResource = initializeResource();

    for (;;) {  // main accept loop
        sin_size = sizeof their_addr;

        // accepting
        conn_fd = accept(listen_fd, (struct sockaddr *)&their_addr, &sin_size);

        if (conn_fd == -1) {    // error-checking
            perror("accept");
            continue;
        }

        // presentation format address to network format address (string -> addr)
        inet_ntop(their_addr.ss_family, get_in_addr((SA *)&their_addr), s, sizeof s);

        printf("server: got connection from %s\n", s);

        int n;
        //char recvbuff[512];
        char *recvbuff = (char*)malloc(BUFFER_SIZE * sizeof(char));
        n = recv(conn_fd, recvbuff, BUFFER_SIZE, 0);

        struct HTTP_Req *request = HTTP_Request_Handler(recvbuff);

        struct HTTP_Resp *response = HTTP_Response_Handler(request, conn_fd, headResource);

        // save response in buffer
        //snprintf((char*)buff, sizeof(buff), "HTTP/1.0 200 OK\r\n\r\nReply");
        // send response
        //send(conn_fd, (char*)buff, strlen((char*)buff), 0);

        //free(request);
        //free(response);
        //free(recvbuff);

        close(conn_fd);
    }

    freeRescource(headResource);
}