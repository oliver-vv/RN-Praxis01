#include "http.h"


const int http_method_count = 3;
const char* http_methods[] = {"GET", "PUT", "DELETE"};

char* serializeRequest(Request* req) {
    char* buffer = calloc(MAX_HTTP_BUFFER_SIZE, sizeof(char));
    int size = 0;
    size += snprintf(buffer+size, MAX_HTTP_BUFFER_SIZE, "%s %s %s%s", http_methods[req->method], req->uri, req->version, SEPARATOR);
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
    }
    // reason
    char* reason = strtok(NULL, SEPARATOR);
    if (reason == NULL)
        reason = "";
    resp->reason = malloc(strlen(reason)*sizeof(char));
    strcpy(resp->reason, reason);
    // headers
    char* header_token = strtok(NULL, SEPARATOR);
    char** headers_token = calloc(MAX_HTTP_HEADER_COUNT, sizeof(_Header));
    int header_count = 0;
    for (int i = 0; header_token; i++) {
        headers_token[i] = header_token;
        header_token = strtok(NULL, SEPARATOR);
        header_count++;
    }
    int content_length = 0;
    _Header* headers = malloc(header_count * sizeof(_Header));
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
        resp->payload = malloc(strlen(pyld)*sizeof(char));
        strcpy(resp->payload, pyld);
    } else {
        resp->payload = NULL;
    }
    // next packet
    if (content_length >= 0)
        *nxtPacketPtr = pyld + content_length;
    else
        *nxtPacketPtr = pyld;

    return resp;
}


Request* deserializeRequest(char* bytestream, char** nxtPacketPtr) {
    Request* req = calloc(1,sizeof(Request));
    char* header_end = strstr(bytestream, SEPARATOR2);
    for (int i = 0; i < 4; i++)
        header_end[i] = '\0';

    int flag = 0;
    // method
    char* method = strtok(bytestream, " ");
    if (method!=NULL) {
        for (int i = 0; i < http_method_count; i++)
            if (strcmp(http_methods[i], method)==0) {
                req->method = i;
                break;
            }
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
    char** headers_token = calloc(MAX_HTTP_HEADER_COUNT, sizeof(_Header));
    int header_count = 0;
    for (int i = 0; header_token; i++) {
        headers_token[i] = header_token;
        header_token = strtok(NULL, SEPARATOR);
        header_count++;
    }
    int content_length = 0;
    _Header* headers = malloc(header_count * sizeof(_Header));
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
        req->payload = malloc(strlen(pyld)*sizeof(char));
        strcpy(req->payload, pyld);
    } else {
        req->payload = NULL;
    }
    // next packet
    if (content_length >= 0)
        *nxtPacketPtr = pyld + content_length;
    else
        *nxtPacketPtr = pyld;

    return req;
}


void freeRequest(Request* req) {
    free(req->uri);
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
    free(resp->version);
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


