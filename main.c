#include "walker.h"
#include "conf.h"
#include "fdopt.h"
#include "threadpool.h"

#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>IPv6
#include <sys/epoll.h>
extern struct walker walker;
extern struct config walkerconf[];

#define QUESIZE 10

int epollfd;

int main (int argc, char* argv[])
{
    int ret;/*for many func's return value*/

    init_file_config();
    init_opt_config(argc, argv);

    init_daemon();
    
    int port = atoi(walkerconf[PORT].value);
    int nlisten = atoi(walkerconf[LISTENNUMBER].value);
    walker.fd = tcp_listen(port,/*address*/ NULL, nlisten);


    /*test if mutiprocess on*/

    if (strcasecmp(walkerconf[MUTIPROCESS].value, "on") == 0 &&(ret = atoi(walkerconf[PROCESSNUMBER].value)) >0){
        int pid;
        walker.pids = (pid_t*)malloc(sizeof(pid_t)*ret); 
        for (int i=0; i<ret; i++){
            if ((pid=fork())==0)
                break;
            else
                walker.pids[i] = pid;
        }

        if (pid==0)
            do_subprocess_job(walker.fd);
        else{
            /*parent job*/
        }
    } 

    
    return 0; 
}
