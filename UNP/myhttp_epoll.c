/*
 * @Descripttion: 
 * @version: 
 * @Author: liyu
 * @Date: 2021-11-10 06:55:57
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2021-11-13 09:19:54
 */
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <error.h>
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <strings.h>
#include <string.h>
#include <dirent.h>/* scandir() */

#define MAXSIZE 2048
#define BUFFERSIZE 1024

/* 初始化监听 */
int init_listen(int port, int epfd)
{
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd == -1)
    {
        perror("socket error");
        //exit(1);
    }

    struct sockaddr_in srv_addr, clt_addr;

    bzero(&srv_addr, 0);
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(port);
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    /* 设置端口复用 */
    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, 1);

    /* 给lfd绑定地址结构 */
    int ret = bind(lfd, (struct sockaddr *)&srv_addr, sizeof(srv_addr));
    if (ret == -1)
    {
        perror("bind error");
        //exit(1);
    }

    listen(lfd, 20);

    struct epoll_event tep;
    tep.events = EPOLLIN;
    tep.data.fd = lfd;
    ret = epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &tep);
    if (ret == -1)
    {
        perror("epoll_ctl add lfd to tree error");
        // exit(1);
    }

    return lfd;
}

/* 接收客户端的连接请求，并将其上树 */
void do_accept(int lfd, int epfd)
{
    struct sockaddr_in clt_addr;
    socklen_t clt_addr_len;

    clt_addr_len = sizeof(clt_addr);
    int cfd = accept(lfd, (struct sockaddr *)&clt_addr, &clt_addr_len);
    if (cfd == -1)
    {
        perror("accept error");
        //exit(1);
    }

    /* 打印客户端信息 */
    char client_ip[64] = {0};
    printf("client ip:%s, port: %d, cfd=%d\n",
           inet_ntop(AF_INET, &clt_addr.sin_addr.s_addr, client_ip, sizeof(client_ip)),
           ntohs(clt_addr.sin_port),
           cfd);

    /* 设置cfd非阻塞 */
    int flag = fcntl(cfd, F_GETFL);
    flag |= O_NONBLOCK;
    fcntl(cfd, F_SETFL, flag);

    struct epoll_event tep;
    tep.events = EPOLLIN;
    tep.data.fd = cfd;

    /* 设置边沿非阻塞模式 */
    tep.events = EPOLLIN | EPOLLET;
    int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &tep);
    if (ret == -1)
    {
        perror("epoll_ctl add cfd error");
        //exit(1);
    }
}

/* 读取请求首部的一行 */
int get_line(int cfd, char *buf, int size)
{
    int i = 0;
    char c = '\0';
    int n;
    while ((i < size - 1) && (c != '\n'))
    {
        n = recv(cfd, &c, 1, 0);
        if (n > 0)
        {
            if (c == '\r')
            {
                n = recv(cfd, &c, 1, MSG_PEEK); /* 模拟读一次 */
                if ((n > 0) && (c == '\n'))
                {
                    recv(cfd, &c, 1, 0);
                }
                else
                {
                    c = '\n';
                }
            }
            buf[i] = c;
            i++;
        }
        else
        {
            c = '\n';
        }
    }
    buf[i] = '\0';

    if (n == -1)
    {
        i = n;
    }

    return i;
}

/* 获取文件类型 */
char *get_file_type(const char *filename)
{
    char *dot;
    /* 自右向左查找'.'，如不存在则返回NULL */
    dot = strrchr(filename, '.');
    if (dot == NULL)
        return "text/plain;charset=utf-8";
    if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
        return "text/html;charset=utf-8";
    if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
        return "image/jpeg";
    if (strcmp(dot, ".gif") == 0)
        return "image/gif";
    if (strcmp(dot, ".png") == 0)
        return "image/png";
    if (strcmp(dot, ".ico") == 0)
        return "image/vnd.microsoft.icon";
    if (strcmp(dot, ".css") == 0)
        return "text/css";
    if (strcmp(dot, ".au") == 0)
        return "audio/basic";
    if (strcmp(dot, ".wav") == 0)
        return "audio/wav";
    if (strcmp(dot, ".avo") == 0)
        return "vedio/x-msvideo";
    if (strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0)
        return "video/quicktime";
    if (strcmp(dot, ".mpeg") == 0 || strcmp(dot, ".mpe") == 0)
        return "video/mpeg";
    if (strcmp(dot, ".vrml") == 0 || strcmp(dot, ".wrl") == 0)
        return "model/vrml";
    if (strcmp(dot, ".midi") == 0 || strcmp(dot, ".mid") == 0)
        return "audio/midi";
    if (strcmp(dot, ".mp3") == 0)
        return "audio/mpeg";
    if (strcmp(dot, ".ogg") == 0)
        return "application/ogg";
    if (strcmp(dot, ".pac") == 0)
        return "application/x-ns-proxy-autoconfig";

    return "text/plain;charset=utf-8";
}

