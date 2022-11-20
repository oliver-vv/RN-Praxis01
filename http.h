#ifndef WEBSERVER
#define WEBSERVER

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define MAX_HTTP_BUFFER_SIZE 4084
#define MAX_HTTP_HEADER_COUNT 100
#define SEPARATOR "\r\n"
#define SEPARATOR2 "\r\n\r\n"

enum FLAGS {
    SUCCESS=0,
    ERROR=1,
};

enum HTTP_METHOD {
    GET,
    PUT,
    DELETE,
};



typedef struct _Header {
    char* key;
    char* val;
} _Header;


typedef struct Request {
    enum HTTP_METHOD method;
    char* uri;
    char* version;
    int header_count;
    _Header* headers; // NULLable
    char* payload; // NULLable
    enum FLAGS flags;
} Request;

typedef struct Response {
    char* version;
    int status;
    char* reason; // NULLable
    int header_count;
    _Header* headers; // NULLable
    char* payload; // NULLable
    enum FLAGS flags;
} Response;


char*		serializeRequest	(Request* req);
char*		serializeResponse	(Response* resp);
Response*	deserializeResponse	(char* payload, char** nxtPacketPtr);
Request*	deserializeRequest	(char* payload, char** nxtPacketPtr);
void 		freeResponse		(Response* resp);
void 		freeRequest		(Request* req);
// TODO implement
int 		findHeader		(_Header* arr, _Header* h);
int 		setHeader 		(_Header* arr, _Header* h);
int 		addHeader 		(_Header* arr, _Header* h);
int 		removeHeader 		(_Header* arr, int idx);

#endif
