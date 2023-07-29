#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h> //This is needed for ssize_t
#include <unistd.h>

//int open (const char *name, int flags, mode_t mode);

int main(int a_number, char* happy[]) {
	const char *write_f = happy[1];
	const char *write_s = happy[2];
	int fd;
	int nr;
	//size_t wr_s_size;
	//w_r_size = sizeof()
	//printf("Hello world %d %s %s /n", number_of_args, write_f, write_s);
	//int open (const char *name, int flags, mode_t mode);
	openlog(NULL,0,LOG_USER);

	if (a_number < 3)
	{
		syslog(LOG_ERR, "Invalid Number of arguments: %d", a_number);
		return 1;
	}
		fd = open (write_f, O_WRONLY | O_TRUNC | O_CREAT, 0664);
		if (fd == -1)
		{
			syslog(LOG_ERR, "Could not open file");
			return -1; 
		}
		else
		{
			nr = write(fd, write_s, strlen(write_s));
			if (nr == -1)
			{
				 syslog(LOG_ERR, "Total failure to write");
				 return -1;
			}
		}
	syslog(LOG_DEBUG, "Successfully wrote %s to %s", write_f, write_s);
	close(fd);
	closelog();
return 0;
}

