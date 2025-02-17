#ifndef SNTP_TIME_MODULE_H
#define SNTP_TIME_MODULE_H

#include <time.h>
#include <sys/time.h>

void sync_time(void);
void get_current_time(struct tm *timeinfo);
void initialize_sntp(void); // Make initialize_sntp static

#endif // SNTP_TIME_MODULE_H