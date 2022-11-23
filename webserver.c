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
#define MAX_RETIRES 10

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

char *getResource(Resource *head, char *path) {
    Resource *current = head;

    while (current != NULL) {
        if(current->path != NULL || current->data != NULL)
            if (strcmp(current->path, path) == 0) {
                return current->data;
            }
        current = current->next;
    }

    return NULL;
}

// find a resource in linked list
char *findResource(Resource *head, char *path) {
	Resource *current = head;

	while (current != NULL) {
		if(current->path != NULL || current->data != NULL)
			if (strcmp(current->path, path) == 0) {   // TODO: check for data ? -> strcmp(current->data, data) == 0
                return current->data;
            }
		current = current->next;
	}

	return NULL;
}
// add a resource in linked list
void addResource(Resource *head, char *path, char *data) {

	Resource * current = head;

	if (current->data == NULL || current->path == NULL || current == NULL) {
		head->path = path;
		head->data = data;
		head->next = NULL;
		return;
	}

	while (current->next != NULL) {
		current = current->next;
	}

	/* now we can add a new variable */
	current->next = (Resource *) malloc(sizeof(Resource));
	current->next->path = path;
	current->next->data = data;
	current->next->next = NULL;
}
// delete a resource by value from a linked list
Resource *deleteResource(Resource *head, char *path, char *data) {
	Resource *current = head;
	Resource *prev = NULL;

	if (head == NULL)
		return NULL;

	// TODO: currently only deleting when path matches -> data needs to be included
	while (strcmp(current->path, path) != 0 && strcmp(current->data, data) != 0) {

		if (current->next == NULL) {
			return NULL;
		} else {
			prev = current;
			current = current->next;
		}
	}

	if (current == head) {
		head->path = NULL;
		head->data = NULL;
		head->next = NULL;
	} else {
		prev->next = current->next;
		free(current);
	}

	//free(current); ?

	return head;
}
// free resource linked list
void freeResource(Resource *head) {

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
// 201 -> Overwritten
Response* Resp_Overwritten(Response * resp) {
    resp->status = 201;
    resp->reason = "Overwritten";
    resp->payload = NULL;
    return resp;
}
// ############################################

// ############# HTTP METHODS ##################
// method == GET
void GetMethod(Request *request, Response *response, Resource *head) {
	Resp_Ok(response);
	char* res;
	if (strcmp(request->uri, "/static/foo") == 0)
		response->payload = "Foo";

	else if (strcmp(request->uri, "/static/bar") == 0)
		response->payload = "Bar";

	else if (strcmp(request->uri, "/static/baz") == 0)
		response->payload = "Baz";

	else if ((res = getResource(head, request->uri)) != NULL) {
		response->payload = res;
	}

	else
		Resp_NotFound(response);

}
// method == PUT
void PutMethod(Request *request, Response *response, Resource *head) {
	if (strncmp(request->uri, "/dynamic/", 8) == 0) {
        char *res;
		if ((res = findResource(head, request->uri)) != NULL) {
			// exists already

            if (strcmp(request->payload, res) == 0) {
                // if data is the same respond -> ok
                Resp_Ok(response);
            } else {
                // if data is new -> overwrite
                deleteResource(head, request->uri, res);
                addResource(head, request->uri, request->payload);

                Resp_Overwritten(response);
            }

		} else {
			// doesn't exist yet
			addResource(head, request->uri, request->payload);

			Resp_Created(response);
		}
	} else {
		// forbidden 403
		Resp_Forbidden(response);
	}
}
// method == DELETE
void DeleteMethod(Request *request, Response *response, Resource *head) {
	if (strncmp(request->uri, "/dynamic/", 8) == 0) {
        char *res;
		if ((res = findResource(head, request->uri)) != NULL) {
			// exists
			deleteResource(head, request->uri, request->payload);
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

	if (request->version != NULL)
		response->version = request->version;
	else
		response->version = "HTTP/1.1";


	// incorrect request
	if (request->flags == ERROR)
		Resp_BadRequest(response);

	else if (request->method == GET)
		GetMethod(request, response, headResource);

	else if (request->method == PUT)
		PutMethod (request, response, headResource);

	else if (request->method == DELETE)
		DeleteMethod(request, response, headResource);
	else if (request->method == NONE)
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

void processBuffer(char *buffer, size_t *len, int conn_fd, Resource *headResource) {

	size_t initial_len = *len;
	char* tmp;
	char* nextAddr;
	Request *request;

	while (true) {

		// try to read and parse a complete request package
		//printf("request: \n%s\n", buffer);
		request = deserializeRequest(buffer, &nextAddr);
		// if not complete -> return
		if (request == NULL)
			break;

		// handle request with corresponding response
		Response *response = Response_Handler(request, headResource);
		setHeader(&response->headers, &response->header_count, (Header) {"Connection", "Keep-Alive"}); 
		setHeader(&response->headers, &response->header_count, (Header) {"Keep-Alive", "timeout=5, max=6"}); 
		char* s = calloc(25, sizeof(char));
		if (response->payload)
			sprintf(s, "%lu", strlen(response->payload));
		else 
			sprintf(s, "%d", 0);
		setHeader(&response->headers, &response->header_count, (Header) {"Content-Length", s}); 
		// serialize response
		char *str = serializeResponse(response);


		// send response
		int n = send(conn_fd, str, strlen(str), 0);
		if (n == -1)
			perror("send");


		printf("%s\n", str);

		*len -= (nextAddr - buffer);

		if (*len > 0) {
			memset(buffer, '\0', (buffer - nextAddr));
			memmove(buffer, nextAddr, *len);
			memset(nextAddr, '\0', *len);
		}
		else
			return;
	}

}

int main(int argc, char** argv) {

	int listen_fd, conn_fd;
	struct addrinfo hints;
	struct addrinfo *servinfo;
	struct addrinfo *p; 

	struct sockaddr_storage their_addr;
	socklen_t sin_size;
	struct sigaction sa; 
	int yes = 1;
	char s[INET6_ADDRSTRLEN];
	int rv; 

	// initialize addresses
	memset(&hints, 0, sizeof hints); 
	hints.ai_family = AF_UNSPEC; 
	hints.ai_socktype = SOCK_STREAM; 
	hints.ai_flags = AI_PASSIVE; 

	if ((rv = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
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
	Resource *headResource = (Resource *) malloc(sizeof(Resource));
	char *recv_buff = (char *) malloc(BUFFER_SIZE * sizeof(char));

	// main accept loop
	while (1) {
		// listen on socket
		if (listen(listen_fd, BACKLOG) == -1) {  // listening
			perror("listen");
			exit(1);
		}
		sin_size = sizeof their_addr;

		// accepting connections
		printf("waiting for connection\n");
		conn_fd = accept(listen_fd, (struct sockaddr *)&their_addr, &sin_size);

		// error-check connected socket
		if (conn_fd == -1) {
			perror("accept");
			continue;
		}

		// presentation format address to network format address (string -> addr)
		inet_ntop(their_addr.ss_family, get_in_addr((SA *)&their_addr), s, sizeof s);

		printf("server: got connection from %s\n", s);

		size_t recv_len = 0;
		long recv_result;
		int retries = 0;
		while (1){
			recv_result = recv(conn_fd, recv_buff + recv_len, BUFFER_SIZE - recv_len, 0);
			if (recv_result < 0) {
				//printf("received error: %s\n", strerror(recv_result));
			}
			else if (recv_result == 0) {
				if (++retries >= MAX_RETIRES)
					break;
				continue;
			}
			recv_len += recv_result;
			//printf("received %lu bytes, %lu bytes in total\n", recv_result, recv_len);
			processBuffer(recv_buff, &recv_len, conn_fd, headResource);
			//break;
		}

		// close connected socket
		close(conn_fd);
	}

	free (headResource);
	freeResource(headResource);
	return (EXIT_SUCCESS);
}
