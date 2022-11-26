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

// http.h begin
#ifndef WEBSERVER
#define WEBSERVER

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#define MAX_HTTP_BUFFER_SIZE 4096
#define MAX_HTTP_HEADER_COUNT 100
#define SEPARATOR "\r\n"
#define SEPARATOR2 "\r\n\r\n"

enum FLAGS {
	SUCCESS=0,
	ERROR=1,
};

enum HTTP_METHOD {
	NONE	   ,
	GET	   ,
	PUT	   ,
	DELETE	   ,
};

enum HTTP_STATUS {
	OK 			= 200,
	CREATED 		= 201,
	ACCEPTED 		= 202,
	NO_CONTENT 		= 204,
	MOVED_PERMANENTLY 	= 301,
	BAD_REQUEST 		= 400,
	FORBIDDEN 		= 403,
	NOT_FOUND 		= 404,
	INTERNAL_SERVER_ERROR 	= 500,
};


typedef struct Header {
	char* key;
	char* val;
} Header;


typedef struct Request {
	enum HTTP_METHOD method;
	char* uri;
	char* version;
	int header_count;
	Header* headers; 	// NULLable
	char* payload; 		// NULLable
	enum FLAGS flags;
} Request;

typedef struct Response {
	char* version;
	int status;
	char* reason; 		// NULLable
	int header_count;
	Header* headers; 	// NULLable
	char* payload; 		// NULLable
	enum FLAGS flags;
} Response;

// TODO documentation
// high level
Response*	deserializeResponse	(char* payload, char** nxtPacketPtr);
Request*	deserializeRequest	(char* payload, char** nxtPacketPtr);
char*		serializeRequest	(Request* req);
char*		serializeResponse	(Response* resp);
void 		freeResponse		(Response* resp);
void 		freeRequest		(Request* req);
int 		findHeaderByKey		(Header* arr, int size, Header* h);
int 		findHeaderByVal		(Header* arr, int size, Header* h);
int 		setHeader 		(Header** arr, int* size, Header h);
// compare
bool 		cmpRequest		(Request* a, Request* b);
bool 		cmpResponse		(Response* a, Response* b);
bool 		cmpHeader 		(Header* a, Header* b);
// deep copies
Header* 	copyHeader 		(const Header* h);
Request*	copyRequest		(const Request* r);
Response*	copyResponse		(const Response* r);
// utility
enum HTTP_METHOD method2enum		(char* method);
char* 		enum2method 		(enum HTTP_METHOD num);

// TODO test
int 		removeHeader 		(Header** arr, int* size, int idx);

#endif
// http.h end

// http.c begin
enum HTTP_METHOD method2enum(char* method) {
	if (strcasecmp(method, "GET")==0)
		return GET;
	if (strcasecmp(method, "PUT")==0)
		return PUT;
	if (strcasecmp(method, "DELETE")==0)
		return DELETE;
	return NONE;
}

char* enum2method(enum HTTP_METHOD num) {
	switch(num) {
		case GET:
			return "GET";
		case PUT:
			return "PUT";
		case DELETE:
			return "DELETE";
		default:
			return "NONE";
	}
}


char* serializeRequest(Request* req) {
	char* buffer = calloc(MAX_HTTP_BUFFER_SIZE, sizeof(char));
	int size = 0;
	size += snprintf(buffer+size, MAX_HTTP_BUFFER_SIZE, "%s %s %s%s", enum2method(req->method), req->uri, req->version, SEPARATOR);
	for (int i = 0; i < req->header_count; i++)
		size += snprintf(buffer+size, MAX_HTTP_BUFFER_SIZE, "%s:%s%s", req->headers[i].key, req->headers[i].val, SEPARATOR);
	size += snprintf(buffer+size, MAX_HTTP_BUFFER_SIZE, "%s", SEPARATOR);
	if (req->payload)
		size += snprintf(buffer+size, MAX_HTTP_BUFFER_SIZE, "%s", req->payload);
	buffer = realloc(buffer, size * sizeof(char));
	return buffer;
}


