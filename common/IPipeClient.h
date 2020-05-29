#pragma once

#include <string>

#include "common/IPipeServer.h"

class IPipeClient
{
public:
	IPipeClient();
	virtual ~IPipeClient();

	bool	Open(const char * name);
	void	Close(void);
	
	bool	ReadMessage(UInt8 * buf, UInt32 length);
	bool	WriteMessage(IPipeServer::MessageHeader * msg);

private:
	HANDLE		m_pipe;
	std::string	m_name;
};
