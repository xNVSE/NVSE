#include <share.h>
#include "common/IDebugLog.h"
#include "common/IFileStream.h"
#include <shlobj.h>

std::FILE			*IDebugLog::logFile = nullptr;
char				IDebugLog::sourceBuf[16] = { 0 };
char				IDebugLog::headerText[16] = { 0 };
char				IDebugLog::formatBuf[8192] = { 0 };
int					IDebugLog::indentLevel = 0;
int					IDebugLog::rightMargin = 0;
int					IDebugLog::cursorPos = 0;
int					IDebugLog::inBlock = 0;
bool				IDebugLog::autoFlush = true;
bool				IDebugLog::logToFolder = false;
char				IDebugLog::cPath[] = {};
IDebugLog::LogLevel	IDebugLog::logLevel = kLevel_DebugMessage;
IDebugLog::LogLevel	IDebugLog::printLevel = kLevel_Message;

IDebugLog::IDebugLog(const char *name, const bool useLogsFolder) {
	logToFolder = useLogsFolder;
	GetCurrentDirectoryA(MAX_PATH, cPath); // Find CWD (e.g. C:/Steam/SteamApps/Common/Fallout New Vegas)
	Open(name);
}

IDebugLog::~IDebugLog() { if (logFile) { fclose(logFile); } }

void IDebugLog::Open(const char *path) {
	if (strlen(path) > MAX_PATH) { return; } // Guard against max path

	char name[1024] = { 0 };
	if (logToFolder) {
		if (sprintf_s(name, sizeof(name), "%s\\%s\\%s", cPath, "Logs", path) < 0) { return; }
		IFileStream::MakeAllDirs(name);
	}
	else { if (sprintf_s(name, sizeof(name), "%s", path) < 0) { return; } }

	logFile = logFile ? _fsopen(name, "w", _SH_DENYWR) : nullptr;
	if (!logFile) {
		UInt32	id = 0;
		while(!logFile && (id < 5)) {
			if (sprintf_s(name, sizeof(name), "%s%d", path, id) < 0) { return; }
			logFile = logFile ? _fsopen(name, "w", _SH_DENYWR) : nullptr;
			id++;
		}
	}
}

void IDebugLog::OpenRelative(const int folderID, const char *relPath) {
	char	path[MAX_PATH];

	ASSERT(SUCCEEDED(SHGetFolderPath(NULL, folderID, NULL, SHGFP_TYPE_CURRENT, path)));

	strcat_s(path, sizeof(path), relPath);
	IFileStream::MakeAllDirs(path);

	Open(path);
}

/**
 *	Output a non-formatted message to the log file
 *	
 *	@param message the message
 *	@param source the source of the message, or NULL to use the previous source
 */
void IDebugLog::Message(const char * message, const char * source) {
	if (source) { SetSource(source); }

	if (inBlock) { SeekCursor(RoundToTab((indentLevel * 4) + strlen(headerText))); }
	else {
		SeekCursor(indentLevel * 4);
		PrintText(headerText);
	}

	PrintText(message);
	NewLine();
}

/**
 *	Output a formatted message to the log file
 *	
 *	@note It is impossible to set the source of a formatted message.
 *	The previous source will be used.
 */
void IDebugLog::FormattedMessage(const char * fmt, ...) {
	va_list	argList;

	va_start(argList, fmt);
	vsprintf_s(formatBuf, sizeof(formatBuf), fmt, argList);
	Message(formatBuf);
	va_end(argList);
}

/**
 *	Output a formatted message to the log file
 *	
 *	@note It is impossible to set the source of a formatted message.
 *	The previous source will be used.
 */
void IDebugLog::FormattedMessage(const char * fmt, const va_list args) {
	vsprintf_s(formatBuf, sizeof(formatBuf), fmt, args);
	Message(formatBuf);
}

void IDebugLog::Log(const LogLevel level, const char * fmt, const va_list args) {
	const bool log = (level <= logLevel);
	const bool print = (level <= printLevel);

	if(log || print) { vsprintf_s(formatBuf, sizeof(formatBuf), fmt, args); }
	if(log) { Message(formatBuf); }
	if(print) { printf("%s\n", formatBuf); }
}

// Set the current message source
void IDebugLog::SetSource(const char * source) {
	strcpy_s(sourceBuf, sizeof(sourceBuf), source);
	strcpy_s(headerText, sizeof(headerText), "[        ]\t");
	
	char	* tgt = headerText + 1;
	char	* src = sourceBuf;

	for(int i = 0; (i < 8) && *src; i++, tgt++, src++)
		*tgt = *src;
}

bool IDebugLog::GetLogFolderOption() { return logToFolder; }
bool IDebugLog::SetLogFolderOption(const bool option) { return logToFolder = option; }

// Clear the current message source
void IDebugLog::ClearSource() { sourceBuf[0] = 0; }

// Increase the indentation level
void IDebugLog::Indent() { indentLevel++; }

// Decrease the indentation level
void IDebugLog::Outdent() { if(indentLevel) { indentLevel--; } }

// Enter a logical block
void IDebugLog::OpenBlock() {
	SeekCursor(indentLevel * 4);
	PrintText(headerText);
	inBlock = 1;
}

// Close a logical block
void IDebugLog::CloseBlock() { inBlock = 0; }

/**
 *	Enable/disable autoflush
 *	@param inAutoFlush autoflush state
 */
void IDebugLog::SetAutoFlush(const bool inAutoFlush) { autoFlush = inAutoFlush; }

// Print spaces to the log. If possible, tabs are used instead of spaces.
void IDebugLog::PrintSpaces(int numSpaces) {
	const int originalNumSpaces = numSpaces;
	if(logFile) {
		while(numSpaces > 0) {
			if(numSpaces >= TabSize()) {
				numSpaces -= TabSize();
				fputc('\t', logFile);
			}
			else {
				numSpaces--;
				fputc(' ', logFile);
			}
		}
	}
	cursorPos += originalNumSpaces;
}

// Prints raw text to the log file
void IDebugLog::PrintText(const char *buf) {
	if(logFile) {
		fputs(buf, logFile);
		if(autoFlush) { fflush(logFile); }
	}

	const char	* traverse = buf;
	char		data;

	while(data = *traverse++)
	{
		if(data == '\t')
			cursorPos += TabSize();
		else
			cursorPos++;
	}
}

/**
 *	Moves to the next line of the log file
 */
void IDebugLog::NewLine() {
	if(logFile) {
		fputc('\n', logFile);
		if (autoFlush) { fflush(logFile); }
	}
	cursorPos = 0;
}

/**
 *	Prints spaces to align the cursor to the requested position
 *	
 *	@note The cursor move will not be performed if the request would move the cursor
 *	backwards.
 */
void IDebugLog::SeekCursor(const int position) { if(position > cursorPos) { PrintSpaces(position - cursorPos); } }

// Returns the number of spaces a tab would occupy at the current cursor position
int IDebugLog::TabSize() { return (~cursorPos & 3) + 1; }

// Rounds a number of spaces to the nearest tab
int IDebugLog::RoundToTab(const int spaces) { return (spaces + 3) & ~3; }
