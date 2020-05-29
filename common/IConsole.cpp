#include "common/IConsole.h"
#include <cstdarg>
#include <cstring>
#include <Windows.h>

IConsole::IConsole()
{
	AllocConsole();
	
	SetConsoleTitle("Console");
	
	inputHandle = GetStdHandle(STD_INPUT_HANDLE);
	outputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	
	ASSERT_STR(inputHandle, "IConsole: couldn't get input handle");
	ASSERT_STR(outputHandle, "IConsole: couldn't get output handle");
	
	SetConsoleMode(inputHandle, ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT);
	SetConsoleMode(outputHandle, ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT);
}

IConsole::~IConsole()
{
	
}

/**
 *	Writes a string to the console
 */
void IConsole::Write(char * buf)
{
	UInt32	charsWritten;
	
	WriteConsole(outputHandle, buf, std::strlen(buf), &charsWritten, NULL);
}

/**
 *	Writes a formatted string to the console
 *	
 *	You may specify a temp buffer to use for the formatted text, but if NULL
 *	is used a local buffer will be provided.
 *	
 *	@param buf a temporary buffer, or NULL to use the internal buffer
 *	@param fmt the format string
 */
void IConsole::Write(char * buf, UInt32 bufLen, const char * fmt, ...)
{
	static char	tempBuf[4096];
	
	if(!buf)
	{
		buf = tempBuf;
		bufLen = sizeof(tempBuf);
	}
	
	va_list	args;
	
	va_start(args, fmt);
	vsprintf_s(buf, bufLen, fmt, args);
	va_end(args);
	
	Write(buf);
}

/**
 *	Reads a single character from the console
 */
char IConsole::ReadChar(void)
{
	char	data;
	UInt32	charsRead;
	
	ReadConsole(inputHandle, &data, 1, &charsRead, NULL);
	
	return data;
}

/**
 *	Reads a newline-terminated string from the console
 *	
 *	@param buf output buffer
 *	@param len buffer size
 *	@return number of characters read
 */
UInt32 IConsole::ReadBuf(char * buf, UInt32 len)
{
	UInt32	charsRead;
	
	buf[0] = 0;
	
	do
	{
		ReadConsole(inputHandle, buf, len, &charsRead, NULL);
	}
	while(!charsRead);
	
	int done = 0;
	for(UInt32 i = charsRead - 1; (i > 0) && !done; i--)
	{
		switch(buf[i])
		{
			case 0x0A:
			case 0x0D:
				buf[i] = 0;
				break;
				
			default:
				done = 1;
				break;
		}
	}
	
	buf[charsRead] = 0;
	
	return charsRead;
}
