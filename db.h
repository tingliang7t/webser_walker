#ifndef _APUE_DB_H
#define _APUE_DB_H


#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


typedef     void * DBHANDLE;

DBHANDLE db_open(const char*, int, ...);
void     db_close(DBHANDLE);
char    *db_fetch(DBHANDLE, const char*);
int      db_store(DBHANDLE, const char*, const char*, int);
int      db_delete(DBHANDLE, const char*);
void     db_rewind(DBHANDLE);
char    *db_nextrec(DBHANDLE, char*);

#define IDXLEN_SZ     4
#define SEP           ':'
#define SPACE         ' '
#define NEWLINE       '\n'


#define PTR_SZ      7
#define PTR_MAX     9999999
#define NHASH_DEF   137
#define FREE_OFF    0
#define HASH_OFF    PTR_SZ

//typedef unsigned int size_t;

typedef unsigned long DBHASH; /*hash value*/
typedef unsigned long   COUNT;

typedef struct{
    int         idxfd;
    int         datfd;
    char        *idxbuf;
    char        *datbuf;
    char        *name;
    
    off_t       idxoff;
    size_t      idxlen;

    off_t       datoff;
    size_t      datlen;
    
    off_t       ptrval;
    off_t       ptroff;
    off_t       chainoff;
    off_t       hashoff;
    DBHASH      nhash;
    COUNT       cnt_delok;
    COUNT       cnt_delerr;
    COUNT       cnt_fetchok;
    COUNT       cnt_fetcherr;
    COUNT       cnt_nextrec;
    COUNT       cnt_stor1;
    COUNT       cnt_stor2;
    COUNT       cnt_stor3;
    COUNT       cnt_stor4;
    COUNT       cnt_storerr;
}DB;
/*
 *db_store()第三个参数的类型
 */

#define DB_INSERT   1
#define DB_REPLACE  2
#define DB_STORE    3
/*
 * end
 */


#define IDXLEN_MIN  6
#define IDXLEN_MAX  1024
#define DATLEN_MIN  2
#define DATLEN_MAX  2014

#endif


