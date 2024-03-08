//Roshini Pulle

#include "asgn2_helper_funcs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <regex.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#define BUFFER_SIZE 2048

regex_t req_regx;
regex_t head_regx;
regex_t regex;
char content_length[] = "Content-Length: ";

void initRegx() {

    if (regcomp(&req_regx,
            "^[[:space:]]*([a-zA-Z]{1,8}+)[[:space:]]+(/"
            "[a-zA-Z0-9.-]{2,64}+)[[:space:]]+([a-zA-Z]+/[0-9]\\.[0-9])[[:space:]]*$",
            REG_EXTENDED)
        != 0) {
        perror("req_regx failed");
        exit(EXIT_FAILURE);
    }

    if (regcomp(&regex, "[^\r\n]*", REG_EXTENDED) != 0) {
        perror("regex failed");
        exit(EXIT_FAILURE);
    }

    if (regcomp(&head_regx, "^[[:space:]]*([a-zA-Z0-9.-]+):[[:space:]]+([[:print:]]+)[:space:]*$",
            REG_EXTENDED)
        != 0) {
        perror("regex failed");
        exit(EXIT_FAILURE);
    }
}

void clearRegx() {
    regfree(&regex);
    regfree(&head_regx);
    regfree(&req_regx);
}

typedef struct HeaderField {
    char *key; // 1 to 128 character set [a-zA-Z0-9.-]
    char *value; // at most 128 characters, only printable ASCII
    struct HeaderField *next;
} HeaderField;

typedef struct HttpRequest {
    char *method; // at most 8 characters
    char *uri; // 2 to 64 characters
    char *version; // HTTP/#.#
    char *body;
    HeaderField *head;
    int error;
    int bodyPos;
    int bodyLen;
} HttpRequest;

HeaderField *createHeaderField(const char *key, int len1, const char *value, int len2) {
    HeaderField *hf = (HeaderField *) malloc(sizeof(HeaderField));
    if (hf == NULL) {
        perror("Memory allocation failed");
        return NULL;
    }
    hf->key = strndup(key, len1);
    hf->value = strndup(value, len2);
    printf("k:%s\n", hf->key);
    printf("v:%s\n", hf->value);
    hf->next = NULL;
    return hf;
}

void addHeaderField(HttpRequest *ds, const char *key, int len1, const char *value, int len2) {

    HeaderField *new_field = createHeaderField(key, len1, value, len2);
    if (new_field == NULL) {
        ds->error = -1;
    } else {
        new_field->next = ds->head;
        ds->head = new_field;
        if (strcmp("Content-Length", new_field->key) == 0) {
            ds->bodyLen = atoi(new_field->value);
        }
    }
}
void addMethod(HttpRequest *ds, const char *method, int length) {
    ds->method = strndup(method, length);
    printf("method:%s\n", ds->method);
}
void addUri(HttpRequest *ds, const char *uri, int length) {
    ds->uri = strndup(uri, length);
    printf("uri:%s\n", ds->uri);
}
void addVersion(HttpRequest *ds, const char *version, int length) {
    ds->version = strndup(version, length);
    printf("version:%s\n", ds->version);
}
void addBody(HttpRequest *ds, const char *body, int length) {
    ds->body = strndup(body, length);
    printf("body:%s\n", ds->body);
}
void freeHeaderField(HeaderField *hf) {
    free(hf->key);
    free(hf->value);
    free(hf);
}

