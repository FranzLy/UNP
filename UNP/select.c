/*
 * @Descripttion: 
 * @version: 
 * @Author: liyu
 * @Date: 2021-10-24 00:06:25
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2021-10-24 00:50:02
 */
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <error.h>
#include <sys/select.h>
#include <math.h>

#define SERVER_PROT 9527

int main()
{
    int lfd, cfd;                          /* listen fd   connect fd */
    int ret,                               /* return value */
        opt,                               /* options used to setsockopt */
        maxfd,                             /* max fd */
        data_size;                         /* data size */
    char buf[BUFSIZ];                      /* buffer used to read and write data */
    struct sockaddr_in srv_addr, clt_addr; /* server address     client address */
    socklen_t clt_addr_len;                /* client address length */
    fd_set rset, allset;                   /* read set     all set */

    /* create socket */
    lfd = socket(AF_INET, SOCK_STREAM, 0);

    /* set port multiple use */
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    /* set server address attribute and bind */
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(SERVER_PROT);
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(lfd, (struct sockaddr *)&srv_addr, sizeof(srv_addr));

    /* set max block */
    listen(lfd, 128);

    /* put lfd into allset */
    maxfd = lfd;
    FD_ZERO(&allset);
    FD_SET(lfd, &allset);

    while (1)
    {
        /* the interesting read set */
        rset = allset;

        /* note: maxfd+1 */
        ret = select(maxfd + 1, &rset, NULL, NULL, NULL);
        if (ret == -1)
        {
            perror("select error");
            exit(1);
        }

        /* check if there is a new connect event */
        if (FD_ISSET(lfd, &rset))
        {
            /* connect */
            clt_addr_len = sizeof(clt_addr);
            cfd = accept(lfd, (struct sockaddr *)&clt_addr, &clt_addr_len);

            /* put the event into allset */
            FD_SET(cfd, &allset);

            /* update the maxfd number */
            if (maxfd < cfd)
            {
                maxfd = cfd;
            }

            /* continue if only the listen event */
            if (ret == 1)
            {
                continue;
            }
        }

        /* check all events and read data */
        for (int i = lfd + 1; i <= maxfd; i++)
        {
            /* if the event is in rset, which says data is coming */
            if (FD_ISSET(i, &rset))
            {
                /* read data */
                data_size = read(i, buf, sizeof(buf));
                if (data_size == -1)
                { /* read error */
                    perror("read error");
                    exit(1);
                }
                else if (data_size == 0)
                { /* no data, client has closed */
                    close(i);
                    FD_CLR(i, &allset);
                }
                else
                { /* print the data and operate */
                    write(STDOUT_FILENO, buf, data_size);
                    for (int j = 0; j < data_size; j++)
                    {
                        buf[j] = toupper(buf[j]);
                    }
                    write(i, buf, data_size);
                }
            }
        }
    }

    /* close listen fd */
    close(lfd);

    return 0;
}