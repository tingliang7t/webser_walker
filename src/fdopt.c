#include "fdopt.h"

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/types.h>


int set_nonblock(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);

    return old_option;
}


int set_port_reused(int fd, void *addr, socklen_t len)
{
   return setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, addr, len); 
}
