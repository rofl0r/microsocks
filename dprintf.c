#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>

int dprintf(int fd, const char *fmt, ...)
{
	char stack_buffer[256];
	char *buf = stack_buffer;
	int len = sizeof(stack_buffer);
	va_list ap;
	int ret = -1;

	va_start(ap, fmt);
	
	// Method 1 try to just print it to stack if possible
	ret = vsnprintf(buf, len, fmt, ap);

	if (ret >= len) {
		// Method 2 dynamically allocate buffer
		len = ret + 1;
		buf = malloc(len);
		if (buf) {
			va_end(ap);
			va_start(ap, fmt);
			ret = vsnprintf(buf, len, fmt, ap);
		} else {
			ret = -1; // Malloc failed due to too little memory??? Couldn't be me
		}
	}
	if (ret >= 0 && (write(fd, buf, ret) != ret)) {
		ret = -1;
	}
	if (buf != stack_buffer) free(buf);
	va_end(ap);
	return ret;
}
