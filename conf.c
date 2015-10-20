#include "conf.h"

#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <ctype.h>

#define MAXLEN 4096

char * opt_s = "m:r:c:p:l:n:t:h:";
struct config walkerconf[] = {  {"mutiprocess",      "off"},
                                {"processnumber",    "3"},
                                {"root"       ,      "/var/www"},
                                {"cgiroot"    ,      "/var/www/cig-bin"},
                                {"defaultfile",      "index.html"},
                                {"port"       ,      "80"},
                                {"listennumber",     "5"},
                                {"load"       ,      "7"},
                                {"threadnumber",    "4"},
                                {"timeout",          "40"},
                                {"logfile",          "err.log"},
                                {NULL,               0} 
                            };

int init_file_config()
{
    char *fpath = CONFILEPATH;

    if (access(fpath, F_OK | R_OK)){
        if (errno == EACCES)
            printf ("%s:Permission denied to config file\n",__FUNCTION__);
        if (errno == ENOENT)
            printf ("%s:Config file path error\n", __FUNCTION__);

        exit(1);
    }
    
    /*
       if (stat(fpath, &statbuf) == -1){
        printf ("%s: get file stat error\n", __FUNCTION__);
        exit(1);     
    }
    */

    FILE *cfile = fopen(fpath, "r");
    if (!cfile){
        printf ("%s:config file open error\n", __FUNCTION__);
        exit(1);
    }

    char *start, *pos, name[20], value[50];

    char line[MAXLEN];
    memset(line, '\0', MAXLEN);
    while (getaline(cfile, line, MAXLEN) != 0){
        
        // Get the config option name
        pos = line; 
        while(isspace(*pos))
            pos++;
        
        if (*pos == '#' || *pos == '\n'){
            memset(line, '\0', MAXLEN);
            continue;
        }

        start = pos;
        while(*pos != '=')
            pos++;

        *pos='\0';
        strncpy(name, start, 20);
        
        //end get name
        
        /*pos++;
          Get the config value
          start = pos;
          while(*pos != '\n')
            pos++;
          pos='\0';
        */
        pos++;
        start=pos;
        strncpy(value, start, 50);
        //end get value
        
        for (int i=0; walkerconf[i].name != NULL; i++){
            if (strcmp(name, walkerconf[i].name) == 0){
                strcpy(walkerconf[i].value, value);
                break;
            }
        }

        memset(line, '\0', MAXLEN);

    }

    return 0; 
}


int getaline(FILE *file, char *buf, int size)
{
    int nread, ret;
    
    nread=0;
    while((ret = fgetc(file)) != '\n' && ret != EOF && nread < size){
        buf[nread] = ret;
        nread++;
    }
    buf[nread]='\0';
    
    if (ret == '\n')
        return 1;

    return nread;
}



int init_opt_config(int argc, char *argv[])
{
    int c, len;

    while((c = getopt(argc, argv, opt_s)) != -1){
        enum confname name = NONE;
        switch (c){
            case 'm':
                name = MUTIPROCESS;
                break;
            case 'r':
                name = ROOT;
                break;
            case 'c':
                name = CGIROOT;
                break;
            case 'p':
                name = PORT;
                break;
            case 'l':
                name = LOAD;
                break;
            case 'n':
                name = PROCESSNUMBER;
                break;
            case 't':
                name = TIMEOUT;
                break;
            case 'h':
                name = THREADNUMBER;
                break;
            default:
                break;

                        
        } 

        len = strlen(optarg);
        strncpy(walkerconf[name].value, optarg, len+1);
    }  

    return 0;
}



// test code
/*
int main (int argc, char *argv[])
{
    init_file_config();
    init_opt_config(argc, argv);
    for (int i=0; walkerconf[i].name != NULL; i++){
        printf ("%s ---- %s\n", walkerconf[i].name, walkerconf[i].value);
    }

    return 0;
    
}*/



