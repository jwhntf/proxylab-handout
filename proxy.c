#include "csapp.h"
#include "http.h"
#include "hash.h"
#include "cache.h"
#define DEBUG
#ifdef DEBUG
#define dbg_printf(...) printf(__VA_ARGS__)
#else
#define dbg_printf(...)
#endif

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

#define SEED 114514

typedef struct {
    int connfd;
} thread_args;

extern cache_object_t Cache[CACHE_SLOT];

extern unsigned int global_time;

pthread_rwlock_t rwlock;

void *thread(void *vargp) {
    pthread_detach(pthread_self());
    thread_args *argp;
    size_t read_len, write_len, url_len, content_len;
    int facing_server_fd;
    unsigned int hash_result;
    HTTPRequest request;
    char *real_request;
    char *response_buf;
    char url[MAXLINE];
    cache_object_t *cobjp;
    

    argp = (thread_args *)vargp;
    
    request_init(&request);
    if(read_request(argp->connfd, &request) == -1) {
        fprintf(stderr, "Malformed Httprequest\n");
    }
    printf("Thread[%p]: received request from client\n", pthread_self());
   
    
    /* 先尝试在本地缓存中寻找 */
    if ((url_len = get_url(&request, url, MAXLINE)) == -1) {
        fprintf(stderr, "No enough buffer\n");
    }
    hash_result = MurmurHash(url, url_len, SEED);
    
    pthread_rwlock_rdlock(&rwlock);
    if((cobjp = find_id(Cache, CACHE_SLOT, hash_result)) != NULL) {
        /* 如果在缓存中找到了, 则直接取出来, 发送 */
        cobjp->timestamp = global_time++;
        response_buf = malloc(sizeof(char) * MAX_OBJECT_SIZE * 2);
        memset(response_buf, 0, sizeof(char) * MAX_OBJECT_SIZE * 2);
        memcpy(response_buf, cobjp->objp, strlen(cobjp->objp));
        /* 访问全局变量结束, 释放锁 */
        printf("Thread[%p]: received %ld bytes from cache\n", pthread_self(), strlen(response_buf));
        pthread_rwlock_unlock(&rwlock);
        send_response(argp->connfd, response_buf, sizeof(char) * MAX_OBJECT_SIZE * 2);
        free(response_buf);
        response_buf = NULL;
        close(argp->connfd);
        free(argp);
        return NULL;
    }

    /* 没在缓存中找到, 先把读写锁释放掉 */
    pthread_rwlock_unlock(&rwlock);
    /* 没在缓存中找到, 再尝试构建请求向服务器发送 */
    facing_server_fd = open_clientfd(request.host, request.port);
    real_request = malloc(sizeof(char) * MAXLINE);
    if ((write_len = construct_real_request(&request, real_request)) == -1) {
        fprintf(stderr, "Fail to construct real request\n");
    };
    
    send_request(facing_server_fd, real_request, write_len);
    free(real_request);
    real_request = NULL;
    request_clear(&request);

    response_buf = malloc(sizeof(char) * MAXLINE);
    if ((response_buf = read_response(facing_server_fd, response_buf, &read_len)) == (void *)-1) {
        fprintf(stderr, "Fail to construct real request\n");
    }
    printf("Thread[%p]: received %ld bytes from server\n", pthread_self(), read_len);
    send_response(argp->connfd, response_buf, read_len);

    /* 如果允许cache那么就parse然后cache */
    content_len = get_content_length(response_buf);
    if (content_len <= MAX_OBJECT_SIZE) {
        cobjp = malloc(sizeof(cache_object_t));
        cobjp->id = hash_result;
        cobjp->timestamp = global_time++;
        cobjp->objp = response_buf;
        pthread_rwlock_wrlock(&rwlock);
        store_obj(Cache, CACHE_SLOT, cobjp);
        pthread_rwlock_unlock(&rwlock);

    } else {
        /* 否则才释放 */
        free(response_buf);
        response_buf = NULL;
    }
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
    /* 初始化缓存 */
    init_cache(Cache, CACHE_SLOT);
    /* 初始化读写锁 */
    pthread_rwlock_init(&rwlock, NULL);
    /* 初始化时间戳 */
    global_time = 0;
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
