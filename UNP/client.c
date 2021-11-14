/*
 * @Descripttion: 
 * @version: 
 * @Author: liyu
 * @Date: 2021-10-23 22:45:36
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2021-10-24 00:08:24
 */
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <error.h>

#define SERVER_PORT 9527

int main()
{
    int cfd;                     /* connect fd */
    char buf[BUFSIZ];            /* buffer */
    int ret;                     /* return value */
    struct sockaddr_in srv_addr; /* server address */

    /* create socket used to connect to server */
    cfd = socket(AF_INET, SOCK_STREAM, 0);

    /* get server attribute */
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(SERVER_PORT);
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    /* connect to server */
    ret = connect(cfd, (struct sockaddr *)&srv_addr, sizeof(struct sockaddr));
    if (ret == -1)
    {
        perror("connect error");
        exit(1);
    }

    /* comunitcate */
    while (1)
    {
        /* read input into buffer */
        ret = read(STDIN_FILENO, buf, sizeof(buf));

        /* transfer strings to server */
        write(cfd, buf, ret);

        /* get result from server */
        ret = read(cfd, buf, sizeof(buf));

        /* print the output */
        write(STDOUT_FILENO, buf, ret);
    }

    /* close connect */
    close(cfd);

    return 0;
}