#ifndef _FLOCK_H
#define _FLOCK_H

#include <fcntl.h>
#include <sys/types.h>

int set_lock(int, int, int, off_t, int, off_t);

int set_lock(int fd, int cmd, int type, off_t offset, int whence, off_t len)
{
    struct flock        lock;
    lock.l_type = type;
    lock.l_start = offset;
    lock.l_whence = whence;
    lock.l_len = len;
    
    return fcntl(fd, cmd, &lock);
}  

#define read_lock(fd, offset, whence, len) \
        set_lock(fd, F_SETLK, F_RDLCK, offset, whence, len)

#define readw_lock(fd, offset, whence, len) \
        set_lock(fd, F_SETLKW, F_RDLCK, offset, whence, len)

#define write_lock(fd, offset, whence, len) \
        set_lock(fd, F_SETLK, F_WRLCK, offset, whence, len)

#define writew_lock(fd, offset, whence, len) \
        set_lock(fd, F_SETLKW, F_WRLCK, offset, whence, len)

#define un_lock(fd, offset, whence, len) \
        set_lock(fd, F_SETLK, F_UNLCK, offset, whence, len)


#endif
