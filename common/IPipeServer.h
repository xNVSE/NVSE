#pragma once

class IPipeServer
{
public:
	struct MessageHeader
	{
		UInt32	type;
		UInt32	length;
	};

	IPipeServer();
	virtual ~IPipeServer();

	bool	Open(const char * name);
	void	Close(void);

	bool	WaitForClient(void);

	bool	ReadMessage(UInt8 * buf, UInt32 length);
	bool	WriteMessage(MessageHeader * msg);

private:
	HANDLE	m_pipe;
};
