/*
 * @Descripttion: 
 * @version: 
 * @Author: liyu
 * @Date: 2021-10-28 07:14:04
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2021-11-03 07:05:22
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define DEFAULT_TIME 10        /* 10s检测一次 */
#define MIN_WAIT_TASK_NUM 10   /* 如果queue_size > MIN_WAIT_TASK_NUM 则添加新的线程到线程池 */
#define DEFAULT_THREAD_VARY 10 /* 每次创建和销毁线程的个数 */
#define true 1
#define false 0

/* 任务结构体 */
typedef struct
{
    void *(*function)(void *);
    void *arg;
} threadpool_task_t;

/* 线程池管理结构体 */
struct threadpool_t
{
    pthread_mutex_t lock;           /* 锁住整个结构体 */
    pthread_mutex_t thread_counter; /* 锁住 忙线程数目busy_thr_num 的锁 */
    pthread_cond_t queue_not_full;  /* 条件变量：任务队列不为满 */
    pthread_cond_t queue_not_empty; /* 条件变量：任务队列不为空 */

    pthread_t *threads;            /* 存放线程的tid,实际上就是管理的线程组 */
    pthread_t admin_tid;           /* 管理者线程tid */
    threadpool_task_t *task_queue; /* 任务队列 */

    /* 线程池信息 */
    int min_thr_num;       /* 线程池中最小线程数 */
    int max_thr_num;       /* 线程池中最大线程数 */
    int live_thr_num;      /* 线程池中最大线程数*/
    int busy_thr_num;      /* 正在处理任务的忙碌线程数目 */
    int wait_exit_thr_num; /* 需要销毁的线程数目 */

    /* 任务队列信息 */
    int queue_front; /* 队头 */
    int queue_rear;  /* 队尾 */
    int queue_size;  /* 任务数 */

    /* 存在的任务数 */
    int queue_max_size; /* 队列能容忍的最大任务数 */

    /* 线程池状态 */
    int shutdown; /* true为关闭 */
};
