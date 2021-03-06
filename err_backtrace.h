#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <execinfo.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#define PRINT_DEBUG
static void print_reason(int sig, siginfo_t * info, void *secret)
{
void *array[10];
size_t size;
#ifdef PRINT_DEBUG
char **strings;
size_t i;
size = backtrace(array, 10);
strings = backtrace_symbols(array, size);
fprintf(stderr,"Obtained %zd stack frames.\n", size);
for (i = 0; i < size; i++)
fprintf(stderr,"%s\n", strings[i]);
//	system(ifconfig|grep "inet addr:"|grep -v "127.0.0.1"|cut -d: -f2|awk '{print $1}');
freopen("out.log","w",stdout);
system("ifconfig");
 freopen("/dev/tty","w",stdout);
free(strings);
#else
int fd = open("err.log", O_CREAT | O_WRONLY);
size = backtrace(array, 10);
backtrace_symbols_fd(array, size, fd);
close(fd);
#endif
fflush(NULL);
exit(0);
}
