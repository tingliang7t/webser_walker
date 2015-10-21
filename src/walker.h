#ifndef _WALKER_H
#define _WALKER_H
#include "threadpool.h"
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#define FDMAX 1024


struct walker{
    int         fd;
    time_t      stime; /*start time*/
    int         stop;  /*control value to stop*/

    FILE *      errlog;
    pid_t *       pids; /*subprocess's pid array*/
};


struct walker walker;

/*******************************/

void init_daemon();

void errlog(char *);

int  tcp_listen(int, char*, int);

void do_subprocess_job(int);



void handle_connect(int, threadpool_t *);

void  job(void *arg);

int user_accept(int);


int getmonth(struct tm*);
char *tmmodify(time_t, char *);

#endif
