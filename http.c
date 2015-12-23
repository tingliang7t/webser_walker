#include "http.h"
#include "conf.h"
#include "threadpool.h"
#include "walker.h"
#include "db.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <pwd.h>
#include <sys/mman.h>
#include <time.h>
#include <dirent.h>

extern struct config walkerconf[];
extern int epollfd;
extern DBHANDLE walker_db;


#define MAXLINE     65535
#define LINE        1024
#define MAXURL      4096
#define MAXQUERY    4096
#define SERVER      "Walker"




char postdata[MAXLINE];

void http_handle(int client, threadpool_t *tpool, int epollfd)
{
    http_analyze(client, tpool);
    epoll_ctl(epollfd, EPOLL_CTL_DEL, client, NULL);
    //close(client);
    if (strcasecmp(walkerconf[THREADNUMBER].value, "0") == 0)
        close(client);
}




/*@ http_getline:
 *@   get a line from port, return a number read in the line.
 *@   If there is no more data in the port, it  attends a single
 *@  '\0' to the line and 0 is returned.
 *@
 */
int http_getline(int fd, char *buf, int size)
{
    char ch = '\0';
    int   n, i;
    n=i=0;

    
    while( (i<size-1) && (ch!='\n')){
        n = recv(fd, &ch, 1, 0);
        if (n>0){
            if (ch=='\r'){
                n = recv(fd, &ch, 1, MSG_PEEK);

                if (n>0 && ch=='\n')
                    recv(fd, &ch, 1, 0);
                else 
                    ch='\n';
            }
            
            if (ch != '\n'){
                buf[i]=ch;
                i++;
            }
        }else 
            ch='\n';
    }

    buf[i] = '\0';
    return i;
}






/*@ http_analyze:
 *@      get request method, quering, url, http version, etc.
 *@
 *@
 *@
 */
 /*
int http_analy(int fd)
{
    char    buf[1024];
    int     numchars;
    char    method[255];
    char    url[255];
    char    path[255];
    int     i,j;
    struct stat statbuf;
    int cgi = 0;
    char * query;

    numchars = http_getline(fd, buf, sizeof(buf));
    i=j=0;

    while(!isspace(buf[j]) && (i<(sizeof(method)-1))){
        method[i] = buf[j];
        i++; j++;
    }

    method[i] = '\0';

    while(!isspace(buf[i]) && (i<(sizeof(url)-1)) && (j<sizeof(buf))){
        url[i] = buf[j];
        i++; j++;
    }

    url[i]='\0';
   
    char *filepath;
    if (strcasecmp(method, "GET")==0){

        query = url;
        while((*query != '?') && (*query != '\0'))
            query ++;
        
        if (*query == '?'){
            cgi = 1;
            *query = '\0';
            query++;
        }
    

        if (url[0] != '/'){
                http_not_found(fd);
        }else{
            filepath =(char*)malloc(strlen(url)+strlen(walkerconf[ROOT].value)+1);
            strcpy(filepath, walkerconf[ROOT].value);
            strcat(filepath, url);
        } 


        if (stat(filepath, &statbuf) == -1){
            while ((numchars > 0) && strcmp("\n", buf))
                numchars = http_getline(fd, buf, sizeof(buf));
            http_not_found(fd);
        }else{
            if ((statbuf.st_mode & S_IFMT ) == S_IFDIR)
                strcat(path, "/index.html");

            if ((statbuf.st_mode & S_IXUSR) ||
                (statbuf.st_mode & S_IXGRP) ||
                (statbuf.st_mode & S_IXOTH)  )
                http_send_file(fd, filepath);
        }
    }
}
*/

/*@http_analyze:
 *@    http_analy* version 2
 *@
 *@
 *@
 *@
 */

