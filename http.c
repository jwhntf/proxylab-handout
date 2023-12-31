#include "http.h"
#include "csapp.h"
void display_request(HTTPRequest *request) {
    printf("Method: ");
    switch (request->method)
    {
    case GET:
        printf("GET\n");
        break;
    case POST:
        printf("POST\n");
        break;
    case PUT:
        printf("PUT\n");
        break;
    default:
        printf("UNINIT\n");
        break;
    }
    printf("Version: ");
    switch (request->version)
    {
    case V0_9:
        printf("HTTP/0.9\n");
        break;
    case V1_0:
        printf("HTTP/1.0\n");
        break;
    case V1_1:
        printf("HTTP/1.1\n");
        break;
    case V2_0:
        printf("HTTP/2.0\n");
        break;
    default:
        printf("Unknown\n");
        break;
    }
    printf("Host: %s\n", request->host);
    printf("Port: %s\n", request->port);
    printf("Path: %s\n", request->path);

    header_entry *ptr;
    for (ptr = request->headers; ptr; ptr = ptr->next) {
        printf("Header key|%s, value|%s", ptr->key, ptr->value);
    }

    printf("Body: %s\n", request->body);
}

void request_init(HTTPRequest *request) {
    request->method = UNINIT;
    request->version = VOther;
    request->host = NULL;
    request->port = NULL;
    request->path = NULL;
    request->body = NULL;
    request->headers = NULL;
}

void request_clear(HTTPRequest *request) {
    header_entry* prev, *succ;
    request->method = UNINIT;
    request->version = VOther;
    if (request->host) free(request->host);
    request->host = NULL;
    if (request->port) free(request->port);
    request->port = NULL;
    if (request->path) free(request->path);
    request->path = NULL;
    if (request->body) free(request->body);
    request->body = NULL;
    prev = succ = request->headers;
    while (succ) {
        succ = prev->next;
        free(prev->key);
        free(prev->value);
        free(prev);
        prev = succ;
    }
    request->headers = NULL;
}

/* 从套接字描述符fd读取并且解析HTTP请求/响应, 成功0, 失败-1*/
int read_request(int fd, HTTPRequest *request)
{
    char buf[MAXLINE];
    char method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    rio_t rio;
    size_t uri_len, host_len, port_len, path_len, key_len, value_len;
    char *host_path_sprt = NULL;
    char *host_start = NULL;
    char *path_start = NULL;
    char *port_start = NULL;


    rio_readinitb(&rio, fd);

    if (!rio_readlineb(&rio, buf, MAXLINE))
        return -1;
    printf("First_line: %s\b", buf);
    sscanf(buf, "%s %s %s", method, uri, version);
    printf("Method: %s, URI: %s, Version: %s\n", method, uri, version);
    if (!strcasecmp(method, "GET")) {
        request->method = GET;
    } else if (!strcasecmp(method, "POST")) {
        request->method = POST;
    } else if (!strcasecmp(method, "PUT")) {
        request->method = PUT;
    } else if (!strcasecmp(method, "HEAD")) {
        request->method = HEAD;
    } else if (!strcasecmp(method, "CONNECT")) {
        request->method = CONNECT;
    } else {
        // return -1;
        request->method = UNINIT;
    }

    uri_len = strlen(uri);
    /* 拆解uri为hostname和资源路径 */
    if ((host_path_sprt = strstr(uri, "://")) != NULL) {
        host_start = host_path_sprt + 3;
    } else {
        host_start = uri;
    }

    if ((path_start = strchr(host_start, '/')) == NULL) {
        /* 没有指明资源路径, 设置连'/'都没有 */
        /* 那么将资源路径设置为'/', 自动附加 */
        request->path = malloc(sizeof(char) * 2);
        request->path[0] = '/';
        request->path[1] = '\0';
        path_start = uri + uri_len; /* path_start放到末尾 */
    } else {
        /* 指明了资源路径 */
        path_len = strlen(path_start);
        request->path = malloc(sizeof(char) * (path_len + 1));
        memcpy(request->path, path_start, sizeof(char)*path_len);
        request->path[path_len] = '\0';
    }

    /* 夹在host_start和path_start两个指针之间的就是hostname + port */
    *path_start = '\0';
        
    if ((port_start = strchr(host_start, ':')) == NULL) {
        /* 没有发现端口 */
        /* 则request里面的端口号就空着 */
        port_start = uri + uri_len; /* port_start放到末尾 */ 
    } else {
        /* 发现了端口号 */
        port_len = strlen(port_start + 1);
        request->port = malloc(sizeof(char) * (port_len + 1));
        memcpy(request->port, port_start + 1, sizeof(char)*port_len);
        request->port[port_len] = '\0';
        // printf("PORTTTTTT: %s\n", request->port);
    }
    *port_start = '\0';

    host_len = strlen(host_start);
    request->host = malloc(sizeof(char) * (host_len + 1));
    memcpy(request->host, host_start, sizeof(char) * host_len);
    request->host[host_len] = '\0';
    

    if (!strcasecmp(version, "HTTP/0.9")) {
        request->version = V0_9;
    } else if (!strcasecmp(version, "HTTP/1.0")) {
        request->version = V1_0;
    } else if (!strcasecmp(version, "HTTP/1.1")) {
        request->version = V1_1;
    } else if (!strcasecmp(version, "HTTP/2.0")) {
        request->version = V2_0;
    }

    char *p = NULL;
    header_entry *ptr, **guard = &request->headers;
    while (strcmp(buf, "\r\n")) {
        /* while循环的这个判断条件会导致直接舍去请求体 */
        rio_readlineb(&rio, buf, MAXLINE);
        printf("%s", buf);

        if ((p = strchr(buf, ':')) != NULL) {
            ptr = malloc(sizeof(header_entry));
            memset(ptr, 0, sizeof(header_entry));
            *p = '\0';
            key_len = strlen(buf);
            value_len = strlen(p + 1);
            // printf("key_len: %u, value_len: %u\n", key_len, value_len);
            ptr->key = malloc(sizeof(char) * (key_len + 1));
            ptr->value = malloc(sizeof(char) * (value_len + 1));
            memcpy(ptr->key, buf, sizeof(char) * key_len);
            ptr->key[key_len] = '\0';
            memcpy(ptr->value, p+1, sizeof(char) * value_len);
            ptr->value[value_len] = '\0';
            
            *guard = ptr;
            guard = &(ptr->next);
        }
    }
    return 0;
}