HttpRequest *createHttpRequest(char buffer[]) {
    HttpRequest *ds = (HttpRequest *) malloc(sizeof(HttpRequest));
    if (ds == NULL) {
        perror("Memory allocation failed");
        return NULL;
    }
    ds->error = 0;
    ds->method = NULL;
    ds->uri = NULL;
    ds->version = NULL;
    ds->head = NULL;
    ds->body = NULL;
    ds->bodyPos = -1;
    ds->bodyLen = -1;

    printf("errof:%d", ds->error);
    //parser buffer and fill HttpRequest

    int curField = 0; //0 for request line, 1 for header field, 2 for body
    regmatch_t match;
    int offset = 0;
    while (regexec(&regex, buffer + offset, 1, &match, 0) == 0) {
        if (match.rm_so == -1) {
            break;
        }
        // Print the matched token
        if (curField == 0) {
            printf("Request line: %.*s\n", (int) (match.rm_eo - match.rm_so),
                buffer + offset + match.rm_so);
            regmatch_t pmatch[req_regx.re_nsub + 1];
            char tmp = buffer[match.rm_eo];
            buffer[match.rm_eo] = '\0';
            int result = regexec(
                &req_regx, buffer + offset + match.rm_so, req_regx.re_nsub + 1, pmatch, 0);
            buffer[match.rm_eo] = tmp;

            if (result != 0) { //returns 0 upon sucess
                printf("It is not a match"); //handle invalid match
            } else {
                if (pmatch[1].rm_so >= 0) {
                    addMethod(ds, buffer + offset + pmatch[1].rm_so,
                        (int) (pmatch[1].rm_eo - pmatch[1].rm_so));
                }
                if (pmatch[2].rm_so >= 0) {
                    addUri(ds, buffer + offset + pmatch[2].rm_so,
                        (int) (pmatch[2].rm_eo - pmatch[2].rm_so));
                }
                if (pmatch[3].rm_so >= 0) {
                    addVersion(ds, buffer + offset + pmatch[3].rm_so,
                        (int) (pmatch[3].rm_eo - pmatch[3].rm_so));
                }
            }
            curField = 1; //next header fields
        } else if (curField == 1) {
            if ((int) (match.rm_eo - match.rm_so) == 0) {
                curField = 2; //next body
                ds->bodyPos = offset + 2;
            } else {
                printf("add field: '%.*s'\n", (int) (match.rm_eo - match.rm_so),
                    buffer + offset + match.rm_so);

                regmatch_t pmatch1[head_regx.re_nsub + 1];
                char tmp = *(buffer + offset + (int) (match.rm_eo - match.rm_so));

                *(buffer + offset + (int) (match.rm_eo - match.rm_so)) = '\0';

                int result = regexec(
                    &head_regx, buffer + offset + match.rm_so, head_regx.re_nsub + 1, pmatch1, 0);
                *(buffer + offset + (int) (match.rm_eo - match.rm_so)) = tmp;
                if (result != 0) { //returns 0 upon sucess
                    printf("It is not a match\n"); //handle invalid match
                } else {
                    if (pmatch1[1].rm_so >= 0) {
                        //printf("add header: %.*s\n", (int)(pmatch1[1].rm_eo - pmatch1[1].rm_so), buffer + offset + pmatch1[1].rm_so);
                        addHeaderField(ds, buffer + offset + pmatch1[1].rm_so,
                            (int) (pmatch1[1].rm_eo - pmatch1[1].rm_so),
                            buffer + offset + pmatch1[2].rm_so,
                            (int) (pmatch1[2].rm_eo - pmatch1[2].rm_so));
                    }
                }
            }
        } else if (curField == 2) {
            if ((int) (match.rm_eo - match.rm_so) == 0) {
                break;
            } else {
                printf("add body: %.*s\n", (int) (match.rm_eo - match.rm_so),
                    buffer + offset + match.rm_so);
                ds->bodyPos = offset + match.rm_so;
                return ds;
            }
        } else {
            printf("Check field: %.*s\n", (int) (match.rm_eo - match.rm_so),
                buffer + offset + match.rm_so);
        }
        // Move the offset to the end of the matched token
        offset += match.rm_eo + 2;
    }
    return ds;
}

void sendHTTPResponseMessage(int socket, char *message, int code) {
    char tmpBuf[80];
    sprintf(tmpBuf, "HTTP/1.1 %d %s\r\n%s%lu\r\n\r\n%s\n", code, message, content_length,
        strlen(message) + 1, message);
    write_n_bytes(socket, tmpBuf, strlen(tmpBuf));
}

