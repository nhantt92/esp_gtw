/*
 * util.c
 *
 *  Created on: Sep 5, 2015
 *      Author: lieven
 */
#include "Sys.h"

const char* space = "                                 ";

void IROM strAlign(char *dst, int dstLength, char* src, int srcLength) {
	int copyLength = (srcLength < dstLength) ? srcLength : dstLength;
	strncat(dst, src, copyLength);
	strncat(dst, space, dstLength - copyLength);
}
#include <stdarg.h>
char lastLog[256];
uint32_t conflicts=0;

const char* SysLogLevelStr[] = { "TRACE", "DEBUG", "INFO", "WARN", "ERROR",
		"FATAL" };
#include "mutex.h"
mutex_t logMutex=1;
void IROM SysLog(SysLogLevel level, const char* file, const char* function,
		const char * format, ...) {
	if (! GetMutex(&logMutex)) {
		conflicts++;
		return;

	}
	uint32_t time = system_get_time() / 1000;

	char buffer[256];
	va_list args;
	va_start(args, format);
	ets_vsnprintf(buffer, 256, format, args);
	va_end(args);

	char dst[40];
	dst[0] = '\0';
	strAlign(dst, 18, file, strlen(file));
	strAlign(&dst[18], 18, function, strlen(function));

	if (level < LOG_INFO) { // put log in mqtt buffer
		ets_sprintf(lastLog, "%s:%s:%s", SysLogLevelStr[level], dst, buffer);
	}

	ets_printf("%06d.%03d |%s| %s | %s\r\n", time / 1000, time % 1000,
			SysLogLevelStr[level], dst, buffer);
	ReleaseMutex(&logMutex);
}
