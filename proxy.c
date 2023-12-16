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

int main(int argc, char **argv)
{
    /* 忽略SIGPIPE信号 */
    signal(SIGPIPE, SIG_IGN);

    int listenfd, connfd, facing_server_fd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    char client_hostname[MAXLINE], client_port[MAXLINE];
    // char server_hostname[MAXLINE], server_port[MAXLINE];
    int read_request_result;
    HTTPRequest request;
    char *real_request;
    char *response_buf;
    size_t write_len, read_len;

    /* 检查命令行参数 */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    /* 在命令行指定的端口号上监听client的连接 */
    listenfd = open_listenfd(argv[1]);
    while(1) {
        clientlen = sizeof(clientaddr);
        /* 获取一个连接套接字描述符, 客户端信息保存在clientaddr里面 */
        connfd = accept(listenfd, (SA *)&clientaddr, &clientlen);
        /* 从clientaddr里面解析出host和port */
        getnameinfo((SA *)&clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
        dbg_printf("Proxy[%d]: with client connfd: %d\n", __LINE__, connfd);
        dbg_printf("Proxy[%d]: ready to read request from client\n", __LINE__);
        /* 从客户端读取请求 */
        request_init(&request);
        if(read_request(connfd, &request) == -1) {
            fprintf(stderr, "Malformed Httprequest\n");
        }
        
        facing_server_fd = open_clientfd(request.host, request.port);
        dbg_printf("Proxy[%d]: facing_server_fd: %d\n", __LINE__, facing_server_fd);
        real_request = malloc(sizeof(char) * MAXLINE);
        if ((write_len = construct_real_request(&request, real_request)) == -1) {
            fprintf(stderr, "Fail to construct real request\n");
        };
        dbg_printf("Proxy[%d]: real_request: %s\n", __LINE__, real_request);
        /* 向服务器发送http请求 */
        printf("Proxy[%d]: received %ld bytes from client\n", __LINE__, write_len);
        send_request(facing_server_fd, real_request, write_len);
        dbg_printf("Proxy[%d]: sent request to server\n", __LINE__);
        free(real_request);
        real_request = NULL;
        request_clear(&request);


        response_buf = malloc(sizeof(char) * MAXLINE);
        dbg_printf("Proxy[%d]: ready to read response from server\n", __LINE__);
        dbg_printf("response_buf: %p\n", response_buf);
        if ((response_buf = read_response(facing_server_fd, response_buf, &read_len)) == -1) {
            fprintf(stderr, "Fail to construct real request\n");
        }
        printf("Proxy[%d]: received %ld bytes from server\n", __LINE__, read_len);
        dbg_printf("response_buf: %p\n", response_buf);
        send_response(connfd, response_buf, read_len);
        dbg_printf("Proxy[%d]: response sent back to client\n", __LINE__);
        free(response_buf);
        dbg_printf("Proxy[%d]: response_buf freed explicitly\n", __LINE__);
        response_buf = NULL;
        close(connfd);
    }
    return 0;
}
