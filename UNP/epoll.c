/*
 * @Descripttion: 
 * @version: 
 * @Author: liyu
 * @Date: 2021-10-24 07:02:07
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2021-10-24 08:24:15
 */
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <error.h>
#include <ctype.h>

#define SERVER_PORT 9527
#define EPOLL_MAX_SIZE 500

int main()
{
    int lfd,                                    /* listen fd */
        cfd,                                    /* connect fd */
        efd;                                    /* epoll root fd */
    int ret,                                    /* return value of functions */
        opt,                                    /* used to setsockopt */
        nfds,                                   /* number of fds which are transferring data*/
        i,                                      /* loop factor1 */
        j,                                      /* lopp factor2 */
        data_size;                              /* data size */
    char buf[BUFSIZ];                           /* data buffer */
    struct sockaddr_in srv_addr, clt_addr;      /* server addr    client addr */
    socklen_t clt_addr_len;                     /* client addr length */
    struct epoll_event tep, ep[EPOLL_MAX_SIZE]; /* temperaray epoll event    epoll event array */

    /* create socket */
    lfd = socket(AF_INET, SOCK_STREAM, 0);

    /* set socket option */
    opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    /* set server addr attribute and bind */
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(SERVER_PORT);
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(lfd, (struct sockaddr *)&srv_addr, sizeof(struct sockaddr));
    printf("%d\n", ntohl(srv_addr.sin_addr.s_addr));

    /* set  max listen number */
    listen(lfd, 128);

    /* create epoll module */
    efd = epoll_create(EPOLL_MAX_SIZE);
    if (efd == -1)
    {
        perror("epoll_create error");
        exit(1);
    }

    /* put lfd into epoll module */
    tep.events = EPOLLIN;
    tep.data.fd = lfd;

    ret = epoll_ctl(efd, EPOLL_CTL_ADD, lfd, &tep);
    if (ret == -1)
    {
        perror("epoll_ctl error");
        exit(1);
    }

    while (1)
    {
        /* wait connect or data */
        nfds = epoll_wait(efd, ep, EPOLL_MAX_SIZE, -1);
        if (nfds == -1)
        {
            perror("epoll_wait error");
            exit(1);
        }

        for (i = 0; i < nfds; i++)
        {
            /* continue if it's not read event */
            if (!(ep[i].events & EPOLLIN))
            {
                continue;
            }

            /* check if it's lfd or not */
            if (ep[i].data.fd == lfd)
            {
                /* get client addr */
                clt_addr_len = sizeof(clt_addr);
                cfd = accept(lfd, (struct sockaddr *)&clt_addr, &clt_addr_len);
                if (cfd == -1)
                {
                    perror("accept error");
                    exit(1);
                }

                /* put client fd into epoll module */
                tep.events = EPOLLIN;
                tep.data.fd = cfd;
                ret = epoll_ctl(efd, EPOLL_CTL_ADD, cfd, &tep);
                if (ret == -1)
                {
                    perror("epoll_ctl error");
                    exit(1);
                }
            }
            else
            {
                /* read data */
                data_size = read(ep[i].data.fd, buf, sizeof(buf));
                if (data_size == -1)
                {
                    perror("read error");
                    exit(1);
                }
                else if (data_size == 0)
                {
                    /* no data, then close client connection */
                    ret = epoll_ctl(efd, EPOLL_CTL_DEL, ep[i].data.fd, NULL);
                    if (ret == -1)
                    {
                        perror("epoll_ctl clear cfd error");
                        exit(1);
                    }
                    close(ep[i].data.fd);
                }
                else
                {
                    /* read data and operate */
                    write(STDOUT_FILENO, buf, data_size);

                    for (int j = 0; j < data_size; j++)
                    {
                        buf[j] = toupper(buf[j]);
                    }

                    write(ep[i].data.fd, buf, data_size);
                }
            }
        }
    }

    close(lfd);

    return 0;
}