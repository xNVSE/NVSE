#pragma once
#include <string>
#include <vector>

class SocketException : public std::exception
{
public:
	SocketException(const std::string& message) : std::exception(message.c_str()) {}
};

class SocketServer
{
public:
	UInt32 m_errno;
	UInt32 m_socket;
	UInt32 m_clientSocket;
	SocketServer(UInt16 port);

	bool WaitForConnection();
	bool CloseConnection();
	bool ReadData(char* buffer, UInt32 numBytes);

	bool ReadData(std::string& strBuf, UInt32 numBytes)
	{
		std::vector<char> buf(numBytes, 0);
		if (!ReadData(buf.data(), numBytes))
		{
			return false;
		}
		strBuf = std::string(buf.begin(), buf.end());
		return true;
	}

	template <typename T>
	bool ReadData(T& t)
	{
		if (!ReadData(reinterpret_cast<char*>(&t), sizeof(T)))
		{
			return false;
		}
		return true;
	}
};

class SocketClient
{
public:
	UInt32 m_errno;
	UInt32 m_socket;
	SocketClient(const std::string& address, UInt32 port);
	~SocketClient();
	bool SendData(const char* data, std::size_t size);

	template<typename T>
	bool SendData(T& t)
	{
		return SendData(reinterpret_cast<char*>(&t), sizeof(T));
	}
};
