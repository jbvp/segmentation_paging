#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

int main(int argc, char *argv[])
{
	unsigned long localvar = 0x1337133713371337;
	unsigned long addr = (unsigned long)&localvar;
	int fd;

	printf("localvar = 0x%lx\n", localvar);
	printf("&localvar = 0x%lx\n", addr);

	fd = open("/dev/paging", O_WRONLY);
	if (fd < 0) {
		perror("Error opening /dev/paging");
		return errno;
	}

	write(fd, &addr, sizeof(unsigned long));

	close(fd);

	return 0;
}
