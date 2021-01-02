#pragma once
#include <string>
#include <vector>
#include <typeinfo>

std::string GetLastErrorString();

class SocketException : public std::exception
{
public:
	UInt32 m_errno;
	SocketException(const std::string& message) : m_errno(WSAGetLastError()),
		std::exception(std::string("Socket error: " + message + " (\"" + GetLastErrorString() +"\" errno: " + std::to_string(WSAGetLastError()) + ")").c_str()) {}
};

class SocketServer
{
public:
	UInt32 m_serverSocket;
	UInt32 m_clientSocket;
	explicit SocketServer(UInt16 port);

	void WaitForConnection();
	void CloseConnection();
	void ReadData(char* buffer, UInt32 numBytes);

	void ReadData(std::string& strBuf, UInt32 numBytes)
	{
		std::vector<char> buf(numBytes, 0);
		try
		{
			ReadData(buf.data(), numBytes);
		}
		catch (const SocketException&)
		{
			throw SocketException("Failed to read to string (string size: " + std::to_string(numBytes) + " )");
		}
		strBuf = std::string(buf.begin(), buf.end());
	}

	template <typename T>
	void ReadData(T& t)
	{
		try
		{
			ReadData(reinterpret_cast<char*>(&t), sizeof(T));
		}
		catch (const SocketException&)
		{
			throw SocketException("Failed to receive to object '" + std::string(typeid(T).name()));
		}
	}
};

class SocketClient
{
public:
	UInt32 m_socket;
	SocketClient(const std::string& address, UInt32 port);
	~SocketClient();
	void SendData(const char* data, std::size_t size) const;

	template<typename T>
	void SendData(T& t)
	{
		try
		{
			SendData(reinterpret_cast<char*>(&t), sizeof(T));
		}
		catch (const SocketException&)
		{
			throw SocketException("Failed to send object " + std::string(typeid(T).name()));
		}
	}
};
