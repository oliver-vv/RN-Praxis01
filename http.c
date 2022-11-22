#include "http.h"



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
