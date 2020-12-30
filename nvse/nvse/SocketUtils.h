#pragma once
#include <string>
#include <vector>

class SocketServer
{
public:
	UInt32 m_errno;
	UInt32 m_socket;
	UInt32 m_clientSocket;
	SocketServer(UInt16 port);

	bool WaitForConnection();
	bool CloseConnection();
	bool ReadData(std::vector<char>& buffer, UInt32 numBytes);
};

class SocketClient
{
public:
	UInt32 m_errno;
	UInt32 m_socket;
	SocketClient(const std::string& address, UInt32 port);
	~SocketClient();
	bool SendData(UInt8* data, std::size_t size);
};
