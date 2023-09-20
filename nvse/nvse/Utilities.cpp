#include "Utilities.h"
#include "SafeWrite.h"
#include <string>
#include <algorithm>
#include <filesystem>
#include <tlhelp32.h>
#include "containers.h"
#include "GameData.h"
#include "Hooks_Gameplay.h"
#include "LambdaManager.h"
#include "PluginAPI.h"
#include "PluginManager.h"

#if RUNTIME
#include "GameAPI.h"
#include "GameForms.h"
#endif

void DumpClass(void * theClassPtr, UInt32 nIntsToDump)
{
	_MESSAGE("DumpClass:");
	UInt32* basePtr = (UInt32*)theClassPtr;

	gLog.Indent();

	if (!theClassPtr) return;
	for (UInt32 ix = 0; ix < nIntsToDump; ix++ ) {
		UInt32* curPtr = basePtr+ix;
		const char* curPtrName = NULL;
		UInt32 otherPtr = 0;
		float otherFloat = 0.0;
		const char* otherPtrName = NULL;
		if (curPtr) {
			curPtrName = GetObjectClassName((void*)curPtr);

			__try
			{
				otherPtr = *curPtr;
				otherFloat = *(float*)(curPtr);
			}
			__except(EXCEPTION_EXECUTE_HANDLER)
			{
				//
			}

			if (otherPtr) {
				otherPtrName = GetObjectClassName((void*)otherPtr);
			}
		}

		_MESSAGE("%3d +%03X ptr: 0x%08X: %32s *ptr: 0x%08x | %f: %32s", ix, ix*4, curPtr, curPtrName, otherPtr, otherFloat, otherPtrName);
	}

	gLog.Outdent();
}

#pragma warning (push)
#pragma warning (disable : 4200)
struct RTTIType
{
	void	* typeInfo;
	UInt32	pad;
	char	name[0];
};

struct RTTILocator
{
	UInt32		sig, offset, cdOffset;
	RTTIType	* type;
};
#pragma warning (pop)

