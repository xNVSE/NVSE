#pragma once

#include "common/IDataStream.h"

class ITextParser
{
public:
	ITextParser();
	ITextParser(IDataStream * stream);
	~ITextParser();

	void			Attach(IDataStream * stream);
	IDataStream *	GetStream(void)	{ return m_stream; }

	bool	HitEOF(void)	{ return m_stream->HitEOF(); }

	void	SkipWhitespace(void);
	void	SkipLine(void);

	void	ReadLine(char * out, UInt32 length);
	void	ReadToken(char * out, UInt32 length);

private:
	IDataStream	* m_stream;
};