int construct_real_request(HTTPRequest *request, char *real_request) {
    /* 注意每一个header里面的value都是自带\r\n的 */
    char *method;
    char *guard;
    header_entry *ptr;
    int have_connection = 0;
    int have_proxy_connection = 0;
    size_t method_len, path_len, version_len, header_len, body_len;
    guard = real_request;
    switch (request->method)
    {
    case GET:
        method="GET";
        break;
    case POST:
        method="POST";
        break;
    case PUT:
        method="PUT";
        break;
    case HEAD:
        method="HEAD";
        break;
    case CONNECT:
        method="CONNECT";
        break;
    default:
        method="GET";
        break;
    }
    // sprintf(real_request, method);
    method_len = strlen(method);
    memcpy(guard, method, sizeof(char) * method_len);
    guard += method_len;

    // sprintf(real_request, "%s %s", real_request, request->path);
    *(guard++) = ' ';
    path_len = strlen(request->path);
    memcpy(guard, request->path, sizeof(char)*path_len);
    guard += path_len;

    // sprintf(real_request, "%s %s\r\n", real_request, "HTTP/1.0");
    *(guard++) = ' ';
    version_len = strlen("HTTP/1.0\r\n");
    memcpy(guard, "HTTP/1.0\r\n", sizeof(char)*version_len);
    guard += version_len;

    for(ptr = request->headers; ptr; ptr=ptr->next) {
        if (!strcasecmp(ptr->key, "User-Agent")){
            // sprintf(real_request, "%s%s: %s", real_request, "User-Agent", user_agent_hdr);
            header_len = strlen("User-Agent");
            memcpy(guard, "User-Agent", sizeof(char)*header_len);
            guard += header_len;

            *(guard++) = ':';
            *(guard++) = ' ';
            header_len = strlen(user_agent_hdr);
            memcpy(guard, user_agent_hdr, sizeof(char) * header_len);
            guard += header_len;
        } else if (!strcasecmp(ptr->key, "Connection")) {
            have_connection = 1;
            // sprintf(real_request, "%s%s: %s", real_request, "Connection", connection_hdr);
            header_len = strlen("Connection");
            memcpy(guard, "Connection", sizeof(char) * header_len);
            guard += header_len;

            *(guard++) = ':';
            *(guard++) = ' ';

            header_len = strlen(connection_hdr);
            memcpy(guard, connection_hdr, sizeof(char) * header_len);
            guard += header_len;
        }
        else if (!strcasecmp(ptr->key, "Proxy-Connection")) {
            have_proxy_connection = 1;
            // sprintf(real_request, "%s%s: %s", real_request, "Proxy-Connection", proxy_connection_hdr);
            header_len = strlen("Proxy-Connection");
            memcpy(guard, "Proxy-Connection", sizeof(char)*header_len);
            guard += header_len;

            *(guard++) = ':';
            *(guard++) = ' ';

            header_len = strlen(proxy_connection_hdr);
            memcpy(guard, proxy_connection_hdr, sizeof(char) * header_len);
            guard += header_len;
        }
        else {
            // sprintf(real_request, "%s%s:%s", real_request, ptr->key, ptr->value);
            header_len = strlen(ptr->key);
            memcpy(guard, ptr->key, sizeof(char) * header_len);
            guard += header_len;
            
            *(guard++) = ':';

            header_len = strlen(ptr->value);
            memcpy(guard, ptr->value, sizeof(char) * header_len);
            guard += header_len;
        }
    }
    if (!have_connection) {
        // sprintf(real_request, "%s%s: %s", real_request, "Connection", connection_hdr);
        header_len = strlen("Connection");
        memcpy(guard, "Connection", sizeof(char) * header_len);
        guard += header_len;

        *(guard++) = ':';
        *(guard++) = ' ';

        header_len = strlen(connection_hdr);
        memcpy(guard, connection_hdr, sizeof(char) * header_len);
        guard += header_len;
    }
    if (!have_proxy_connection) {
        // sprintf(real_request, "%s%s: %s", real_request, "Proxy-Connection", proxy_connection_hdr);
        header_len = strlen("Proxy-Connection");
        memcpy(guard, "Proxy-Connection", sizeof(char)*header_len);
        guard += header_len;

        *(guard++) = ':';
        *(guard++) = ' ';

        header_len = strlen(proxy_connection_hdr);
        memcpy(guard, proxy_connection_hdr, sizeof(char) * header_len);
        guard += header_len;
    }
    if (request->body) {
        // sprintf(real_request, "%s%s\r\n", real_request, request->body);
        body_len = strlen(request->body);
        memcpy(guard, request->body, sizeof(char)*body_len);
        guard += body_len;

        *(guard++) = '\r';
        *(guard++) = '\n';
    }
    // sprintf(real_request, "%s\r\n", real_request);
    *(guard++) = '\r';
    *(guard++) = '\n';
    body_len = guard - real_request;
    // return strlen(real_request);
    *(guard++) = '\0';
    return body_len;
}

