#include <stdio.h>        // printf
#include <time.h>         // time_t, struct tm, time(), localtime()
#include <stdlib.h>       // setenv()

int main(void)
{
	setenv("TZ", "America/Los_Angeles", 1);
	tzset();

	time_t now;
	time(&now);
	
	struct tm *sp;
	sp = localtime(&now);
	
	if (sp == NULL){
		printf("Failed to convert time\n");
		return 1;
	}

	printf("%02d/%02d/%04d %02d:%02d %s\n", sp->tm_mon + 1, sp->tm_mday, sp->tm_year + 1900, sp->tm_hour, 
	sp->tm_min, tzname[sp->tm_isdst]);
	
	return 0;
}