// use the RTTI information to return an object's class name
const char * GetObjectClassName(void * objBase)
{
	const char	* result = "<no rtti>";

	__try
	{
		void		** obj = (void **)objBase;
		RTTILocator	** vtbl = (RTTILocator **)obj[0];
		RTTILocator	* rtti = vtbl[-1];
		RTTIType	* type = rtti->type;

		// starts with ,?
		if((type->name[0] == '.') && (type->name[1] == '?'))
		{
			// is at most 100 chars long
			for(UInt32 i = 0; i < 100; i++)
			{
				if(type->name[i] == 0)
				{
					// remove the .?AV
					result = type->name + 4;
					break;
				}
			}
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		// return the default
	}

	return result;
}

const std::string & GetFalloutDirectory(void)
{
	static std::string s_falloutDirectory;

	if(s_falloutDirectory.empty())
	{
		// can't determine how many bytes we'll need, hope it's not more than MAX_PATH
		char	falloutPathBuf[MAX_PATH];
		UInt32	falloutPathLength = GetModuleFileName(GetModuleHandle(NULL), falloutPathBuf, sizeof(falloutPathBuf));

		if(falloutPathLength && (falloutPathLength < sizeof(falloutPathBuf)))
		{
			std::string	falloutPath(falloutPathBuf, falloutPathLength);

			// truncate at last slash
			std::string::size_type	lastSlash = falloutPath.rfind('\\');
			if(lastSlash != std::string::npos)	// if we don't find a slash something is VERY WRONG
			{
				s_falloutDirectory = falloutPath.substr(0, lastSlash + 1);

				_DMESSAGE("fallout root = %s", s_falloutDirectory.c_str());
			}
			else
			{
				_WARNING("no slash in fallout path? (%s)", falloutPath.c_str());
			}
		}
		else
		{
			_WARNING("couldn't find fallout path (len = %d, err = %08X)", falloutPathLength, GetLastError());
		}
	}

	return s_falloutDirectory;
}

static const std::string & GetNVSEConfigPath(void)
{
	static std::string s_configPath;

	if(s_configPath.empty())
	{
		std::string	falloutPath = GetFalloutDirectory();
		if(!falloutPath.empty())
		{
			s_configPath = falloutPath + "Data\\NVSE\\nvse_config.ini";

			_MESSAGE("config path = %s", s_configPath.c_str());
		}
	}

	return s_configPath;
}

std::string GetNVSEConfigOption(const char * section, const char * key)
{
	std::string	result;

	const std::string & configPath = GetNVSEConfigPath();
	if(!configPath.empty())
	{
		char	resultBuf[256];
		resultBuf[0] = 0;

		UInt32	resultLen = GetPrivateProfileString(section, key, NULL, resultBuf, 255, configPath.c_str());

		result = resultBuf;
	}

	return result;
}

bool GetNVSEConfigOption_UInt32(const char * section, const char * key, UInt32 * dataOut)
{
	std::string	data = GetNVSEConfigOption(section, key);
	if(data.empty())
		return false;

	return (sscanf(data.c_str(), "%lu", dataOut) == 1);
}

namespace MersenneTwister
{

/* 
   A C-program for MT19937, with initialization improved 2002/1/26.
   Coded by Takuji Nishimura and Makoto Matsumoto.

   Before using, initialize the state by using init_genrand(seed)  
   or init_by_array(init_key, key_length).

   Copyright (C) 1997 - 2002, Makoto Matsumoto and Takuji Nishimura,
   All rights reserved.                          

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

     1. Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.

     2. Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

     3. The names of its contributors may not be used to endorse or promote 
        products derived from this software without specific prior written 
        permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


   Any feedback is very welcome.
   http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/emt.html
   email: m-mat @ math.sci.hiroshima-u.ac.jp (remove space)
*/

/* Period parameters */  
#define N 624
#define M 397
#define MATRIX_A 0x9908b0dfUL   /* constant vector a */
#define UPPER_MASK 0x80000000UL /* most significant w-r bits */
#define LOWER_MASK 0x7fffffffUL /* least significant r bits */

static unsigned long mt[N]; /* the array for the state vector  */
static int mti=N+1; /* mti==N+1 means mt[N] is not initialized */

/* initializes mt[N] with a seed */
void init_genrand(unsigned long s)
{
    mt[0]= s & 0xffffffffUL;
    for (mti=1; mti<N; mti++) {
        mt[mti] = 
	    (1812433253UL * (mt[mti-1] ^ (mt[mti-1] >> 30)) + mti); 
        /* See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier. */
        /* In the previous versions, MSBs of the seed affect   */
        /* only MSBs of the array mt[].                        */
        /* 2002/01/09 modified by Makoto Matsumoto             */
        mt[mti] &= 0xffffffffUL;
        /* for >32 bit machines */
    }
}

/* initialize by an array with array-length */
/* init_key is the array for initializing keys */
/* key_length is its length */
/* slight change for C++, 2004/2/26 */
void init_by_array(unsigned long init_key[], int key_length)
{
    int i, j, k;
    init_genrand(19650218UL);
    i=1; j=0;
    k = (N>key_length ? N : key_length);
    for (; k; k--) {
        mt[i] = (mt[i] ^ ((mt[i-1] ^ (mt[i-1] >> 30)) * 1664525UL))
          + init_key[j] + j; /* non linear */
        mt[i] &= 0xffffffffUL; /* for WORDSIZE > 32 machines */
        i++; j++;
        if (i>=N) { mt[0] = mt[N-1]; i=1; }
        if (j>=key_length) j=0;
    }
    for (k=N-1; k; k--) {
        mt[i] = (mt[i] ^ ((mt[i-1] ^ (mt[i-1] >> 30)) * 1566083941UL))
          - i; /* non linear */
        mt[i] &= 0xffffffffUL; /* for WORDSIZE > 32 machines */
        i++;
        if (i>=N) { mt[0] = mt[N-1]; i=1; }
    }

    mt[0] = 0x80000000UL; /* MSB is 1; assuring non-zero initial array */ 
}

/* generates a random number on [0,0xffffffff]-interval */
unsigned long genrand_int32(void)
{
    unsigned long y;
    static unsigned long mag01[2]={0x0UL, MATRIX_A};
    /* mag01[x] = x * MATRIX_A  for x=0,1 */

    if (mti >= N) { /* generate N words at one time */
        int kk;

        if (mti == N+1)   /* if init_genrand() has not been called, */
            init_genrand(5489UL); /* a default initial seed is used */

        for (kk=0;kk<N-M;kk++) {
            y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
            mt[kk] = mt[kk+M] ^ (y >> 1) ^ mag01[y & 0x1UL];
        }
        for (;kk<N-1;kk++) {
            y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
            mt[kk] = mt[kk+(M-N)] ^ (y >> 1) ^ mag01[y & 0x1UL];
        }
        y = (mt[N-1]&UPPER_MASK)|(mt[0]&LOWER_MASK);
        mt[N-1] = mt[M-1] ^ (y >> 1) ^ mag01[y & 0x1UL];

        mti = 0;
    }
  
    y = mt[mti++];

    /* Tempering */
    y ^= (y >> 11);
    y ^= (y << 7) & 0x9d2c5680UL;
    y ^= (y << 15) & 0xefc60000UL;
    y ^= (y >> 18);

    return y;
}

/* generates a random number on [0,0x7fffffff]-interval */
long genrand_int31(void)
{
    return (long)(genrand_int32()>>1);
}

/* generates a random number on [0,1]-real-interval */
double genrand_real1(void)
{
    return genrand_int32()*(1.0/4294967295.0); 
    /* divided by 2^32-1 */ 
}

/* generates a random number on [0,1)-real-interval */
double genrand_real2(void)
{
    return genrand_int32()*(1.0/4294967296.0); 
    /* divided by 2^32 */
}

/* generates a random number on (0,1)-real-interval */
double genrand_real3(void)
{
    return (((double)genrand_int32()) + 0.5)*(1.0/4294967296.0); 
    /* divided by 2^32 */
}

/* generates a random number on [0,1) with 53-bit resolution*/
double genrand_res53(void) 
{ 
    unsigned long a=genrand_int32()>>5, b=genrand_int32()>>6; 
    return(a*67108864.0+b)*(1.0/9007199254740992.0); 
} 
/* These real versions are due to Isaku Wada, 2002/01/09 added */

#undef N
#undef M
#undef MATRIX_A
#undef UPPER_MASK
#undef LOWER_MASK

};

Tokenizer::Tokenizer(const char* src, const char* delims)
: m_offset(0), m_delims(delims), m_data(src)
{
	//
}

UInt32 Tokenizer::NextToken(std::string& outStr)
{
	if (m_offset == m_data.length())
		return -1;

	size_t const start = m_data.find_first_not_of(m_delims, m_offset);
	if (start != -1)
	{
		size_t end = m_data.find_first_of(m_delims, start);
		if (end == -1)
			end = m_data.length();

		m_offset = end;
		outStr = m_data.substr(start, end - start);
		return start;
	}

	return -1;
}

std::string Tokenizer::ToNewLine()
{
	if (m_offset == m_data.length())
		return "";

	size_t const start = m_data.find_first_not_of(m_delims, m_offset);
	if (start != -1)
	{
		size_t end = m_data.find_first_of('\n', start);
		if (end == -1)
			end = m_data.length();

		m_offset = end;
		return m_data.substr(start, end - start);
	}

	return "";
}

UInt32 Tokenizer::PrevToken(std::string& outStr)
{
	if (m_offset == 0)
		return -1;

	size_t searchStart = m_data.find_last_of(m_delims, m_offset - 1);
	if (searchStart == -1)
		return -1;

	size_t end = m_data.find_last_not_of(m_delims, searchStart);
	if (end == -1)
		return -1;

	size_t start = m_data.find_last_of(m_delims, end);	// okay if start == -1 here

	m_offset = end + 1;
	outStr = m_data.substr(start + 1, end - start);
	return start + 1;
}

ScriptTokenizer::ScriptTokenizer(std::string_view scriptText) : m_scriptText(scriptText)
{
	//
}

bool ScriptTokenizer::TryLoadNextLine()
{
	m_loadedLineTokens = {};
	m_tokenOffset = 0;
	if (m_scriptOffset == m_scriptText.length())
		return false;

#if _DEBUG
	ASSERT(m_scriptOffset <= m_scriptText.length());
#endif

	auto GetLineEndPos = [this](size_t startPos) -> size_t
	{
		auto result = m_scriptText.find_first_of("\n\r", startPos);
		if (result == std::string_view::npos)
			result = m_scriptText.length();
		return result;
	};

	// Finds the pos of the the start of a valid token on a new line, if any.
	auto GetNextLineStartPos = [this](size_t startPos) -> size_t
	{
		auto result = m_scriptText.find_first_of("\n\r", startPos);
		if (result == std::string_view::npos)
			result = m_scriptText.length();
		else
		{
			// Skip over consecutive empty lines.
			result = m_scriptText.find_first_not_of(" \t\n\r", result + 1);
			if (result == std::string_view::npos)
				result = m_scriptText.length();
		}
		return result;
	};

	// Ignore spaces and newlines at the start - find the start of a REAL line.
	// Line pos is relative to the entire script text.
	if (auto linePos = m_scriptText.find_first_not_of(" \t\n\r", m_scriptOffset);
		linePos != std::string_view::npos)
	{
		if (m_inMultilineComment)
		{
			auto const multilineCommentEndStartPos = m_scriptText.find("*/", linePos);
			if (multilineCommentEndStartPos == std::string_view::npos)
			{
				m_scriptOffset = m_scriptText.size();
				return false; // unable to find an end to the multiline comment (xEdit shenanigans?)
			}
			m_inMultilineComment = false;
			if (multilineCommentEndStartPos + 2 == m_scriptText.length())
			{
				m_scriptOffset = m_scriptText.size();
				return false;
			}

			// If there was a ';' comment inside of the multiline comment on the line where it ended,
			// .. it will still comment out the rest of the line.
			if (std::string_view const insideOfMultilineCommentOnThisLine(&m_scriptText.at(linePos), multilineCommentEndStartPos - linePos);
				insideOfMultilineCommentOnThisLine.find_first_of(';') != std::string_view::npos)
			{
				// Line is commented out; ignore it and try loading the next one.
				m_scriptOffset = GetNextLineStartPos(multilineCommentEndStartPos + 2);
				return TryLoadNextLine();
			}
			//else, there might be something left in this line.
			linePos = multilineCommentEndStartPos + 2;
		}

		// If line is empty, skip to a new line.
		linePos = m_scriptText.find_first_not_of(" \t\n\r", linePos);
		if (linePos == std::string_view::npos)
		{
			m_scriptOffset = m_scriptText.size();
			return false; // rest of file is empty
		}

		// Handle comments and try loading tokens that are on the same line.
		for (auto const lineEndPos = GetLineEndPos(linePos); true; )
		{
			// Check if the rest of the line is commented out via ';'.
			if (m_scriptText.at(linePos) == ';')
			{
				m_scriptOffset = GetNextLineStartPos(linePos + 1);
				if (m_loadedLineTokens.empty())
				{
					// Line is fully commented out; ignore it and try loading the next one.
					return TryLoadNextLine();
				}
				//else, there were some tokens in the line before the comment.
				return true;
			}

			// Handle possible back-to-back multiline comments on the same line.
			// Ex: "/* *//* */  /* */ Finally,_I_Am_A_Real_Token. /* \n */\n"
			for (std::string_view lineView(&m_scriptText.at(linePos), lineEndPos - linePos);
				lineView.size() >= 2 && lineView.front() == '/' && lineView.at(1) == '*';
				lineView = { &m_scriptText.at(linePos), lineEndPos - linePos })
			{
				auto HandleCommentSpanningMultipleLines = [this, lineEndPos]() -> bool
				{
					// Line ended; assume multiline comment that spans multiple lines.
					m_inMultilineComment = true;
					m_scriptOffset = m_scriptText.find_first_not_of(" \t\n\r", lineEndPos);
					if (m_scriptOffset == std::string_view::npos)
						m_scriptOffset = m_scriptText.size();
					if (!m_loadedLineTokens.empty())
						return true;
					return TryLoadNextLine();
				};

				// ignore the "/*" chars
				if (linePos + 2 == lineEndPos)
					return HandleCommentSpanningMultipleLines();

				// Check if the multiline comment ends on this line.
				if (auto endMultilineCommentStartPos = lineView.find("*/");
					endMultilineCommentStartPos == std::string_view::npos)
				{
					return HandleCommentSpanningMultipleLines();
				}
				else
				{
					// else, multiline comment ends in this line.
					// There might be tokens left in this line; if not, skip to the next if this line was empty.

					// Make pos relative to the entire script text.
					endMultilineCommentStartPos += linePos;
					if (endMultilineCommentStartPos + 2 == m_scriptText.size())
					{
						m_scriptOffset = m_scriptText.size();
						return false;
					}

					// If there was a ';' comment inside of the multiline comment on the line where it ended,
					// .. it will still comment out the rest of the line.
					if (std::string_view const insideOfMultilineComment(&m_scriptText.at(linePos + 2), endMultilineCommentStartPos - linePos);
						insideOfMultilineComment.find_first_of(';') != std::string_view::npos)
					{
						// Line is commented out; IF we didn't get tokens, ignore it and try loading the next one.
						m_scriptOffset = GetNextLineStartPos(endMultilineCommentStartPos + 2);
						if (!m_loadedLineTokens.empty())
							return true;
						return TryLoadNextLine();
					}

					// Handle multiline comment that ends in-line, but is followed by end-of-line.
					m_scriptOffset = m_scriptText.find_first_not_of(" \t", endMultilineCommentStartPos + 2);
					if (m_scriptOffset == std::string_view::npos)
					{
						m_scriptOffset = m_scriptText.size();
						return !m_loadedLineTokens.empty();
					}
					if (m_scriptOffset == lineEndPos)
					{
						if (!m_loadedLineTokens.empty())
							return true;
						// Line is commented out; ignore it and try loading the next one.
						return TryLoadNextLine();
					}

					// Else, line goes on.
					linePos = m_scriptText.find_first_not_of(" \t\n\r", endMultilineCommentStartPos + 2);
					if (linePos == std::string_view::npos)
					{
						m_scriptOffset = m_scriptText.size();
						return !m_loadedLineTokens.empty();
					}
				}
			}

			// Handle ';' comment right after 1-line /* */ comment(s)
			if (m_scriptText.at(linePos) == ';')
			{
				m_scriptOffset = GetNextLineStartPos(linePos + 1);
				if (m_loadedLineTokens.empty())
				{
					// Line is fully commented out; ignore it and try loading the next one.
					return TryLoadNextLine();
				}
				//else, there were some tokens in the line before the comment.
				return true;
			}

			// Line pos should now point to the start of a token.
			// Get the post-the-end character position for the token.
			size_t endOfTokenPos;
			if (m_scriptText.at(linePos) == '"')
			{
				// Get the full string as a single token.
				endOfTokenPos = m_scriptText.find_first_of('"', linePos + 1);
				if (endOfTokenPos == std::string_view::npos)
					endOfTokenPos = m_scriptText.size();
				else
					++endOfTokenPos; // include '"' char at the end.
			}
			else
			{
				endOfTokenPos = m_scriptText.find_first_of(" \t\n\r", linePos);
				if (endOfTokenPos == std::string_view::npos)
					endOfTokenPos = m_scriptText.size();
			}

			auto tokenView = m_scriptText.substr(linePos, endOfTokenPos - linePos);

			// trim comments off of the end of the token
			if (tokenView.back() == ';')
			{
				--endOfTokenPos;
			}
			else if (tokenView.size() > 2 && 
				tokenView.at(tokenView.size() - 2) == '/' && tokenView.back() == '*')
			{
				endOfTokenPos -= 2;
			}
			tokenView = m_scriptText.substr(linePos, endOfTokenPos - linePos);

			m_loadedLineTokens.push_back(tokenView);
			if (endOfTokenPos == m_scriptText.size())
				break;

			linePos = m_scriptText.find_first_not_of(" \t", endOfTokenPos);
			if (linePos == std::string_view::npos)
			{
				linePos = m_scriptText.size();
				break;
			}
			if (linePos == lineEndPos)
				break;
		}

		m_scriptOffset = GetNextLineStartPos(linePos);
		return !m_loadedLineTokens.empty();
	}
	// else, rest of script is just spaces
	m_scriptOffset = m_scriptText.size();
	return false;
}

std::string_view ScriptTokenizer::GetNextLineToken()
{
	if (m_loadedLineTokens.empty() || m_tokenOffset >= m_loadedLineTokens.size())
		return "";
	return m_loadedLineTokens.at(m_tokenOffset++);
}

std::string_view ScriptTokenizer::GetLineText()
{
	if (!m_loadedLineTokens.empty())
	{
		if (m_loadedLineTokens.size() > 1)
		{
			const char* startAddr = m_loadedLineTokens[0].data();
			std::string_view lastToken = m_loadedLineTokens[m_loadedLineTokens.size() - 1];
			// assume lastToken isn't empty
			const char* endAddr = &lastToken.at(lastToken.size() - 1);

			size_t count = endAddr - startAddr + 1;
			size_t startPos = startAddr - m_scriptText.data();
			return m_scriptText.substr(startPos, count);
		}
		else // only 1 token
		{
			return m_loadedLineTokens[0];
		}
	}
	return "";
}

#if RUNTIME

const char GetSeparatorChar(Script * script)
{
	if(IsConsoleMode())
	{
		if(script && script->GetModIndex() != 0xFF)
			return '|';
		else
			return '@';
	}
	else
		return '|';
}

const char * GetSeparatorChars(Script * script)
{
	if(IsConsoleMode())
	{
		if(script && script->GetModIndex() != 0xFF)
			return "|";
		else
			return "@";
	}
	else
		return "|";
}

void Console_Print_Long(const std::string& str)
{
	UInt32 numLines = str.length() / 500;
	for (UInt32 i = 0; i < numLines; i++)
		Console_Print("%s ...", str.substr(i*500, 500).c_str());

	Console_Print("%s", str.substr(numLines*500, str.length() - numLines*500).c_str());
}

void Console_Print_Str(const std::string& str)
{
	if (str.size() < 512)
		Console_Print("%s", str.c_str());
	else
		Console_Print_Long(str);
}

#endif

struct ControlName
{
	UInt32		unk0;
	const char	* name;
	UInt32		unkC;
};

ControlName ** g_keyNames = (ControlName **)0x011D52F0;
ControlName ** g_mouseButtonNames = (ControlName **)0x011D5240;
ControlName ** g_joystickNames = (ControlName **)0x011D51B0;

const char * GetDXDescription(UInt32 keycode)
{
	const char * keyName = "<no key>";

	if(keycode <= 220)
	{
		if(g_keyNames[keycode])
			keyName = g_keyNames[keycode]->name;
	}
	else if(255 <= keycode && keycode <= 263)
	{
		if(keycode == 255)
			keycode = 256;
		if(g_mouseButtonNames[keycode - 256])
			keyName = g_mouseButtonNames[keycode - 256]->name;
	}
	else if (keycode == 264)
		keyName = "WheelUp";
	else if (keycode == 265)
		keyName = "WheelDown";

	return keyName;
}

bool ci_equal(char ch1, char ch2)
{
	return game_tolower((unsigned char)ch1) == game_tolower((unsigned char)ch2);
}

bool ci_less(const char* lh, const char* rh)
{
	ASSERT(lh && rh);
	while (*lh && *rh) {
		char l = game_toupper(*lh);
		char r = game_toupper(*rh);
		if (l < r) {
			return true;
		}
		else if (l > r) {
			return false;
		}
		lh++;
		rh++;
	}

	return game_toupper(*lh) < game_toupper(*rh);
}

void MakeUpper(std::string& str)
{
	std::transform(str.begin(), str.end(), str.begin(), game_toupper);
}

void MakeLower(std::string& str)
{
	std::transform(str.begin(), str.end(), str.begin(), game_tolower);
}

#if RUNTIME

char* CopyCString(const char* src)
{
	UInt32 size = src ? strlen(src) : 0;
	char* result = (char*)FormHeap_Allocate(size+1);
	result[size] = 0;
	if (size) {
		strcpy_s(result, size+1, src);
	}

	return result;
}

#endif

#pragma warning(push)
#pragma warning(disable: 4996)	// warning about std::transform()

void MakeUpper(char* str)
{
	if (str) {
		UInt32 len = strlen(str);
		std::transform(str, str + len, str, game_toupper);
	}
}

void MakeLower(char* str)
{
	if (str) {
		UInt32 len = strlen(str);
		std::transform(str, str + len, str, game_tolower);
	}
}

#pragma warning(pop)

// ErrOutput
ErrOutput::ErrOutput(_ShowError errorFunc, _ShowWarning warningFunc)
{
	ShowWarning = warningFunc;
	ShowError = errorFunc;
}

void ErrOutput::vShow(ErrOutput::Message& msg, va_list args)
{
	char msgText[0x400];
	vsprintf_s(msgText, sizeof(msgText), msg.fmt, args);
	if (msg.bCanDisable)
	{
		if (!msg.bDisabled)
			if (ShowWarning(msgText))
				msg.bDisabled = true;
	}
	else
		ShowError(msgText);
}

void ErrOutput::Show(ErrOutput::Message msg, ...)
{
	va_list args;
	va_start(args, msg);

	vShow(msg, args);
}

void ErrOutput::Show(const char* msg, ...)
{
	va_list args;
	va_start(args, msg);

	vShow(msg, args);
}

void ErrOutput::vShow(const char* msg, va_list args)
{
	Message tempMsg;
	tempMsg.fmt = msg;
	tempMsg.bCanDisable = false;
	tempMsg.bDisabled = false;

	vShow(tempMsg, args);
}

void ShowErrorMessageBox(const char* message)
{
	int msgboxID = MessageBox(
		NULL,
		message,
		"Error",
		MB_ICONWARNING | MB_OK
	);
}

#if RUNTIME

const char* GetModName(TESForm* form)
{
	if (!form)
		return "Unknown or deleted script";
	const char* modName = IS_ID(form, Script) ? "In-game console" : "Dynamic form";
	if (form->mods.Head() && form->mods.Head()->data)
		return form->mods.Head()->Data()->name;
	if (form->GetModIndex() != 0xFF)
	{
		modName = DataHandler::Get()->GetNthModName(form->GetModIndex());
		if (!modName || !modName[0])
			modName = "Unknown";
	}
	return modName;
}
#if NVSE_CORE
UnorderedSet<UInt32> g_warnedScripts;

void ShowRuntimeError(Script* script, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	char errorMsg[0x800];
	vsprintf_s(errorMsg, sizeof(errorMsg), fmt, args);
	
	char errorHeader[0x900];
	const auto* modName = GetModName(script);
	
	const auto* scriptName = script ? script->GetName() : nullptr; // JohnnyGuitarNVSE allows this
	auto refId = script ? script->refID : 0;
	const auto modIdx = script ? script->GetModIndex() : 0;
	if (script && LambdaManager::IsScriptLambda(script))
	{
		Script* parentScript;
		if (auto* parentEventList = LambdaManager::GetParentEventList(script); 
			parentEventList && ((parentScript = parentEventList->m_script)))
		{
			refId = parentScript->refID;
			scriptName = parentScript->GetName();
		}
	}
	if (scriptName && strlen(scriptName) != 0)
	{
		sprintf_s(errorHeader, sizeof(errorHeader), "Error in script %08X (%s) in mod %s\n%s", refId, scriptName, modName, errorMsg);
	}
	else
	{
		sprintf_s(errorHeader, sizeof(errorHeader), "Error in script %08X in mod %s\n%s", refId, modName, errorMsg);
	}

	if (g_warnScriptErrors && g_myMods.contains(modIdx) && g_warnedScripts.Insert(refId))
	{
		char message[512];
		snprintf(message, sizeof(message), "%s: Script error (see console print)", GetModName(script));
		if (!IsConsoleMode())
			QueueUIMessage(message, 0, reinterpret_cast<const char*>(0x1049638), nullptr, 2.5F, false);
	}

	if (strlen(errorHeader) < 512)
		Console_Print("%s", errorHeader);
	else
		Console_Print_Long(errorHeader);
	_MESSAGE("%s", errorHeader);

	PluginManager::Dispatch_Message(0, NVSEMessagingInterface::kMessage_RuntimeScriptError, errorMsg, 4, NULL);

	va_end(args);
}
#endif
#endif

std::string FormatString(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	char msg[0x800];
	vsprintf_s(msg, 0x800, fmt, args);
	return msg;
}

std::vector<void*> GetCallStack(int i)
{
	std::vector<void*> vecTrace(i, nullptr);
	CaptureStackBackTrace(1, i, reinterpret_cast<PVOID*>(vecTrace.data()), nullptr);
	return vecTrace;
}

bool FindStringCI(const std::string& strHaystack, const std::string& strNeedle)
{
	auto it = std::search(
		strHaystack.begin(), strHaystack.end(),
		strNeedle.begin(), strNeedle.end(),
		[](char ch1, char ch2) { return game_toupper(ch1) == game_toupper(ch2); }
	);
	return (it != strHaystack.end());
}

void ReplaceAll(std::string &str, const std::string& from, const std::string& to)
{
    size_t startPos = 0;
    while((startPos = str.find(from, startPos)) != std::string::npos) 
	{
        str.replace(startPos, from.length(), to);
        startPos += to.length(); // Handles case where 'to' is a substring of 'from'
    }

}

void GeckExtenderMessageLog(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	auto* window = FindWindow("RTEDITLOG", nullptr);
	if (!window)
	{
		_MESSAGE("Failed to find GECK Extender Message Log window");
		return;
	}
	
	char buffer[0x400];
	vsprintf_s(buffer, 0x400, fmt, args);
	strcat_s(buffer, "\n");

	SendMessage(window, 0x8004, 0, reinterpret_cast<LPARAM>(buffer));
}

bool Cmd_Default_Execute(COMMAND_ARGS)
{
	return true;
}

bool Cmd_Default_Eval(COMMAND_ARGS_EVAL)
{
	return true;
}

void ToWChar(wchar_t* ws, const char* c)
{
	swprintf(ws, 100, L"%hs", c);
}

bool IsProcessRunning(const char* executableName)
{
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);

	const auto snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	if (!Process32First(snapshot, &entry)) {
		CloseHandle(snapshot);
		return false;
	}

	do {
		if (!_stricmp(entry.szExeFile, executableName)) {
			CloseHandle(snapshot);
			return true;
		}
	} while (Process32Next(snapshot, &entry));

	CloseHandle(snapshot);
	return false;
}
#if NVSE_CORE && RUNTIME
void DisplayMessage(const char* msg)
{
	ShowMessageBox(msg, 0, 0, ShowMessageBox_Callback, 0, 0x17, 0.0, 0.0, "Ok", 0);
}
#endif