char* serializeResponse(Response* resp) {
	char* buffer = calloc(MAX_HTTP_BUFFER_SIZE, sizeof(char));
	int size = snprintf(buffer, MAX_HTTP_BUFFER_SIZE, "%s %d %s%s", resp->version, resp->status, resp->reason, SEPARATOR);
	for (int i = 0; i < resp->header_count; i++)
		size += snprintf(buffer+size, MAX_HTTP_BUFFER_SIZE, "%s:%s%s", resp->headers[i].key, resp->headers[i].val, SEPARATOR);
	size += snprintf(buffer+size, MAX_HTTP_BUFFER_SIZE, "%s", SEPARATOR);
	if (resp->payload)
		size += snprintf(buffer+size, MAX_HTTP_BUFFER_SIZE, "%s", resp->payload);
	buffer = realloc(buffer, size * sizeof(char));
	return buffer;
}

Response* deserializeResponse(char* payload, char** nxtPacketPtr) {
	Response* resp = calloc(1,sizeof(Response));
	char* header_end = strstr(payload, SEPARATOR2);
	if (header_end==NULL)
		return NULL;
	for (int i = 0; i < 4; i++)
		header_end[i] = '\0';

	int flag = 0;
	// version
	char* version = strtok(payload, " ");
	if (version!=NULL) {
		resp->version = malloc(strlen(version)*sizeof(char));
		strcpy(resp->version, version);
	} else {
		flag |= ERROR;
	}
	// status
	int status = atoi(strtok(NULL, " "));
	if (status!=0) {
		resp->status = status;

	} else {
		flag |= ERROR;
		return NULL;
	}
	// reason
	char* reason = strtok(NULL, SEPARATOR);
	if (reason == NULL)
		reason = "";
	resp->reason = malloc(strlen(reason)*sizeof(char));
	strcpy(resp->reason, reason);
	// headers
	char* header_token = strtok(NULL, SEPARATOR);
	char** headers_token = calloc(MAX_HTTP_HEADER_COUNT, sizeof(Header));
	int header_count = 0;
	for (int i = 0; header_token; i++) {
		headers_token[i] = header_token;
		header_token = strtok(NULL, SEPARATOR);
		header_count++;
	}
	int content_length = 0;
	Header* headers = malloc(header_count*sizeof(Header));
	for (int i = 0; i < header_count; i++) {
		char* key = strtok(headers_token[i], ":");
		if (key!=NULL) {
			headers[i].key = malloc(strlen(key)*sizeof(char));
			strcpy(headers[i].key, key);
		} else {
			flag |= ERROR;
		}

		char* value = strtok(NULL, SEPARATOR);
		if (value!=NULL) {
			headers[i].val = malloc(strlen(value)*sizeof(char));
			strcpy(headers[i].val, value);
		} else {
			flag |= ERROR;
		}

		if (key && value && strcmp(key, "Content-Length")==0) {
			int new_content_length = atoi(value);
			if (new_content_length > content_length)
				content_length = new_content_length;
		}
	}
	free(header_token);
	resp->header_count = header_count;
	if (header_count>0) {
		resp->headers = headers;
	} else {
		resp->headers = NULL;
		free(headers);
	}
	// payload
	char* pyld = header_end + 4;
	if (content_length > 0) {
		resp->payload = malloc(content_length*sizeof(char));
		strncpy(resp->payload, pyld, content_length);
	} else {
		resp->payload = NULL;
	}
	// next packet
	if (content_length >= 0)
		*nxtPacketPtr = pyld + content_length;
	else 
		*nxtPacketPtr = pyld;
	// flag
	resp->flags = flag;

	return resp;
}


