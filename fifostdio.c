#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdbool.h>

struct termios old_tio, new_tio;
char *out_path;

void restore(int s)
{
	tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
	exit(0);
}

int main(int argc, char *argv[])
{
	int outfd;
	char c;
	int status;

	if(argc != 2) {
		printf("usage: %s fifo\n", argv[0]);
		return -1;
	}

	out_path = argv[1];

	tcgetattr(STDIN_FILENO,&old_tio);
	new_tio=old_tio;
	new_tio.c_lflag &=(~ICANON & ~ECHO);
	tcsetattr(STDIN_FILENO,TCSANOW,&new_tio);

	outfd = open(out_path, O_WRONLY);
	if(outfd < 0) {
		printf("error opening %s: %s (errno=%d)\n",
		       out_path, strerror(errno), errno);
		return -1;
	}

	signal(SIGINT, restore);

	while(1) {
		status = read(STDIN_FILENO, &c, 1);
		if(status == 1) {
			write(outfd, &c, 1);
		}
	}

	return 0;
}

