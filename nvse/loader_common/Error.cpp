#include "Error.h"

void PrintLoaderError(const char * fmt, ...)
{
	va_list	args;

	va_start(args, fmt);

	gLog.Log(IDebugLog::kLevel_FatalError, fmt, args);

	char	buf[4096];
	vsprintf_s(buf, sizeof(buf), fmt, args);

	MessageBox(NULL, buf, "NVSE Loader", MB_OK | MB_ICONEXCLAMATION);

	va_end(args);
}
