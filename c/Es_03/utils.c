#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

ssize_t write_all(int fd, const void *vptr, size_t n)
{
	size_t to_copy;
	ssize_t cc;
	const uint8_t *ptr;

	ptr = vptr;
	to_copy = n;
	while (to_copy > 0) {
		if ((cc = write(fd, ptr, to_copy)) <= 0) {
			if (cc < 0 && errno == EINTR)
				cc = 0;
			else
				return -1;
		}

		to_copy -= cc;
		ptr += cc;
	}
	return n;
}