Request* deserializeRequest(char* bytestream, char** nxtPacketPtr) {
	Request* req = calloc(1,sizeof(Request));
	char* header_end = strstr(bytestream, SEPARATOR2);
	if (header_end == NULL)
		return NULL;
	for (int i = 0; i < 4; i++)
		header_end[i] = '\0';

	int flag = 0;
	// method
	char* method = strtok(bytestream, " ");
	if (method!=NULL) {
		req->method = method2enum(method);
	} else {
		flag |= ERROR;
	}
	// uri
	char* uri = strtok(NULL, " ");
	if (uri!=NULL) {
		req->uri = malloc(strlen(uri)*sizeof(char));
		strcpy(req->uri, uri);
	} else {
		flag |= ERROR;
	}
	// version
	char* version = strtok(NULL, SEPARATOR);
	if (version!=NULL) {
		req->version = malloc(strlen(version)*sizeof(char));
		strcpy(req->version, version);
	} else {
		flag |= ERROR;
	}
	// headers
	char* header_token = strtok(NULL, SEPARATOR);
	char** headers_token = calloc(MAX_HTTP_HEADER_COUNT, sizeof(Header));
	int header_count = 0;
	for (int i = 0; header_token; i++) {
		headers_token[i] = header_token;
		header_token = strtok(NULL, SEPARATOR);
		header_count++;
	}
	int content_length = 0;
	Header* headers = malloc(header_count*sizeof(Header));
	for (int i = 0; i < header_count; i++) {
		char* key = strtok(headers_token[i], ":");
		if (key!=NULL) {
			headers[i].key = malloc(strlen(key)*sizeof(char));
			strcpy(headers[i].key, key);
		} else {
			flag |= ERROR;
		}

		char* value = strtok(NULL, SEPARATOR);
		if (value!=NULL) {
			headers[i].val = malloc(strlen(value)*sizeof(char));
			strcpy(headers[i].val, value);
		} else {
			flag |= ERROR;
		}

		if (key && value && strcmp(key, "Content-Length")==0) {
			int new_content_length = atoi(value);
			if (new_content_length > content_length)
				content_length = new_content_length;
		}
	}
	free(header_token);
	req->header_count = header_count;
	if (header_count>0) {
		req->headers = headers;
	} else {
		req->headers = NULL;
		free(headers);
	}
	// payload 
	char* pyld = header_end + 4;
	if (content_length > 0) {
		req->payload = malloc(content_length*sizeof(char));
		strncpy(req->payload, pyld, content_length);
	} else {
		req->payload = NULL;
	}
	// next packet
	if (content_length >= 0)
		*nxtPacketPtr = pyld + content_length;
	else 
		*nxtPacketPtr = pyld;
	// flag
	req->flags = flag;

	return req;
}


void freeRequest(Request* req) {
	if (req->uri)
		free(req->uri);
	if (req->version)
		free(req->version);
	for (int i = 0; i < req->header_count;i++){
		free(req->headers[i].key);
		free(req->headers[i].val);
	}
	if (req->headers)
		free(req->headers);
	if (req->payload)
		free(req->payload);
	free(req);
}

void freeResponse(Response* resp){
	if (resp->version)
		free(resp->version);
	if (resp->reason)
		free(resp->reason);
	for (int i = 0; i < resp->header_count;i++){
		free(resp->headers[i].key);
		free(resp->headers[i].val);
	}
	if (resp->headers)
		free(resp->headers);
	if (resp->payload)
		free(resp->payload);
	free(resp);
}

bool cmpHeader(Header* a, Header* b) {
	return (strcmp(a->key, b->key)==0)&&(strcmp(a->val, b->val)==0);
}

bool cmpRequest(Request* a, Request* b) {
	if (a->header_count != b->header_count)
		return false;
	int headers = 0;
	for (int i = 0; i < a->header_count; i++)
		headers += cmpHeader(&a->headers[i], &b->headers[i]);
	return headers==a->header_count \
		&& a->flags==b->flags \
		&& a->method == b->method \
		&& strcasecmp(a->version, b->version)==0 \
		&& strcmp(a->uri, b->uri)==0 \
		&& strcmp(a->payload, b->payload)==0;
}

