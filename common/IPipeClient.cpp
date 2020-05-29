#include "IPipeClient.h"

IPipeClient::IPipeClient()
:m_pipe(INVALID_HANDLE_VALUE)
{
	//
}

IPipeClient::~IPipeClient()
{
	Close();
}

bool IPipeClient::Open(const char * name)
{
	Close();

	m_pipe = CreateFile(
		name,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);

	return m_pipe != INVALID_HANDLE_VALUE;
}

void IPipeClient::Close(void)
{
	if(m_pipe != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_pipe);
		m_pipe = INVALID_HANDLE_VALUE;
	}
}

bool IPipeClient::ReadMessage(UInt8 * buf, UInt32 length)
{
	UInt32	bytesRead;

	ReadFile(m_pipe, buf, length, &bytesRead, NULL);

	IPipeServer::MessageHeader	* header = (IPipeServer::MessageHeader *)buf;

	return
		(bytesRead >= sizeof(IPipeServer::MessageHeader)) &&	// has a valid header
		(bytesRead >= (sizeof(IPipeServer::MessageHeader) + header->length));
}

bool IPipeClient::WriteMessage(IPipeServer::MessageHeader * msg)
{
	UInt32	bytesWritten;
	UInt32	length = sizeof(IPipeServer::MessageHeader) + msg->length;

	WriteFile(m_pipe, msg, length, &bytesWritten, NULL);

	return bytesWritten >= length;
}
