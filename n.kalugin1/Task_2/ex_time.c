#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(void)
{
    time_t current_time;
    struct tm *california_time;
    char timezone_name[16];

#ifdef _WIN32
    // Windows CRT понимает зону в формате PST8PDT.
    if (_putenv_s("TZ", "PST8PDT") != 0)
    {
        fprintf(stderr, "Unable to set TZ\n");
        return 1;
    }
    _tzset();
#else
    // В Unix задаём часовую зону по имени из базы IANA.
    if (setenv("TZ", "America/Los_Angeles", 1) == -1)
    {
        perror("setenv");
        return 1;
    }
    tzset();
#endif

    current_time = time(NULL);
    if (current_time == (time_t)-1)
    {
        perror("time");
        return 1;
    }

    california_time = localtime(&current_time);
    if (california_time == NULL)
    {
        perror("localtime");
        return 1;
    }

    // Сокращение PST/PDT зависит от даты и летнего времени.
    if (strftime(timezone_name, sizeof(timezone_name), "%Z", california_time) == 0)
    {
        fprintf(stderr, "Unable to format timezone name\n");
        return 1;
    }

    // В tm месяц начинается с нуля, а год отсчитывается от 1900.
    printf("%02d/%02d/%04d %02d:%02d:%02d %s\n",
           california_time->tm_mon + 1,
           california_time->tm_mday,
           california_time->tm_year + 1900,
           california_time->tm_hour,
           california_time->tm_min,
           california_time->tm_sec,
           timezone_name);

    return 0;
}