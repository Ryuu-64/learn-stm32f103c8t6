#include <stdio.h>
#include <stdarg.h>

void Error_Handler_UART(const char *file, const int line, const char *format, ...) {
    va_list args;
    va_start(args, format);
    printf("Error occurred in file %s at line %d:\n", file, line);
    vprintf(format, args);
    va_end(args);
}