int send_request(int facing_server_fd, const char *real_request, size_t len) {
    int writed = rio_writen(facing_server_fd, real_request, len);
    return writed;
}

void *read_response(int facing_server_fd, char *response_buf, size_t *read_len) {
    if (facing_server_fd == -1)
        return -1;
    rio_t rio;
    size_t radix = 1;
    size_t time = 0;
    size_t rc = 0;
    char *guard = response_buf;
    char buf[MAXLINE];
    response_buf[0] = '\0';

    rio_readinitb(&rio, facing_server_fd);
    if ((rc = rio_readnb(&rio, buf, MAXLINE)) == 0)
        return -1;
    // printf("Read: %d bytes\n", rc);
    time+=1;
    memcpy(guard, buf, sizeof(char) * rc);
    guard += rc;

    while((rc = rio_readnb(&rio, buf, MAXLINE))) {
        // printf("Read: %d bytes\n", rc);
        // printf("response_buf: %p\n", response_buf);
        if (time == radix) {
            radix *= 2;
            size_t diff = guard - response_buf;
            response_buf = realloc(response_buf, radix * MAXLINE);
            guard = response_buf + diff;
        }
        time += 1;
        memcpy(guard, buf, sizeof(char) * rc);
        guard += rc;
    }
    // printf("response_buf length: %ld\n", radix*MAXLINE);
    *read_len = (guard - response_buf);
    return response_buf;
}

int send_response(int connfd, const char *response, size_t len) {
    int writed = rio_writen(connfd, response, len);
    return writed;
}

size_t get_url(const HTTPRequest *request, char *buf, size_t len) {
    size_t host_len, path_len;
    host_len = strlen(request->host);
    path_len = strlen(request->path);
    if (host_len + path_len >= len)
        return -1;
    memcpy(buf, request->host, host_len);
    memcpy(buf + host_len, request->path, path_len);
    *(buf + host_len + path_len) = '\0';
    return host_len + path_len;
}

// int parse_response(HTTPResponse *response, char *response_line, size_t response_len, size_t *result) {
//     size_t status_text_len, header_len, key_len, value_len, content_len;
//     char version_buf[16], status_code_buf[16], status_text_buf[16];
//     char header_buf[MAXLINE];
//     char *p, *q;
//     header_entry *ptr, **guard = &response->headers;
//     char *line_ptr;
   
//     line_ptr = response_line;

