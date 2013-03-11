#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>

#include "utils.h"

int load_file(char *path, char **data, size_t *datalen)
{
	struct stat sb;
	int status;
	size_t filesize;
	size_t bytesread;
	char *buf = NULL, *cursor;
	int fd = -1;

	fd = open(path, O_RDONLY);
	if((fd = open(path, O_RDONLY)) < 0) {
		fprintf(stderr, "error opening %s: %s (errno=%d)\n",
		        path, strerror(errno), errno);
		goto error;
	}

	if(fstat(fd, &sb)) {
		fprintf(stderr, "error in stat(\"%s\"): %s (errno=%d)\n",
		        path, strerror(errno), errno);
		goto error;
	}
	filesize = sb.st_size;

	if(!(buf = malloc(filesize))) {
		fprintf(stderr, "out of memory\n");
		goto error;
	}

	cursor = buf;
	bytesread = 0;
	while(filesize) {
		status = read(fd, cursor, filesize);
		if(status < 0){
			if(errno != EINTR) {
				fprintf(stderr, "read error: %s (errno=%d)\n",
				        strerror(errno), errno);
				goto error;
			}
		}
		if(status == 0) {
			fprintf(stderr, "Warning: unexpected end of file at byte %lu\n",
			        bytesread);
			break;
		}
		filesize -= status;
		cursor += status;
		bytesread += status;
	}

	*data = buf;
	*datalen = bytesread;
	close(fd);
	return 0;

error:
	free(buf);
	if(fd >= 0) {
		close(fd);
	}
	return -1;
}