bool cmpResponse(Response* a, Response* b) {
	if (a->header_count != b->header_count)
		return false;
	int headers = 0;
	for (int i = 0; i < a->header_count; i++)
		headers += cmpHeader(&a->headers[i], &b->headers[i]);
	return headers==a->header_count \
		&& a->flags==b->flags \
		&& a->status == b->status \
		&& strcasecmp(a->version, b->version)==0 \
		&& strcasecmp(a->reason, b->reason)==0 \
		&& strcmp(a->payload, b->payload)==0;
}

Header* copyHeader(const Header* h) {
	Header* new = malloc(sizeof(Header));
	if (h->key) {
		new->key = malloc(strlen(h->key)*sizeof(char));
		strcpy(new->key, h->key);
	}
	if (h->val) {
		new->val = malloc(strlen(h->val)*sizeof(char));
		strcpy(new->val, h->val);
	}
	return new;
}

Request* copyRequest(const Request* r) {
	Request* new = malloc(sizeof(Request));
	new->method = r->method;
	if (r->uri) {
		new->uri = malloc(strlen(r->uri)*sizeof(char));
		strcpy(new->uri, r->uri);
	}
	if (r->version) {
		new->version = malloc(strlen(r->version)*sizeof(char));
		strcpy(new->version, r->version);
	}
	new->header_count = r->header_count;
	if (r->headers) {
		new->headers = malloc(new->header_count*sizeof(Header));
		for (int i = 0; i < new->header_count; i++)
			new->headers[i] = *copyHeader(&r->headers[i]);
	}
	if (r->payload) {
		new->payload = malloc(strlen(r->payload)*sizeof(char));
		strcpy(new->payload, r->payload);
	}
	new->flags = r->flags;
	return new;
}

Response* copyResponse(const Response* r) {
	Response* new = malloc(sizeof(Response));
	if (r->version) {
		new->version = malloc(strlen(r->version)*sizeof(char));
		strcpy(new->version, r->version);
	}
	new->status = r->status;
	if (r->reason) {
		new->reason = malloc(strlen(r->reason)*sizeof(char));
		strcpy(new->reason, r->reason);
	}
	new->header_count = r->header_count;
	if (r->headers) {
		new->headers = malloc(new->header_count*sizeof(Header));
		for (int i = 0; i < new->header_count; i++)
			new->headers[i] = *copyHeader(&r->headers[i]);
	}
	if (r->payload) {
		new->payload = malloc(strlen(r->payload)*sizeof(char));
		strcpy(new->payload, r->payload);
	}
	new->flags = r->flags;
	return new;
}


int findHeaderByKey(Header* arr, int size, Header* h) {
	for (int i = 0; i < size; i++) {
		if (strcmp(arr[i].key, h->key)==0)
			return i;
	}
	return -1;
}

int findHeaderByVal(Header* arr, int size, Header* h) {
	for (int i = 0; i < size; i++) {
		if (strcmp(arr[i].val, h->val)==0)
			return i;
	}
	return -1;
}

int setHeader (Header** arr_ptr, int *header_count, Header h){
	int idx;
	if (*arr_ptr==NULL) {
		*arr_ptr=malloc(*header_count * sizeof(Header));
		idx = -1;
	}
	else {
		idx = findHeaderByKey(*arr_ptr, *header_count, &h);
	}
	Header* arr = *arr_ptr;
	if (idx >= 0) {
		arr[idx].val = h.val;
	} else {
		(*header_count)++;
		*arr_ptr = realloc(*arr_ptr, *header_count*sizeof(Header));
		arr = *arr_ptr;
		arr[*header_count-1] = h;
	}
	return 0;
}

int removeHeader (Header** arr_ptr, int *header_count, int idx) {
	Header* arr = *arr_ptr;
	if (idx < 0 || *header_count >= idx)
		return -1;
	for (int i = idx; i < *header_count-1; i++)
		arr[i] = arr[i+1];
	*arr_ptr = realloc(*arr_ptr, --*header_count * sizeof(Header));
	return 0;
}

// http.c end

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
