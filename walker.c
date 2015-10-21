#include "walker.h"
#include "http.h"
#include "threadpool.h"
#include "fdopt.h"
#include "conf.h"

#include <sys/epoll.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

/*@ init_daemon:
 *@     to make the server a daemon
 */
extern struct walker walker;
extern struct config walkerconf[];
#define EPOLLMAX    10
#define RW_RW_RW (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)

int epollfd;


void init_daemon()
{
    pid_t pid;

    pid = fork();

    if(pid<0){
        printf ("daemon_init error\n");
        exit(0);
    }
    
    if (pid>0)
        exit(0);
    
    setsid();

    pid = fork();
    
    if(pid<0)
        exit(1);
    if(pid>0)
        exit(0);

    umask(0027);
    for (int i=0; i<FDMAX; i++)
        close(i);
    
    walker.errlog = fopen(walkerconf[LOGFILE].value ,"a");
    walker.stime = time(NULL);
    //int fd = creat("walker.pid", RW_RW_RW);
    pid = getpid();
    creat("walker.pid", S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    FILE *f = fopen("walker.pid", "w");
    fprintf(f, "%d", pid);
    chdir("/");
    
}

/*@ errlog:
 *@ print the error information to error logfile
 */

void errlog(char *string)
{
    time_t tm = time(NULL);

    fprintf (walker.errlog, "%s" "%s",  asctime(localtime(&tm)), string);
}


/*@ tcp_listen:
 *@     establish a tcp listen on a specific port.
 *@     function return the listen fd
 */

int tcp_listen(int port, char *addr, int nlisten)
{
    struct sockaddr_in seraddr;
    int sockfd;

    bzero(&seraddr, sizeof(seraddr));
    seraddr.sin_family =AF_INET;
    seraddr.sin_port = htons(port);
    if (addr){
        if( inet_pton(AF_INET, addr, &seraddr.sin_addr.s_addr) == 0){
            errlog("address format error");
            exit(1);
        }
    }else{
        seraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    if (bind(sockfd, (struct sockaddr*)&seraddr, sizeof(seraddr)) == -1){
        errlog("tcp_listen: bind failed\n");
        if (errno==EACCES)
            errlog("*****try root again\n");
        exit(1);
    }

    if (listen(sockfd, nlisten)){
        errlog("tcp_listen: listen failed\n");
        exit(1);
    }

    set_nonblock(sockfd);
    set_port_reused(sockfd, &seraddr, sizeof(seraddr));
    walker.fd = sockfd;
    return sockfd;
}


/*@ handle_connect:
 *@    handle user request
 *@
 *@
 */
/*
void handle_connect(int fd, threadpool_t *tpool)
{
    int clifd;

    epollfd = epoll_create(100);
    struct epoll_event ev_ser, ev_cli;
    ev_ser.data.fd = fd;
    ev_ser.events = EPOLLIN;
    
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev_ser);
    
    struct epoll_event *ea = (struct epoll_event *)malloc(sizeof(struct epoll_event)*EPOLLMAX);

    walker.stop=0;
    while (!walker.stop){

        memset(ea, '\0', sizeof(struct epoll_event)*EPOLLMAX);
        int ret = epoll_wait(epollfd, ea, EPOLLMAX, -1);

        for (int i=0; i<ret; i++){
            if (ea[i].data.fd == fd && (ea[i].events & EPOLLIN)){
                clifd = user_accept(fd);
                ev_cli.events = EPOLLIN;
                ev_cli.data.fd = clifd;
                epoll_ctl(epollfd, EPOLL_CTL_ADD, clifd, &ev_cli);   
            }else if(ea[i].events & EPOLLIN){
                http_handle(ea[i].data.fd);
                threadpool_add(tpool, job, (void*)ea[i].data.fd, 0);
            }
        }
    }
}*/

/*
void job(void *arg)
{
    http_handle((int)arg);
}
*/

/*@ user_accept:
 *@      accept the user connection,
 *@      return client fd.
 */

int user_accept(int fd)
{
    struct sockaddr_in addr;
    int cli_fd;
    socklen_t len;
    cli_fd = accept(fd, (struct sockaddr*)&addr, &len);
    /*slightly handle the thundering herd*/
    if (cli_fd == -1 && errno == EAGAIN){
        return -1;
    }
    
    //char *ip = inet_ntoa(addr.sin_addr);
    //if (strcasecmp(ip, "0.0.0.0")==0){
    //    return -1;
    //}
    return cli_fd;
}



/*@ do_subprocess_job:
 *@      subprocess listen on the server fd using epoll function.
 *@      When it returns, accept and ignore the thundering herd.
 *@      Subprocess will also create a threadpool. When it is a 
 *@      GET without qeuring string, just add the job to threadpool,
 *@      else, create a cgi job and a pipe to wait.
 */

void do_subprocess_job(int serfd)
{
    int clifd, epollfd, nthread;
    struct epoll_event ev, *ea;
    threadpool_t *tpool=NULL;
    
    nthread = atoi(walkerconf[THREADNUMBER].value);
    if (nthread>0)
        tpool = threadpool_create(nthread, 4, 0);

    /*epoll routine*/
    epollfd = epoll_create(100);
    ev.data.fd = serfd;
    ev.events = EPOLLIN;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, serfd, &ev);
    ea=(struct epoll_event*)malloc(sizeof(ev)*EPOLLMAX);
    
    
    while(1){
        memset(ea, 0, sizeof(ev)*EPOLLMAX);
        int ret = epoll_wait(epollfd, ea, EPOLLMAX, -1);
        for (int i=0; i<ret; i++){
            if (ea[i].data.fd == serfd && ea[i].events & EPOLLIN){
                clifd = user_accept(serfd);
                if (clifd == -1)/*connected has been established by other process*/
                    continue;
                ev.data.fd = clifd;
                ev.events = EPOLLIN | EPOLLONESHOT;
                //ev.events = EPOLLIN;
                epoll_ctl(epollfd, EPOLL_CTL_ADD, clifd, &ev);
            }else if (ea[i].events & EPOLLIN){
                http_handle(ea[i].data.fd, tpool, epollfd);             
            }
        }
    }
}



char *tmmodify(time_t timeval, char *time)
{
    char other[24];
    char year[8];
    int month;

    struct tm * local = localtime(&timeval);

    strftime(year, 7, "%Y", local);
    month = getmonth(local);

	strftime(other,23,"%d %H:%M:%S",local);
	sprintf(time,"%s/%d/%s\r\n",year,month,other);
	return time;
}


int getmonth(struct tm* local)   // return month index ,eg. Oct->10
{
	char buf[8];
	int i;
	static char *months[]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sept","Oct","Nov","Dec"};

	strftime(buf,127,"%b",local);
	for(i=0;i<12;++i)
	{
		if(!strcmp(buf,months[i]))
			return i+1;
	}
	return 0;
}
