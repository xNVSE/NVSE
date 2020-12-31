#include "SocketUtils.h"
#include "Ws2tcpip.h"

SocketServer::SocketServer(const UInt16 port)
{
	this->m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (this->m_socket == INVALID_SOCKET)
	{
		this->m_errno = errno;
		return;
	}
	sockaddr_in s_address{};
	s_address.sin_family = AF_INET;
	s_address.sin_port = htons(port);
	s_address.sin_addr.s_addr = INADDR_ANY;
	auto result = bind(this->m_socket, reinterpret_cast<sockaddr*>(&s_address), sizeof(s_address));
	if (result == -1)
	{
		this->m_errno = errno;
		return;
	}
	result = listen(this->m_socket, 1);
	if (result == -1)
	{
		this->m_errno = errno;
		return;
	}
	this->m_errno = 0;
}

bool SocketServer::WaitForConnection()
{
	sockaddr_in sAddress{};
	int sSize = sizeof(sAddress);
	const auto clientSocket = accept(this->m_socket, reinterpret_cast<sockaddr*>(&sAddress), &sSize);
	if (clientSocket == INVALID_SOCKET)
	{
		this->m_errno = errno;
		return false;
	}
	this->m_clientSocket = clientSocket;
	return true;
}

bool SocketServer::CloseConnection()
{
	if (closesocket(this->m_clientSocket) == -1)
	{
		this->m_errno = errno;
		return false;
	}
	this->m_clientSocket = 0;
	return true;
}

char s_recvBuf[0x1000];

bool SocketServer::ReadData(char* buffer, UInt32 numBytes)
{
	auto total = 0U;
	while (total < numBytes)
	{
		const auto numBytesRead = recv(m_clientSocket, s_recvBuf, numBytes - total, 0);
		if (numBytesRead == -1)
		{
			this->m_errno = errno;
			return false;
		}
		if (numBytes == 0)
		{
			_MESSAGE("Hot Reload Error: Tried to receive more bytes than sent");
			return false;
		}
		std::memcpy(buffer + total, s_recvBuf, numBytesRead);
		total += numBytesRead;
	}
	return true;
}

SocketClient::SocketClient(const std::string& address, UInt32 port)
{
	this->m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (this->m_socket == INVALID_SOCKET)
	{
		this->m_errno = errno;
		return;
	}
	sockaddr_in socket_addr{};
	socket_addr.sin_family = AF_INET;
	socket_addr.sin_port = htons(port);
	if (InetPton(AF_INET, address.c_str(), &socket_addr.sin_addr) == -1)
	{
		this->m_errno = errno;
		return;
	}
	if (connect(this->m_socket, reinterpret_cast<sockaddr*>(&socket_addr), sizeof socket_addr) == -1)
	{
		this->m_errno = errno;
		return;
	}
	this->m_errno = 0;
}

SocketClient::~SocketClient()
{
	closesocket(this->m_socket);
}

bool SocketClient::SendData(const char* data, std::size_t size)
{
	auto total = 0U;
	while (total < size)
	{
		const auto numBytesSent = send(m_socket, &data[total], size - total, 0);
		if (numBytesSent == -1)
		{
			this->m_errno = errno;
			return false;
		}
		total += numBytesSent;
	}
	return true;
}
