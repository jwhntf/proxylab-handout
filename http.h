#ifndef HTTP_H
#define HTTP_H
#include <stdlib.h>
/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *connection_hdr = "close\r\n";
static const char *proxy_connection_hdr = "close\r\n";
/* HTTP协议的Method */
enum Method { GET, POST, PUT, HEAD, CONNECT, UNINIT, };

/* HTTP协议的Version */
/* 最终都要转化为V1_0 */
enum Version { V0_9, V1_0, V1_1, V2_0, VOther, };

/* HTTPRequest和HTTPResponse结构里面请求/响应头的链表的项 */
typedef struct {
    char *key;
    char *value;
    struct header_entry* next;
} header_entry;
/* 注意释放其中的指针! */
typedef struct {
    enum Method method;
    char *host;
    char *port;
    char *path;
    enum Version version;
    /* 一个链表, 链表中的每一项都是一个键值对, 以及指向下一项的指针 */
    /* 灵感来源于书上的addrinfo链表 */
    struct header_entry* headers;
    /* 请求体 */
    char *body;
}  HTTPRequest;

/* HTTPResponse, 一开始写出来是想做一下响应解析 */
/* 不过后来发现可以直接返回和缓存响应字符串, 就没再使用, 也没舍得删 */
typedef struct {
    enum Version version;
    int status_code;
    char *status_text;
    struct header_entry* headers;
    char *body;
} HTTPResponse;

/* 展示HTTPRequest结构, 用于调试 */
void display_request(HTTPRequest *request);

/* 初始化一个HTTPRequest结构 */
void request_init(HTTPRequest *request);

/* 清理一个HTTPRequest结构 */
void request_clear(HTTPRequest *request);

/* 展示HTTPResponse结构, 用于调试 */
void display_response(HTTPResponse *response);

/*  初始化一个HTTPResponse结构*/
void response_init(HTTPResponse *response);

/* 清理一个HTTPResponse结构 */
void response_clear(HTTPResponse *response);

/* 从描述符fd中读取请求并且转化为一个HTTPRequest结构 */
int read_request(int fd, HTTPRequest*);

/* 从客户发来的request构建发往server的HTTP请求, 写入real_request字符串 */
/* 返回值为real_request的长度, 如果返回-1则构建失败 */
int construct_real_request(HTTPRequest *request, char *real_request);

// int construct_cached_response(const HTTPResponse *response, char *response_buf);

/* 向facing_server_fd发送构造好的请求字符串 */
int send_request(int facing_server_fd, const char *real_request, size_t len);

/* 从facing_server_fd读取响应, 放入buf里面, 读取的长度放到read_len里面, 返回buf */
/* 由于不知道响应预计有多长, 所以需要动态扩展buf, 用到了realloc */
/* 这样一来buf的地址可能被改变, 因此返回值是最终确定的buf地址 */
/* 这么做有点丑, 但是我觉得char **buf_ptr这种东西更加丑, 所以就没有用这种方法 */
void *read_response(int facing_server_fd, char *buf, size_t *read_len);

/* 向connfd即client传递响应 */
int send_response(int connfd, const char *response, size_t len);

/* 这个函数是存在并且写好了的, 但是没用到. 将字符串解析为一个HTTPResponse结构 */
// int parse_response(HTTPResponse *response, char *response_line, size_t response_len, size_t *result);

/* 从响应字符串中获取Content-Length字段值, 用于Cache */
size_t get_content_length(const char *response_line);

/* 获取URL. 获取的字符串用于传递给哈希函数变成一个32bit的无符号整数作为缓存的tag, 返回值是URL的长度 */
/* 获取到的URL会舍弃掉端口号(只从HTTPRequest结构中拿取host和path两个字段组合起来) */
size_t get_url(const HTTPRequest *request, char *buf, size_t len);
#endif