/* 16进制转化为10进制 */
int hexit(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c > 'A' && c <= 'F')
        return c - 'A' + 10;
    return 0;
}

/* 将本地的中文编码成浏览器可识别的unicode码 */
/* 
    %20 在URL中表示空格' '
    %21 '!' 
    %22 '"'
    %23 '#'
    %24 '$'
    ...
 */
void encode_str(char *dst, int dstsize, const char *src)
{
    int dstlen;
    for (dstlen = 0; *src != '\0' && dstlen + 4 < dstsize; ++src)
    {
        if (isalnum(*src) || strchr("/_.-~", *src) != (char *)0)
        {
            *dst = *src;
            ++dst;
            ++dstlen;
        }
        else
        {
            sprintf(dst, "%%%02x", (int)*src & 0xff);
            dst += 3;
            dstlen += 3;
        }
    }
    *dst = '\0';
}

/* 将浏览器发送过来的中文unicode码编成本地中文 */
void decode_str(char *dst, char *src)
{
    for (; *src != '\0'; ++dst, ++src)
    {
        if (src[0] == '%' && isxdigit(src[1]) && isxdigit(src[2]))
        {
            *dst = hexit(src[1]) * 16 + hexit(src[2]);
            src += 2;
        }
        else
        {
            *dst = *src;
        }
    }
    *dst = '\0';
}

/* 客户端的fd, 错误号 错误描述 回发文件类型，文件长度 */
void send_respond(int cfd, int no, char *comment, char *type, int len)
{
    char buf[1024] = {0};
    sprintf(buf, "HTTP/1.1 %d %s\r\n", no, comment);
    sprintf(buf + strlen(buf), "Content-Type:%s\r\n", type);
    sprintf(buf + strlen(buf), "Content-length:%d\r\n", len);
    send(cfd, buf, strlen(buf), 0);
    send(cfd, "\r\n", 2, 0);
}

/* 发送404错误页面 */
void send_error_page(int cfd)
{
    int n = 0;
    char buf[BUFFERSIZE] = {0};
    int fd;

    send_respond(cfd, 404, "Not found", get_file_type("/home/liyu/Desktop/UNP/test.html"), -1);

    fd = open("/home/liyu/Desktop/UNP/test.html", O_RDONLY);
    while ((n = read(fd, buf, sizeof(buf))) > 0)
    {
        send(cfd, buf, n, 0);
    }
}

/* 将文件内容回发给浏览器 */
void send_file(int cfd, const char *file)
{
    int n = 0;
    char buf[1024] = {0};
    int fd = open(file, O_RDONLY);
    if (fd == -1)
    {
        /* 404错误页面 */
        send_error_page(cfd);
        perror("open file error");
        //exit(1);
    }

    while ((n = read(fd, buf, sizeof(buf))) > 0)
    {
        int ret = send(cfd, buf, n, 0);
        if (ret == -1)
        {
            printf("errno= %d \n", errno);
            if (errno == EAGAIN)
            {
                printf("------------EAGAIN!\n");
                continue;
            }
            else if (errno == EINTR)
            {
                printf("------------EINTR!\n");
                continue;
            }
            else
            {
                perror("send error");
                //exit(1);
            }
        }
        printf("send data size = %d \n", ret);
    }

    close(fd);
}

/* 将目录内容回发给浏览器 */
void send_dir(int cfd, const char *dir)
{
    int i, ret;

    /* 拼接一个html页面<table></table> */
    char buf[BUFFERSIZE] = {0};

    sprintf(buf, "<html><head><title>目录名：%s</title></head>", dir);
    sprintf(buf + strlen(buf), "<body><h1>当前目录：%s</h1><table>", dir);

    char enstr[BUFFERSIZE] = {0};
    char path[BUFFERSIZE] = {0};

    /* 目录项二级指针 */
    struct dirent **ptr;
    int num = scandir(dir, &ptr, NULL, alphasort);

    /* 遍历目录 */
    for (i = 0; i < num; i++)
    {
        char *name = ptr[i]->d_name;

        /* 拼接文件的完整路径 */
        sprintf(path, "%s/%s", dir, name);
        printf("------path=%s------\n", path);

        struct stat st;
        stat(path, &st);

        encode_str(enstr, sizeof(enstr), name);
        printf("enstr=%s\n", enstr);

        if (S_ISREG(st.st_mode)) /* 如果是文件 */
        {
            sprintf(buf + strlen(buf),
                    "<tr><td><a href=\"%s\">%s</a></td><td>%ld</td></tr>",
                    enstr, name, (long)st.st_size);
        }
        else if (S_ISDIR(st.st_mode)) /* 如果是目录 */
        {
            sprintf(buf + strlen(buf),
                    "<tr><td><a href=\"%s/\">%s/</a></td><td>%ld</td></tr>",
                    enstr, name, (long)st.st_size);
        }
        //printf("html=%s", buf);
        ret = send(cfd, buf, strlen(buf), 0);
        if (ret == -1)
        {
            if (errno == EAGAIN || errno == EINTR)
            {
                perror("send error:");
                continue;
            }
            else
            {
                perror("send error:");
                //exit(1);
            }
        }
        memset(buf, 0, sizeof(buf));
    }

    sprintf(buf + strlen(buf), "</table></body><html>");
    send(cfd, buf, strlen(buf), 0);

    printf("send dir msg OK.\n");
}

