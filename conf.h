#ifndef _CONFIG_H
#define _CONFIG_H

#define CONFILEPATH  "walker.conf"
#include <stdio.h>

union conf_value{
    int     n;
    char    string[50];
};


struct config{
    char                *name;
    char                value[50];
};

enum confname{MUTIPROCESS=0,PROCESSNUMBER, ROOT, CGIROOT, DEFAULTFILE, PORT,LISTENNUMBER, LOAD, THREADNUMBER, TIMEOUT, LOGFILE, NONE};

// config with config file
int init_file_config();
int getaline(FILE *, char *, int);

//config with command line
/*@ m:mutiprocess
 *@ r:root
 *@ c:cgiroot
 *@ p:port
 *@ l:load
 *@ n:processnumber
 *@ t:timeout
 *@ h:threadnumber
 */

int init_opt_config(int argc, char *argv[]);
#endif
