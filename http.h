#ifndef _HTTP_H
#define _HTTP_H

#include "threadpool.h"


struct http_job{
    int         fd;
    char        *filepath;   
};

struct http_job * http_make_job(int, char*);
void http_thread_work(void*);


void http_handle(int, threadpool_t *, int);

int http_analyze(int, threadpool_t *);

int http_analy(int);

int http_getline(int, char*, int);

int http_send_file(int, char *);
int http_getfiletype(char *, char *);

int http_show_dir(int, char *);

void http_mk_head(int);

int http_send(int, char*);





int http_not_found(int);

int http_not_support(int);

int http_error(int);






#endif