std::string GetCurPath()
{
	char buffer[MAX_PATH] = { 0 };
	GetModuleFileName(NULL, buffer, MAX_PATH);
	std::string::size_type pos = std::string(buffer).find_last_of("\\/");
	return std::string(buffer).substr(0, pos);
}

bool ValidString(const char* str)
{
	return str && strlen(str);
}

#if _DEBUG
// debugger can't call unused member functions
const char* GetFormName(TESForm* form)
{
	return form ? form->GetName() : "";
}

const char* GetFormName(UInt32 formId)
{
	return GetFormName(LookupFormByID(formId));
}

#endif

std::string& ToLower(std::string&& data)
{
	ra::transform(data, data.begin(), [](const unsigned char c) { return game_tolower(c); });
	return data;
}

std::string& StripSpace(std::string&& data)
{
	std::erase_if(data, isspace);
	return data;
}

bool StartsWith(std::string left, std::string right)
{
	return ToLower(std::move(left)).starts_with(ToLower(std::move(right)));
}

std::vector<std::string> SplitString(std::string s, std::string delimiter)
{
	size_t pos_start = 0, pos_end, delim_len = delimiter.length();
	std::string token;
	std::vector<std::string> res;

	while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos) {
		token = s.substr(pos_start, pos_end - pos_start);
		pos_start = pos_end + delim_len;
		res.push_back(token);
	}

	res.push_back(s.substr(pos_start));
	return res;
}

UInt8* GetParentBasePtr(void* addressOfReturnAddress, bool lambda)
{
	auto* basePtr = static_cast<UInt8*>(addressOfReturnAddress) - 4;
#if _DEBUG
	if (lambda) // in debug mode, lambdas are wrapped inside a closure wrapper function, so one more step needed
		basePtr = *reinterpret_cast<UInt8**>(basePtr);
#endif
	return *reinterpret_cast<UInt8**>(basePtr);
}