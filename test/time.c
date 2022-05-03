#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int main() 
{
	time_t cur_tm = time(NULL);
	char tm_buff[25] = {0,};
	ctime_r(&cur_tm, tm_buff);
	tm_buff[24] = '\0';
	fprintf(stderr, "[%s]\n", tm_buff);
}