//     if ((p = strchr(line_ptr, ' ')) == NULL) {
//         return -1;
//     }
//     memcpy(version_buf, line_ptr, p - line_ptr);
//     line_ptr = p + 1;
//     if ((p = strchr(line_ptr, ' ')) == NULL) {
//         return -1;
//     }
//     memcpy(status_code_buf, line_ptr, p - line_ptr);
//     line_ptr = p + 1;
//     if ((p = strstr(line_ptr, "\r\n")) == NULL) {
//         return -1;
//     }
//     memcpy(status_text_buf, line_ptr, p - line_ptr);
//     line_ptr = p + 2;

//     if (!strcasecmp(version_buf, "HTTP/0.9")) {
//         response->version = V0_9;
//     } else if (!strcasecmp(version_buf, "HTTP/1.0")) {
//         response->version = V1_0;
//     } else if (!strcasecmp(version_buf, "HTTP/1.1")) {
//         response->version = V1_1;
//     } else if (!strcasecmp(version_buf, "HTTP/2.0")) {
//         response->version = V2_0;
//     } else {
//         response->version = VOther;
//     }
//     response->status_code = atoi(status_code_buf);
//     status_text_len = strlen(status_text_len);
    
//     response->status_text = malloc(status_text_len + 1);
//     memcpy(response->status_text, status_text_buf, status_text_len);
//     response->status_text[status_text_len] = '\0';

//     /* headers */
//     while((q = strstr(line_ptr, "\r\n")) != line_ptr) {
//         if ((p = strstr(line_ptr, ": ")) != NULL) {
//             ptr = malloc(sizeof(header_entry));
//             memset(ptr, 0, sizeof(header_entry));
//             // *p = '\0';
//             // *q = '\0';
//             // key_len = strlen(line_ptr);
//             // value_len = strlen(p + 2);
//             key_len = p - line_ptr;
//             value_len = q - (p + 2);
//             ptr->key = malloc(sizeof(char) * (key_len + 1));
//             ptr->value = malloc(sizeof(char) * (value_len + 1));
//             memcpy(ptr->key, line_ptr, sizeof(char) * key_len);
//             ptr->key[key_len] = '\0';
//             memcpy(ptr->value, p + 1, sizeof(char) * value_len);
//             ptr->value[value_len] = '\0';

//             if (!strcasecmp(ptr->key, "Content-length")) {
//                 content_len = atol(ptr->value);
//             }

//             *guard = ptr;
//             guard = &(ptr->next);

//             line_ptr = q + 2;
//         }
//     }
//     /* 跳过空的CRLF行 */
//     line_ptr = line_ptr + 2;
//     /* body */
//     response->body = malloc(sizeof(char) * (content_len + 1));
//     memcpy(response->body, line_ptr, content_len);
//     response->body[content_len] = '\0';
//     *result = content_len;
//     return 0;
// }

void display_response(HTTPResponse *response) {
    printf("Version: ");
    switch (response->version)
    {
    case V0_9:
        printf("HTTP/0.9\n");
        break;
    case V1_0:
        printf("HTTP/1.0\n");
        break;
    case V1_1:
        printf("HTTP/1.1\n");
        break;
    case V2_0:
        printf("HTTP/2.0\n");
        break;
    default:
        printf("Unknown\n");
        break;
    }
    printf("Status: %d %s\n", response->status_code, response->status_text);
    header_entry *ptr;
    for (ptr = response->headers; ptr; ptr = ptr->next) {
        printf("Header key|%s, value|%s", ptr->key, ptr->value);
    }

    printf("Body: %s\n", response->body);
}

void response_init(HTTPResponse *response) {
    response->version = VOther;
    response->status_code = 0;
    response->status_text = NULL;
    response->headers = NULL;
    response->body = NULL;
}

void response_clear(HTTPResponse *response) {
    header_entry* prev, *succ;
    response->version = VOther;
    response->status_code = 0;
    if (response->status_text) free(response->status_text);
    response->status_text = NULL;
    if (response->body) free(response->body);
    response->body = NULL;
    prev = succ = response->headers;
    while (succ) {
        succ = prev->next;
        free(prev->key);
        free(prev->value);
        free(prev);
        prev = succ;
    }
    response->headers = NULL;
}

size_t get_content_length(const char *response_line) {
    char *p, *q;
    char buf[128];
    if ((p = strstr(response_line, "Content-length")) == NULL) {
        if ((p = strstr(response_line, "Content-Length")) == NULL) {
            return 0;
        }
    }
    if ((q = strstr(p, "\r\n")) == NULL) {
        return 0;
    }
    size_t numlen = q - (p + 16);
    memcpy(buf, p + 16, sizeof(char) * numlen);
    buf[numlen] = '\0';
    size_t ans = atol(buf);
    return ans;
}