#include "SocketUtils.h"
#include "Ws2tcpip.h"

std::string GetLastErrorString()
{
	wchar_t* s = NULL;
	FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPWSTR)&s, 0, NULL);
	fprintf(stderr, "%S\n", s);
	std::wstring ws(s);
	std::string str(ws.begin(), ws.end());
	LocalFree(s);
	return str;
}
SocketServer::SocketServer(const UInt16 port)
{
	this->m_serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (this->m_serverSocket == INVALID_SOCKET)
	{
		throw SocketException("Failed to create socket for server");
	}
	sockaddr_in s_address{};
	s_address.sin_family = AF_INET;
	s_address.sin_port = htons(port);
	s_address.sin_addr.s_addr = INADDR_ANY;
	auto result = bind(this->m_serverSocket, reinterpret_cast<sockaddr*>(&s_address), sizeof(s_address));
	if (result == -1)
	{
		throw SocketException("Failed to bind socket");
	}
	result = listen(this->m_serverSocket, 1);
	if (result == -1)
	{
		throw SocketException("Failed to listen to socket");
	}
}

void SocketServer::WaitForConnection()
{
	sockaddr_in sAddress{};
	int sSize = sizeof(sAddress);
	const auto clientSocket = accept(this->m_serverSocket, reinterpret_cast<sockaddr*>(&sAddress), &sSize);
	if (clientSocket == INVALID_SOCKET)
	{
		throw SocketException("Failed to establish connection to client");
	}
	this->m_clientSocket = clientSocket;
}

void SocketServer::CloseConnection()
{
	if (!this->m_clientSocket)
		return;
	if (closesocket(this->m_clientSocket) == -1)
	{
		throw SocketException("Failed to close socket");
	}
	this->m_clientSocket = 0;
}

char s_recvBuf[0x1000];

void SocketServer::ReadData(char* buffer, UInt32 numBytes)
{
	auto total = 0U;
	while (total < numBytes)
	{
		const auto numBytesRead = recv(m_clientSocket, s_recvBuf, numBytes - total, 0);
		if (numBytesRead == -1)
		{
			throw SocketException("Failed to read data (num bytes: " + std::to_string(numBytes)+ ")");
		}
		if (numBytes == 0)
		{
			throw SocketException("Tried to receive more bytes than sent");
		}
		std::memcpy(buffer + total, s_recvBuf, numBytesRead);
		total += numBytesRead;
	}
}

SocketClient::SocketClient(const std::string& address, UInt32 port)
{
	this->m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (this->m_socket == INVALID_SOCKET)
	{
		throw SocketException("Failed to create socket for client");
	}
	sockaddr_in socket_addr{};
	socket_addr.sin_family = AF_INET;
	socket_addr.sin_port = htons(port);
	if (InetPton(AF_INET, address.c_str(), &socket_addr.sin_addr) == -1)
	{
		throw SocketException("InetPton failed");
	}
	if (connect(this->m_socket, reinterpret_cast<sockaddr*>(&socket_addr), sizeof socket_addr) == -1)
	{
		throw SocketException("Failed to connect to server socket");
	}
}

SocketClient::~SocketClient()
{
	closesocket(this->m_socket);
}

void SocketClient::SendData(const char* data, std::size_t size) const
{
	auto total = 0U;
	while (total < size)
	{
		const auto numBytesSent = send(m_socket, &data[total], size - total, 0);
		if (numBytesSent == -1)
		{
			throw SocketException("Failed to send data (size: " + std::to_string(size));
		}
		total += numBytesSent;
	}
}
