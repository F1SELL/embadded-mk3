/* No file system or console is used by this firmware. Explicit newlib stubs. */
#include <errno.h>
int _close(int fd) { (void)fd; errno=EBADF; return -1; }
int _lseek(int fd,int offset,int whence) { (void)fd;(void)offset;(void)whence;errno=ESPIPE;return -1; }
int _read(int fd,char *buf,int count) { (void)fd;(void)buf;(void)count;errno=ENOSYS;return -1; }
int _write(int fd,char *buf,int count) { (void)fd;(void)buf;(void)count;errno=ENOSYS;return -1; }
