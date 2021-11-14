/*
 * @Descripttion: 
 * @version: 
 * @Author: liyu
 * @Date: 2021-11-03 08:46:37
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2021-11-03 09:22:17
 */
#include <stdio.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <strings.h>
#include <string.h>

#include <event2/bufferevent.h>
#include <event2/event.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/buffer.h>

#define SERVER_PROT 9527

/* 读缓冲回调 */
void read_cb(struct bufferevent *bev, void *arg)
{
    char buf[1024] = {0};

    bufferevent_read(bev, buf, sizeof(buf));
    printf("client say:%s\n", buf);

    char *p = "我是服务器，已经成功接收到你发送的数据！";

    /* 写数据给客户端 */
    bufferevent_write(bev, p, strlen(p) + 1);

    sleep(1);
}

/* 写缓冲回调 */
void write_cb(struct bufferevent *bev, void *arg)
{
    printf("我是服务器，成功写回数据给客户端，写缓冲区回调函数被回调...\n");
}

/* 事件回调 */
void event_cb(struct bufferevent *bev, short events, void *arg)
{
    if (events & BEV_EVENT_EOF) /* 读到数据末尾，即不再有数据发过来了 */
    {
        printf("connection closed\n");
    }
    else if (events & BEV_EVENT_ERROR)
    {
        printf("some other error\n");
    }

    /* 释放cb_listener()中的bufferevent */
    bufferevent_free(bev);
    printf("bufferevent 资源已被释放...\n");
}

/* 监听回调 */
void cb_listener(
    struct evconnlistener *listener,
    evutil_socket_t fd,
    struct sockaddr *addr,
    int len,
    void *ptr)
{
    printf("connect new client\n");

    struct event_base *base = (struct event_base *)ptr;

    /* 添加新事件 */
    struct bufferevent *bev;
    bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);

    /* 给bufferevent缓冲区设置回调 */
    bufferevent_setcb(bev, read_cb, write_cb, event_cb, NULL);

    /* 启用 bufferevent的读缓冲，默认是disable的 */
    bufferevent_enable(bev, EV_READ);
}

int main(int argc, char *argv[])
{
    /* init server */
    struct sockaddr_in srv_addr;

    bzero(&srv_addr, 0);
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(SERVER_PROT);
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    /* 创建event_base */
    struct event_base *base;
    base = event_base_new();

    /* 创建套接字   绑定    接收连接请求 */
    struct evconnlistener *listener;
    listener = evconnlistener_new_bind(base, cb_listener, base,
                                       LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE,
                                       -1, (struct sockaddr *)&srv_addr, sizeof(srv_addr));

    /* 启动循环监听 */
    event_base_dispatch(base);

    /* 释放监听器 */
    evconnlistener_free(listener);

    /* 释放base */
    event_base_free(base);

    return 0;
}