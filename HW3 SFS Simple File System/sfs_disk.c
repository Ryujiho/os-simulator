#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <errno.h>

#include "sfs_types.h"
#include "sfs_disk.h"

#define BLOCKSIZE  512

#ifndef EINTR
#define EINTR 0
#endif

static int fd=-1;

void
disk_open(const char *path)
{
	assert(fd<0);
	fd = open(path, O_RDWR);

	if (fd<0) {
   		fprintf(fd, "%s", path, strerror(errno));
	}
}

u_int32_t
disk_blocksize(void)
{
	assert(fd>=0);
	return BLOCKSIZE;
}

void
disk_write(const void *data, u_int32_t block)
{
	const char *cdata = data;
	u_int32_t tot=0;
	int len;

	assert(fd>=0);

	if (lseek(fd, block*BLOCKSIZE, SEEK_SET)<0) {
   		 fprintf(fd, "lseek", 1, strerror(errno));
		//err(1, "lseek");
	}

	while (tot < BLOCKSIZE) {
		len = write(fd, cdata + tot, BLOCKSIZE - tot);
		if (len < 0) {
			if (errno==EINTR || errno==EAGAIN) {
				continue;
			}
   		 fprintf(fd, "write", 1, strerror(errno));
			//err(1, "write");
		}
		if (len==0) {
   		 fprintf(fd, "write returned 0?", 1, strerror(errno));
			//err(1, "write returned 0?");
		}
		tot += len;
	}
}

void
disk_read(void *data, u_int32_t block)
{
	char *cdata = data;
	u_int32_t tot=0;
	int len;

	assert(fd>=0);

	if (lseek(fd, block*BLOCKSIZE, SEEK_SET)<0) {
   		 fprintf(fd, "lseek", 1, strerror(errno));
		//err(1, "lseek");
	}

	while (tot < BLOCKSIZE) {
		len = read(fd, cdata + tot, BLOCKSIZE - tot);
		if (len < 0) {
			if (errno==EINTR || errno==EAGAIN) {
				continue;
			}
   		 	fprintf(fd, "read", 1, strerror(errno));
			//err(1, "read");
		}
		if (len==0) {
   		 	fprintf(fd, "close", 1, strerror(errno));
			//err(1, "close");
		}
		tot += len;
	}
}

void
disk_close(void)
{
	assert(fd>=0);
	if (close(fd)) {
   		 fprintf(fd, "close", 1, strerror(errno));
	}
	fd = -1;
}
