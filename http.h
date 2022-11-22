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
int 		findHeaderByKey		(Header* arr, int size, Header* h);
int 		findHeaderByVal		(Header* arr, int size, Header* h);
int 		setHeader 		(Header** arr, int size, Header h);
int 		removeHeader 		(Header** arr, int size, int idx);

#endif