/* 处理http请求 */
void http_request(int cfd, char *buf)
{
    char method[16], path[256], protocol[16];
    /* 利用正则表达式将buf按照一定规则拆解成多个子字符串 [^ ]可用来匹配除空格之外的任意字符 */
    sscanf(buf, "%[^ ] %[^ ] %[^ ]", method, path, protocol);
    printf("------method=%s, path=%s, protocol=%s", method, path, protocol);
    while (1)
    {
        bzero(buf, 0);
        int len = get_line(cfd, buf, BUFFERSIZE);
        if (len == 1 || len == -1)
        {
            break;
        }
        printf("------%s", buf);
    }
    if (strncasecmp(method, "GET", 3) == 0) /* 判断是否是GET方法 */
    {
        char destr[BUFFERSIZE];
        char *file = path + 1;

        /* 若未指定访问资源，则直接展示根目录内容 */
        if (strcmp(path, "/") == 0)
        {
            file = "./";
        }

        printf("-------file=%s------------\n", file);
        decode_str(destr, file);
        printf("-------destr=%s------------\n", destr);

        struct stat sbuf;
        /* 判断文件是否存在 */
        int ret = stat(destr, &sbuf);
        if (ret != 0)
        {
            /* 回发404错误页面 */
            send_error_page(cfd);
            perror("stat error");
            //exit(1);
        }

        if (S_ISREG(sbuf.st_mode)) /* 请求的是一个文件 */
        {
            printf("the client wants a file: %s\n", destr);

            /* 回发响应头部信息 */
            send_respond(cfd, 200, "Ok", get_file_type(destr), sbuf.st_size);

            /* 回发文件数据 */
            send_file(cfd, destr);
        }
        else if (S_ISDIR(sbuf.st_mode))
        {
            printf("the client wants a directory: %s\n", destr);

            /* 回发响应头部信息 */
            send_respond(cfd, 200, "Ok", get_file_type(".html"), sbuf.st_size);

            /* 回发目录数据 */
            send_dir(cfd, destr);
        }
    }
}

void do_read(int cfd, int epfd)
{
    char buf[BUFFERSIZE];
    int data_size = get_line(cfd, buf, sizeof(buf)); /* 读一行http协议，拆分，获取GET 文件名 协议号 */
    if (data_size == -1)
    {
        perror("read error");
        //exit(1);
    }
    else if (data_size == 0)
    {
        int ret = epoll_ctl(epfd, EPOLL_CTL_DEL, cfd, NULL);
        if (ret == -1)
        {
            perror("epoll_ctl del cfd error");
            //exit(1);
        }
        close(cfd);
    }
    else
    {
        http_request(cfd, buf);
    }
}

void *epoll_run(int port)
{
    int i;
    struct epoll_event all_event[MAXSIZE];

    /* 创建epoll监听树 */
    int epfd = epoll_create(MAXSIZE);
    if (epfd < 0)
    {
        perror("epoll_create error");
        //exit(1);
    }

    /* 创建lfd, 并添加至监听树 */
    int lfd = init_listen(port, epfd);

    while (1)
    {
        int nfds = epoll_wait(epfd, all_event, MAXSIZE, -1);

        if (nfds == -1)
        {
            perror("epoll_wait error");
            //exit(1);
        }

        for (int i = 0; i < nfds; i++)
        {
            /* 默认只处理读事件 */
            if (!(all_event[i].events & EPOLLIN))
            {
                continue;
            }

            /* 处理连接事件 */
            if (all_event[i].data.fd == lfd)
            {
                do_accept(lfd, epfd);
            }
            else /* 处理读事件 */
            {
                do_read(all_event[i].data.fd, epfd);
            }
        }
    }
    close(lfd);
}

int main(int argc, char *argv[])
{
    /* 通过命令行参数获取 端口 和 server提供的目录 */
    if (argc < 3)
    {
        printf("format: ./a.out port path\n");
        //exit(1);
    }

    /* 获取用户输入的端口 */
    int port = atoi(argv[1]);

    /* 改变进程工作目录 */
    int ret = chdir(argv[2]);
    if (ret != 0)
    {
        perror("child error");
        // exit(1);
    }

    /* 启动epoll监听 */
    epoll_run(port);

    return 0;
}