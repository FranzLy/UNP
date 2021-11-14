/*
 * @Descripttion: 
 * @version: 
 * @Author: liyu
 * @Date: 2021-11-10 06:37:44
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2021-11-10 06:52:33
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <event2/event.h>

/* 写回调函数 */
void write_cb(evutil_socket_t fd, short what, void *arg)
{
    char buf[1024] = {0};

    printf("write event: %s \n", what & EV_WRITE ? "YES" : "NO");

    static int num = 0;
    sprintf(buf, "hello world-%d\n", num++);
    write(fd, buf, strlen(buf) + 1);

    sleep(1);
}

int main(int argc, char *argv[])
{
    int fd = open("myfifo", O_WRONLY | O_NONBLOCK);
    if (fd == -1)
    {
        perror("open fifo error");
        exit(1);
    }

    /* 创建event_base */
    struct event_base *base = NULL;
    base = event_base_new();

    /* 创建事件 */
    struct event *ev = NULL;
    ev = event_new(base, fd, EV_WRITE, write_cb, NULL); /* 只写一次 */
    //ev = event_new(base, fd, EV_WRITE | EV_PERSIST, write_cb, NULL);/* 持续写*/

    /* 添加事件 */
    event_add(ev, NULL);

    /* 事件循环 */
    event_base_dispatch(base);

    /* 释放资源 */
    event_free(ev);
    event_base_free(base);
    close(fd);

    return 0;
}