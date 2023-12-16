#include "csapp.h"
#include "http.h"
#define DEBUG
#ifdef DEBUG
#define dbg_printf(...) printf(__VA_ARGS__)
#else
#define dbg_printf(...)
#endif

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

typedef struct {
    int connfd;
} thread_args;

void *thread(void *vargp) {
    pthread_detach(pthread_self());
    thread_args *argp;
    size_t read_len, write_len;
    int facing_server_fd;
    
    HTTPRequest request;
    char *real_request;
    char *response_buf;

    argp = (thread_args *)vargp;

    request_init(&request);
    if(read_request(argp->connfd, &request) == -1) {
        fprintf(stderr, "Malformed Httprequest\n");
    }
    
    facing_server_fd = open_clientfd(request.host, request.port);
    real_request = malloc(sizeof(char) * MAXLINE);
    if ((write_len = construct_real_request(&request, real_request)) == -1) {
        fprintf(stderr, "Fail to construct real request\n");
    };
    printf("Thread[%d]: received %ld bytes from client\n", pthread_self(), read_len);
    send_request(facing_server_fd, real_request, write_len);
    free(real_request);
    real_request = NULL;
    request_clear(&request);

    response_buf = malloc(sizeof(char) * MAXLINE);
    if ((response_buf = read_response(facing_server_fd, response_buf, &read_len)) == -1) {
        fprintf(stderr, "Fail to construct real request\n");
    }
    printf("Thread[%d]: received %ld bytes from server\n", pthread_self(), read_len);
    send_response(argp->connfd, response_buf, read_len);
    free(response_buf);
    response_buf = NULL;
    close(argp->connfd);
    free(argp);
    return NULL;
}

int main(int argc, char **argv)
{
    /* 忽略SIGPIPE信号 */
    signal(SIGPIPE, SIG_IGN);

    int listenfd, connfd;
    char client_hostname[MAXLINE], client_port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    pthread_t tid;

    /* 检查命令行参数 */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    /* 在命令行指定的端口号上监听client的连接 */
    listenfd = open_listenfd(argv[1]);
    while(1) {
        clientlen = sizeof(clientaddr);
        connfd = accept(listenfd, (SA *)&clientaddr, &clientlen);
        getnameinfo((SA *)&clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);

        /* 放到堆上 */
        thread_args *arg = malloc(sizeof(thread_args));
        arg->connfd = connfd;
        pthread_create(&tid, NULL, thread, arg);
    }
    return 0;
}
