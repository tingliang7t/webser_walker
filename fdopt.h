#ifndef _FDOPT_H
#define _FDOPT_H

#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>

int set_nonblock(int); 

int set_port_reused(int, void *, socklen_t);





#endif