void processRequest(HttpRequest *ds, int socket) {
    char tmbBuf[11];
    if (ds->error == -1) {
        sendHTTPResponseMessage(socket, "Internal Server Error", 500);
        return;
    } else if (ds->method == NULL) {
        sendHTTPResponseMessage(socket, "Bad Request", 400);
        return;
    } else if (!(strcmp(ds->method, "PUT") == 0 || strcmp(ds->method, "GET") == 0)) {
        sendHTTPResponseMessage(socket, "Not Implemented", 501);
        return;
    } else if (ds->version == NULL || !(strcmp(ds->version, "HTTP/1.1") == 0)) {
        sendHTTPResponseMessage(socket, "Version Not Supported", 505);
        return;
    } else if (strcmp(ds->method, "PUT") == 0) {
        if (access(ds->uri + 1, W_OK) != -1) {
            int fd = open(ds->uri + 1, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd != -1) {
                sendHTTPResponseMessage(socket, "OK", 200);
                if (ds->body != NULL) {
                    write(fd, ds->body, ds->bodyLen);
                }
                close(fd);
            } else {
                sendHTTPResponseMessage(socket, "Forbidden", 403);
            }
            return;
        } else {
            int fd = open(ds->uri + 1, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd != -1) {
                sendHTTPResponseMessage(socket, "Created", 201);
                if (ds->body != NULL) {
                    write(fd, ds->body, ds->bodyLen);
                }
                close(fd);
            } else {
                sendHTTPResponseMessage(socket, "Forbidden", 403);
            }
            return;
        }
    } else if (strcmp(ds->method, "GET") == 0) {
        struct stat file_stat;
        //check if file exists
        if (access(ds->uri + 1, F_OK) != -1) {
            if (stat(ds->uri + 1, &file_stat) == 0) {
                if (access(ds->uri + 1, R_OK) != -1 && S_ISREG(file_stat.st_mode)) {
                    int fd = open(ds->uri + 1, O_RDONLY);
                    struct stat st;
                    if (stat(ds->uri + 1, &st) == 0) {
                        write_n_bytes(socket, "HTTP/1.1 200 OK\r\n", strlen("HTTP/1.1 200 OK\r\n"));
                        write_n_bytes(socket, content_length, strlen(content_length));
                        sprintf(tmbBuf, "%lu", st.st_size);
                        write_n_bytes(socket, tmbBuf, strlen(tmbBuf));
                        write_n_bytes(socket, "\r\n\r\n", strlen("\r\n\r\n"));
                        pass_n_bytes(fd, socket, st.st_size);
                    }
                    close(fd);
                    return;
                } else {
                    sendHTTPResponseMessage(socket, "Forbidden", 403);
                    return;
                }
            }
        } else {
            sendHTTPResponseMessage(socket, "Not Found", 404);
            return;
        }
    } else {
        //check why we are here? we are not supposed to reach this place
        sendHTTPResponseMessage(socket, "Internal Server Error", 500);
    }
}

void freeHttpRequest(HttpRequest *ds) {
    HeaderField *current = ds->head;
    HeaderField *next;
    while (current != NULL) {
        next = current->next;
        freeHeaderField(current);
        current = next;
    }
    free(ds->method);
    free(ds->uri);
    free(ds->version);
    free(ds->body);
    free(ds);
}

void handleHttpRequest(int new_socket) {
    char buffer[BUFFER_SIZE] = { 0 };
    int valread = read_until(new_socket, buffer, BUFFER_SIZE, "\r\n\r\n");

    if (valread == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        fprintf(stderr, "read timeout"); //as per notes given in PDF
    } else if (valread == -1) {
        printf("Client closed connection\n");
    } else {
        printf("received:%d\n", valread);
        buffer[valread] = '\0'; //null terminate buffer for safety
        HttpRequest *ds = createHttpRequest(buffer);

        if (ds == NULL) {
            //sendInternalServerError(new_socket);
            sendHTTPResponseMessage(new_socket, "Internal Server Error", 500);
        } else {
            printf("bi:%d\n", ds->bodyPos);
            printf("bl:%d\n", ds->bodyLen);
            printf("br:%s\n", buffer + ds->bodyPos);
            int receivedBodyLen = valread - ds->bodyPos;
            int pendingBodyLen = ds->bodyLen - receivedBodyLen;
            printf("rb:%d\n", receivedBodyLen);
            printf("pb:%d\n", pendingBodyLen);

            if (pendingBodyLen > 0) { //get remaining body
                valread = read_n_bytes(new_socket, buffer + valread, pendingBodyLen);
                if (valread == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                    fprintf(stderr, "body read timeout\n"); //as per notes given in PDF
                }
            }

            if (ds->bodyLen > 0) {
                ds->body = malloc(ds->bodyLen);
                memcpy(ds->body, buffer + ds->bodyPos, ds->bodyLen);
            }

            processRequest(ds, new_socket);
            freeHttpRequest(ds);
        }
    }
    //memset(buffer, 0, sizeof(buffer)); //not needed as we should a new buffer to handle each request
    close(new_socket);
}

int main(int argc, char *argv[]) {
    //validate input port
    int port = -1;
    if (argc < 2) {
        fprintf(stderr, "Invalid Port 0\n");
        exit(1);
    } else {
        port = atoi(argv[1]);
        if (!(port > 1 && port < 65535)) {
            fprintf(stderr, "Invalid Port 1\n");
            exit(1);
        }
        printf("port: %d\n", port);
    }

    initRegx();

    Listener_Socket lsocket;
    int ret = listener_init(&lsocket, port);
    if (ret == -1) {
        fprintf(stderr, "Invalid Port 2\n");
        exit(1);
    }

    int new_socket;
    while (1) {
        if ((new_socket = listener_accept(&lsocket)) >= 0) {
            handleHttpRequest(new_socket);
        } else {
            fprintf(stderr, "listen or accept error");
            exit(1);
        }
    }

    clearRegx();
    return 0;
}