int http_analyze(int cfd, threadpool_t *tpool)
{
    char method[20],version[20], url[MAXURL], query[MAXQUERY], line[MAXLINE], *pos;  
    int ret=0, iscgi=0;

    if ((ret = http_getline(cfd, line, MAXLINE))==0){
        struct sockaddr_in saddr;
        socklen_t len;
        getsockname(cfd, (struct sockaddr*)&saddr, &len);
        char buf[100];
        sprintf (buf, "%s ip:%s create an error connection", __FUNCTION__, inet_ntoa(saddr.sin_addr));
        errlog(buf);
        return -1;
    }
    
    sscanf(line, "%s %s %s", method, url, version);
    
    if (strncasecmp(method, "GET", 3) !=0 && strncasecmp(method, "POST", 4) !=0){
        http_not_support(cfd);
        return 0;
    }

    pos = url;
    while(*pos != '?' && *pos != '\0')
       pos++;
    if (*pos == '?'){
        iscgi=1;
        *pos='\0'; pos++;
        strncpy(query, pos, strlen(pos)+1);
    } 
    
   
    struct sockaddr_in addr;
    socklen_t socklen;
    getsockname(cfd, (struct sockaddr*)&addr, &socklen);
    
    time_t t = time(NULL);
    char* tmpbuf = (char*)malloc(50);
    sprintf (tmpbuf, "%s----", ctime(&t));
    sprintf (tmpbuf, "%sIP:%s", tmpbuf, inet_ntoa(addr.sin_addr));
    
    db_store(walker_db, (const char*)tmpbuf, (const char*)url, DB_INSERT);
    
    free(tmpbuf);
    
    

    if (strncasecmp(method, "GET", 3)==0){
        char *filepath;
        filepath = (char *)malloc(strlen(walkerconf[ROOT].value)+strlen(url)+1);
        strcpy(filepath, walkerconf[ROOT].value);
        strcat(filepath, url);


        while((ret = http_getline(cfd, line, MAXLINE))>0);


        struct stat st;
        if (stat(filepath, &st) == -1){
            http_not_found(cfd);
            return 0;
        }

        if (access(filepath, R_OK)){
            http_error(cfd);
            return 0;
        }

        if ((S_ISDIR(st.st_mode) || S_ISREG(st.st_mode)) && !iscgi){ 
            struct http_job* job = http_make_job(cfd, filepath);
            //threadpool_add(tpool, http_thread_work, (void*)job, 0);
            http_thread_work((void*)job);
            return 0;
        }else if (S_ISREG(st.st_mode) && iscgi){
            
        }else{
            http_error(cfd);
            return 0;
        }         
    }else if (strncasecmp(method, "POST", 4) == 0){
        int pi[2], pid, content_length;
        char *p; 
        char *filepath;

        filepath = (char *)malloc(strlen(walkerconf[ROOT].value)+strlen(url)+1);
        strcpy(filepath, walkerconf[ROOT].value);
        strcat(filepath, url);
        content_length=-1;
        
        while((ret=http_getline(cfd, line, MAXLINE)) !=0){
            if (strncasecmp(line, "Content-Length:", 15) == 0){
                p = &line[15];
                p += strspn(p, " \t");
                content_length = atoi(p);
            }
                                       
        }

        if (content_length != -1){
            recv(cfd, postdata, content_length, 0);   
        }

        socketpair(AF_UNIX, SOCK_STREAM, 0, pi);
        
        pid = fork();

        if (pid==0){
            dup2(pi[0], 0);
            dup2(pi[1], 1);

            close(pi[0]);
            close(pi[1]);

            char meth_env[255];
            char length_env[255];

            sprintf (meth_env, "REQUEST_METHOD=%s", method);
            putenv(meth_env);
            
            sprintf (length_env, "CONTENT_LENGTH=%d", content_length);
            putenv(length_env);

            execl("/usr/bin/php-cgi", "php-cgi", filepath, NULL);
            exit(0);
        }else{
            char recvdata[MAXLINE], packet[MAXLINE];
            write(pi[0], postdata, content_length);

            if ((ret = read(pi[0], recvdata, MAXLINE))>0){
                recvdata[ret]='\0';
                
                
                
                //sprintf(packet, "%sContent-Type: text/html\r\n", packet);
                sprintf(packet, "HTTP/1.1 200 OK\r\n");
                sprintf(packet, "%sContent-Length: %d\r\n\r\n", packet, ret);
                sprintf(packet, "%s%s", packet, recvdata);
                send(cfd, packet, MAXLINE, 0);
                 
                //send(cfd, recvdata, MAXLINE, 0);
            }
                
            close(pi[0]);
            close(pi[1]);
            free(filepath);
            waitpid(pid, NULL, 0);
        }
            
        
    }
    
    return 0;
}





