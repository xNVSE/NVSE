#include "ITextParser.h"
#include "IDataStream.h"

ITextParser::ITextParser()
:m_stream(NULL)
{
	//
}

ITextParser::ITextParser(IDataStream * stream)
:m_stream(stream)
{
	//
}

ITextParser::~ITextParser()
{
	//
}

void ITextParser::Attach(IDataStream * stream)
{
	m_stream = stream;
}

void ITextParser::SkipWhitespace(void)
{
	while(!m_stream->HitEOF())
	{
		char	data = m_stream->Peek8();

		if(!isspace(data))
			break;

		m_stream->Skip(1);
	}
}

void ITextParser::SkipLine(void)
{
	while(!m_stream->HitEOF())
	{
		char	data = m_stream->Peek8();

		if((data != '\n') && (data != '\r'))
			break;

		m_stream->Skip(1);
	}
}

void ITextParser::ReadLine(char * out, UInt32 length)
{
	m_stream->ReadString(out, length, '\n', '\r');
}

void ITextParser::ReadToken(char * buf, UInt32 bufLength)
{
	char	* traverse = buf;

	ASSERT_STR(bufLength > 0, "ITextParser::ReadToken: zero-sized buffer");

	if(bufLength == 1)
	{
		buf[0] = 0;
	}
	else
	{
		bufLength--;

		for(UInt32 i = 0; (i < bufLength) && !m_stream->HitEOF(); i++)
		{
			UInt8	data = m_stream->Read8();

			if(isspace(data) || !data)
				break;

			*traverse++ = data;
		}

		*traverse++ = 0;
	}
}
