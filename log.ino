
#include <stdarg.h>
#include <stdio.h>

#include <log.h>

static char tmp_log[256];

void log( const char* fmt, ... ) {
	va_list arg;
	va_start( arg, fmt );
	vsnprintf( tmp_log, sizeof(tmp_log), fmt, arg );
	va_end( arg );
	Serial.println(tmp_log);
}