/*@ http_make_job:
 *@     init a struct http_job with fd and filepath. This struct
 *@     is used by threadpool_add.
 *@
 */
struct http_job* http_make_job(int fd, char *filepath)
{
    struct http_job * job = (struct http_job*)malloc(sizeof (struct http_job));
    job->fd = fd;
    job->filepath = (char *)malloc(strlen(filepath)+1);
    strcpy(job->filepath, filepath);
    return job;
}



/*@ http_thread_work:
 *@             
 *@
 *@
 *@
 */

void http_thread_work(void *arg)
{
    struct http_job *job = arg;
    struct stat st;

    if (stat(job->filepath, &st) == -1){
        http_not_found(job->fd);
        return;
    }

    if (S_ISDIR(st.st_mode)){
        http_show_dir(job->fd, job->filepath);
    }
    
    if (S_ISREG(st.st_mode)){
        http_send_file(job->fd, job->filepath);
    } 

    if (strcasecmp(walkerconf[THREADNUMBER].value, "0")!=0)
        close(job->fd);


    free(job);
    return ;

}





/*@ http_not_support
 *@ http_not_found
 *@ http_error
 */
 
int http_not_support(int client)
{
   // http_send(client, "HTTP/1.1 400 NoFoundation\r\n");
    //http_send(client, "Content-Type: text/html\r\n");
    //http_send(client, "\r\n");
    //http_send(client, "<HTML><TITLE>NOT SUPPORT</TITLE>");
    //http_send(client, "<BODY>Walker server do not support the mothod you request.</BODY></HTML>");

    http_send(client, "HTTP/1.1 400 NoFoundation\r\n"
                      "Content-type: text/html\r\n"
                      "\r\n"
                      "<HTML><TITLE>Not Support</TITLE>"
                      "<BODY>Walker server do not support the method you request.</BODY></HTML>");
    
    return 0;

}


int http_not_found(int client)
{
    //http_send(client, "HTTP/1.1 404 NotFoundation\r\n");
    //http_send(client, "Content-Type: text/html\r\n");
    //http_send(client, "\r\n");
    //http_send(client, "<HTML><TITLE>Not Foundation</TITLE>");
    //http_send(client, "<BODY><p>The server could not fullfill your request.");
    //http_send(client, "Because the resource specified is unavailable or not exit");
    //http_send(client, "</p></BODY></HTML>");
    
    http_send(client, "HTTP/1.1 404 Not Found\r\n"
                      "Content-type: text/html\r\n"
                      "\r\n"
                      "<HTML><TITLE>Not Found</TITLE>"
                      "<BODY>File requested isn't found on the server</BODY></HTML>");
    
    return 0;
}


int http_error(int client)
{
    //http_send(client, "HTTP/1.1 Bad Request\r\n");
    //http_send(client, "Content-type: text/html\r\n");
    //http_send(client, "\r\n");
    //http_send(client, "<HTML><TITLE>Bad Request</TITLE>");
    //http_send(client, "<BODY>The resource can't be retrived</BODY></HTML>");
    
    http_send(client, "HTTP/1.1 400 Bad Request\r\n"
                      "Content-Type: text/html\r\n"
                      "\r\n"
                      "<HTML><TITLE>Bad Request</TITLE>"
                      "<BODY>The resource can't be retrived</BODY></HTML>");

    return 0;
}





void http_mk_header(int client, int status, char *phrase)
{
    char buf[MAXLINE];
    sprintf(buf, "HTTP/1.1 %d %s\r\n", status, phrase);
    http_send(client, buf);
    http_send(client, "Content-type: text/html\r\n");
    http_send(client, "\r\n");
}


int http_send(int client, char *s)
{
    return send(client, s, strlen(s), 0);  
}

