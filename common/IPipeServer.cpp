#include "IPipeServer.h"

IPipeServer::IPipeServer()
:m_pipe(INVALID_HANDLE_VALUE)
{
	//
}

IPipeServer::~IPipeServer()
{
	Close();
}

bool IPipeServer::Open(const char * name)
{
	Close();

	m_pipe = CreateNamedPipe(
		name,
		PIPE_ACCESS_DUPLEX | FILE_FLAG_FIRST_PIPE_INSTANCE,
		PIPE_TYPE_MESSAGE | PIPE_TYPE_MESSAGE | PIPE_WAIT,
		PIPE_UNLIMITED_INSTANCES,
		8192, 8192,
		10 * 1000,	// 10 seconds
		NULL);

	return m_pipe != INVALID_HANDLE_VALUE;
}

void IPipeServer::Close(void)
{
	if(m_pipe != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_pipe);
		m_pipe = INVALID_HANDLE_VALUE;
	}
}

bool IPipeServer::WaitForClient(void)
{
	bool	result = ConnectNamedPipe(m_pipe, NULL) != 0;

	// already connected?
	if(!result)
	{
		if(GetLastError() == ERROR_PIPE_CONNECTED)
			result = true;
	}

	return result;
}

bool IPipeServer::ReadMessage(UInt8 * buf, UInt32 length)
{
	UInt32	bytesRead;

	ReadFile(m_pipe, buf, length, &bytesRead, NULL);

	MessageHeader	* header = (MessageHeader *)buf;

	return
		(bytesRead >= sizeof(MessageHeader)) &&	// has a valid header
		(bytesRead >= (sizeof(MessageHeader) + header->length));
}

bool IPipeServer::WriteMessage(MessageHeader * msg)
{
	UInt32	bytesWritten;
	UInt32	length = sizeof(MessageHeader) + msg->length;

	WriteFile(m_pipe, msg, length, &bytesWritten, NULL);

	return bytesWritten >= length;
}
