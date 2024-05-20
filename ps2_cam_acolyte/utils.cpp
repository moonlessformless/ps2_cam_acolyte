#include "utils.h"
#include <iostream>
#include <cstdarg>
#include <windows.h>

void error::fatal_error(const char* str, ...)
{
    static char s_printf_buf[1024];
    va_list args;
    va_start(args, str);
    vsnprintf(s_printf_buf, sizeof(s_printf_buf), str, args);
    va_end(args);
    OutputDebugStringA(s_printf_buf);
    OutputDebugStringA("\n");

	exit(1);
}