int http_getfiletype(char *filename, char *filetype)
{
    if (strstr(filename, ".html"))
        strcpy(filetype, "text/html");
    else if (strstr(filename, ".gif"))
        strcpy(filetype, "image/gif");
    else if (strstr(filename, ".jpg"))
        strcpy(filetype, "image/jpeg");
    else if (strstr(filename, "png"))
        strcpy(filetype, "image/png");
    else
        strcpy(filetype, "text/plain");

    return 0;
        
    
}

int http_send_file(int client, char *filepath)
{
    char buf[MAXLINE];
    char file[MAXLINE];
    char filetype[20];
    FILE *f = fopen(filepath, "r");
    struct stat st;
    stat(filepath, &st);
   
    /*
    fgets(buf, MAXLINE, f);
    sprintf (file, "%s", buf);
    while(!feof(f)){
        fgets(buf, MAXLINE, f);
        sprintf(file, "%s%s", file, buf);
    }
    */
    
    
    http_getfiletype(filepath, filetype);
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    sprintf (buf, "%sContent-type: %s\r\n",buf, filetype);
    sprintf (buf, "%sContent-length: %ld\r\n\r\n",buf, st.st_size);
    http_send(client, buf);

    char *src=(char*)mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, fileno(f), 0);
    fclose(f);
    send(client, src, st.st_size,0);
    munmap(src, st.st_size);
    return 0;
}

int http_show_dir(int client, char *filepath)
{
    DIR *dp;
    struct dirent *dirp; struct stat st;
    struct passwd *filepasswd;
    int num = 1;
    char files[MAXLINE], buf[MAXLINE], name[LINE], img[LINE], mtime[LINE], dir[LINE];
    char *p;

    p = strrchr(filepath, '/');
    ++p;
    strcpy(dir, p);
    strcat(dir, "/");

    if ((dp =opendir(filepath))== NULL){
        http_error(client);
        return -1;
    }

    sprintf (files, "<HTML><TITLE>Dir Browser</TITLE>");
    sprintf (files, "%s<style type = ""text/css""> a:link{text-decoration:none;} </style>", files);
    sprintf (files, "%s<body bgcolor=""ffffff"" font-family=Arial color=#fff font-size=14px}\r\n", files);

    while ((dirp=readdir(dp))!=NULL){
        if (strcmp(dirp->d_name, ".") == 0 || strcmp(dirp->d_name, "..")==0)
            continue;

        sprintf (name, "%s/%s", filepath, dirp->d_name);
        stat(name, &st);
        filepasswd = getpwuid(st.st_uid);

        if (S_ISDIR(st.st_mode))
            sprintf (img, "<img src=""dir.png"" width=""24px"" height=""24px"">");
        else if(S_ISFIFO(st.st_mode))
            sprintf (img, "<img src=""fifo.png"" width=""24px"" height=""24px"">");
        else if (S_ISLNK(st.st_mode))
            sprintf (img, "<img src=""link.png"" width=""24px"" height=""24px"">");
        else if (S_ISSOCK(st.st_mode))
            sprintf (img, "<img src=""sock.png"" width=""24px"" height=""24px"">");
        else 
            sprintf (img, "<img src=""file.png"" width=""24px"" height=""24px"">");
    

        sprintf (files, "%s<p><pre>%-2d %s ""<a href=%s%s"">%-15s</a>%-10s%10d %24s</pre></p>\r\n", files, num++, img, dir, dirp->d_name, dirp->d_name, filepasswd->pw_name, (int)st.st_size, tmmodify(st.st_mode, mtime));
    }
    closedir(dp);
    sprintf (files, "%s</BODY></HTML>", files);
       
    sprintf (buf, "HTTP/1.0 200 OK \r\n");
    sprintf (buf, "%sServer: "SERVER"\r\n", buf);
    sprintf (buf, "%sContent-length: %ld\r\n", buf, strlen(files));
    sprintf (buf, "%sContent-type: text/html\r\n", buf);
    sprintf (buf, "%s\r\n", buf);

    http_send(client, buf);
    http_send(client, files);

    return 0; 

}




