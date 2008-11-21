/*
 * fadvutils/cat.c
 *
 * Author: Shapor Naghibzadeh <shapor@shapor.com>
 * Description: Read a file from stdin, write to stderr, evicting data from
 *   buffer cache via posix_fadvise().  Originally written to make bashgal
 *   (http://shapor.com/bashgal) less invasive to the system.
 *
 * FIXME TODO's:
 * - support non-Linux OS's (which support POSIX_FADV_DONTNEED?)
 * - support some other cat features?
 */

#include <stdio.h>
#include <unistd.h> /* for read/write */
#include <string.h> /* for strerror() */
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h> /* for posix_fadvise */
#include <linux/fadvise.h> /* for POSIX_FADV_DONTNEED */

#define PAGE_SIZE	4096

int main(int argc, char **argv) {
	size_t bytes_read, bytes_written, chunk_start = 0;
	size_t chunk_size = PAGE_SIZE; /* TODO make configurable */
	char buf[chunk_size];
	int fadv_error = 0, verbose = 1;

	while((bytes_read = read(0, &buf[0], chunk_size))) {
		if (bytes_read == -1) {
			if (errno == EINTR)
				continue;
			else {
				fprintf(stderr, "read() returned error: %s (%i)\n", strerror(errno), errno);
				exit(1);
			}
		}

		/* write buffer to stdout */
		size_t offset = 0;
		do
			while ((bytes_written = write(1, &buf[offset], bytes_read)) > 0) {
				offset += bytes_written;
				if (offset == bytes_read) /* write complete */
					break;
			}
		while ((bytes_written == -1) && (errno == EINTR));

		/* evict data read from buffer cache */
		if (!fadv_error && posix_fadvise(0, chunk_start, bytes_read, POSIX_FADV_DONTNEED))
			fadv_error = errno; /* don't keep trying if we error */

		if (bytes_written < 0) {
			fprintf(stderr, "write() returned error: %s (%i)\n", strerror(errno), errno);
			exit(2);
		}
	}
	if (verbose && fadv_error)
		fprintf(stderr, "warning: cache may not be evicted, posix_fadvise() returned error: %s (%i)\n", strerror(errno), errno);
}
