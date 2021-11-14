/*
 * @Descripttion: 
 * @version: 
 * @Author: liyu
 * @Date: 2021-11-03 09:23:29
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2021-11-03 10:02:08
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
    printf("server say:%s\n", buf);

    /* 写数据给客户端 */
    bufferevent_write(bev, buf, strlen(buf) + 1);

    sleep(1);
}

/* 写缓冲回调 */
void write_cb(struct bufferevent *bev, void *arg)
{
    printf("我是客户端，成功写回数据给服务器端，写缓冲区回调函数被回调...\n");
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
    else if (events & BEV_EVENT_CONNECTED)
    {
        printf("已经连接服务器...\\(^O^)/...\n");
        return;
    }

    /* 释放main()中的bufferevent */
    bufferevent_free(bev);
    printf("bufferevent 资源已被释放...\n");
}

/* 客户端与用户交互，从终端读取数据写给服务器 */
void read_terminal(evutil_socket_t fd, short what, void *arg)
{
    /* 读数据 */
    char buf[1024] = {0};
    int len = read(fd, buf, sizeof(buf));

    struct bufferevent *bev = (struct bufferevent *)arg;

    /* 发送数据 */
    bufferevent_write(bev, buf, len + 1);
}

int main(int argc, char *argv[])
{
    struct event_base *base = NULL;
    base = event_base_new();

    int fd = socket(AF_INET, SOCK_STREAM, 0);

    /* 通信的fd放到bufferevent中 */
    struct bufferevent *bev = NULL;
    bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);

    /* init server info */
    struct sockaddr_in srv_addr;
    bzero(&srv_addr, 0);
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(SERVER_PROT);
    inet_pton(AF_INET, "127.0.0.1", &srv_addr.sin_addr.s_addr);

    /* 连接服务器 */
    bufferevent_socket_connect(bev, (struct sockaddr *)&srv_addr, sizeof(srv_addr));

    /* 设置回调 */
    bufferevent_setcb(bev, read_cb, write_cb, event_cb, NULL);

    /* 设置读回调生效 */
    //bufferevent_enable(bev, EV_READ);

    /* 创建事件 */
    struct event *ev = event_new(base, STDIN_FILENO, EV_READ | EV_PERSIST, read_terminal, bev);

    /* 添加事件 */
    event_add(ev, NULL);

    /* 循环监听 */
    event_base_dispatch(base);

    event_free(ev);

    event_base_free(base);

    return 0;
}