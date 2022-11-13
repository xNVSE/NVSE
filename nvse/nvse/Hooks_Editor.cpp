#include "Hooks_Editor.h"

#include "Commands_Scripting.h"
#include "GameAPI.h"
#include "SafeWrite.h"
#include "richedit.h"
#include "ScriptUtils.h"
#include <regex>

#include "Hooks_Script.h"
#if EDITOR

static char s_InfixToPostfixBuf[0x800];
static const UInt32 kInfixToPostfixBufLen = sizeof(s_InfixToPostfixBuf);

static const UInt32 kInfixToPostfixHookAddr_1 =	0x005B1AD9;
static const UInt32 kInfixToPostfixRetnAddr_1 =	0x005B1ADE;

static const UInt32 kInfixToPostfixHookAddr_2 =	0x005B1C6C;
static const UInt32 kInfixToPostfixRetnAddr_2 =	0x005B1C73;

static __declspec(naked) void InfixToPostfixHook_1(void)
{
	__asm {
		lea	eax, s_InfixToPostfixBuf
		push eax
		jmp	[kInfixToPostfixRetnAddr_1]
	}
}

static __declspec(naked) void InfixToPostfixHook_2(void)
{
	__asm {
		lea	edx, s_InfixToPostfixBuf
		mov	byte ptr [edi], 0x20
		jmp	[kInfixToPostfixRetnAddr_2]
	}
}

static void FixInfixToPostfixOverflow(void)
{
	// this function uses a statically allocated 64 byte work buffer that overflows really quickly
	// hook replaces the buffer, safe to use a static buffer because it isn't re-entrant and can't be called from multiple threads
	WriteRelJump(kInfixToPostfixHookAddr_1, (UInt32)&InfixToPostfixHook_1);
	WriteRelJump(kInfixToPostfixHookAddr_2, (UInt32)&InfixToPostfixHook_2);
}

void Hook_Editor_Init(void)
{
	FixInfixToPostfixOverflow();
}

#include "common/IThread.h"
#include "common/IFileStream.h"
#include "Utilities.h"

class EditorHookWindow : public ISingleton <EditorHookWindow>
{
public:
	EditorHookWindow();
	~EditorHookWindow();

	void	PumpEvents(void);

private:
	LRESULT					WindowProc(UINT msg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK	_WindowProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam);

	void	OnButtonHit(void);

	HWND			m_window;
	volatile bool	m_done;

	HWND	m_button;
	HWND	m_editText, m_editText2;
};

EditorHookWindow::EditorHookWindow()
:m_window(NULL), m_button(NULL), m_editText(NULL), m_editText2(NULL)
{
	// register our window class
	WNDCLASS	windowClass =
	{
		0,								// style
		_WindowProc,					// window proc
		0, 0,							// no extra memory required
		GetModuleHandle(NULL),			// instance
		NULL, NULL,						// icon, cursor
		(HBRUSH)(COLOR_BACKGROUND + 1),	// background brush
		NULL,							// menu name
		"EditorHookWindow"				// class name
	};

	ATOM	classAtom = RegisterClass(&windowClass);
	ASSERT(classAtom);

	CreateWindow(
		(LPCTSTR)classAtom,				// class
		"NVSE",							// name
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,	// style
		0, 0,							// x y
		300, 300,						// width height
		NULL,							// parent
		NULL,							// menu
		GetModuleHandle(NULL),			// instance
		(LPVOID)this);
}

EditorHookWindow::~EditorHookWindow()
{
	if(m_window)
		DestroyWindow(m_window);
}

void EditorHookWindow::PumpEvents(void)
{
	MSG	msg;

	m_done = false;

	while(!m_done)
	{
		BOOL	result = GetMessage(&msg, m_window, 0, 0);
		if(result == -1)
		{
			_MESSAGE("message pump error");
			break;
		}

		TranslateMessage(&msg);
		DispatchMessage(&msg);

		if(result == 0)
			break;
	}
}

LRESULT EditorHookWindow::WindowProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
//	_MESSAGE("windowproc: %08X %08X %08X", msg, wParam, lParam);

	switch(msg)
	{
		case WM_CREATE:
			m_button = CreateWindow(
				"BUTTON",
				"Push Button",	// receive bacon
				WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
				0, 0,
				100, 30,
				m_window,
				NULL,
				GetModuleHandle(NULL),
				NULL);
			m_editText = CreateWindow(
				"EDIT",
				NULL,
				WS_CHILD | WS_VISIBLE | ES_LEFT,
				0, 30,
				100, 30,
				m_window,
				NULL,
				GetModuleHandle(NULL),
				NULL);
			m_editText2 = CreateWindow(
				"EDIT",
				NULL,
				WS_CHILD | WS_VISIBLE | ES_LEFT,
				110, 30,
				100, 30,
				m_window,
				NULL,
				GetModuleHandle(NULL),
				NULL);

			ASSERT(m_button && m_editText);
			break;

		case WM_COMMAND:
		{
			HWND	source = (HWND)lParam;
			if(source == m_button)
			{
				OnButtonHit();
			}
		}
		break;

		case WM_DESTROY:
			m_done = true;
			break;
	};

	return DefWindowProc(m_window, msg, wParam, lParam);
}

LRESULT EditorHookWindow::_WindowProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam)
{
	EditorHookWindow	* _this = NULL;

	if(msg == WM_CREATE)
	{
		CREATESTRUCT	* info = (CREATESTRUCT *)lParam;
		_this = (EditorHookWindow *)info->lpCreateParams;

		SetWindowLongPtr(window, GWLP_USERDATA, (LONG_PTR)_this);

		_this->m_window = window;
	}
	else
	{
		_this = (EditorHookWindow *)GetWindowLongPtr(window, GWLP_USERDATA);
	}

	LRESULT	result;

	if(_this)
		result = _this->WindowProc(msg, wParam, lParam);
	else
		result = DefWindowProc(window, msg, wParam, lParam);

	return result;
}

//typedef void * (* _LookupFormByID)(UInt32 id);
//const _LookupFormByID LookupFormByID = (_LookupFormByID)0x00000000;

void EditorHookWindow::OnButtonHit(void)
{
	char	text[256];
	GetWindowText(m_editText, text, sizeof(text));

	char	comment[256];
	GetWindowText(m_editText2, comment, sizeof(comment));

	UInt32	id;
	if(sscanf_s(text, "%x", &id) == 1)
	{
		void	* ptr = LookupFormByID(id);
		
		sprintf_s(text, sizeof(text), "%08X = %08X (%s)", id, (UInt32)ptr, GetObjectClassName(ptr));
		_MESSAGE("%s", text);

		MessageBox(m_window, text, "receive bacon", MB_OK);

		static int idx = 0;
		char	fileName[256];
		if(comment[0])
			sprintf_s(fileName, sizeof(fileName), "mem%08X_%08X_%08X_%s", idx, id, (UInt32)ptr, comment);
		else
			sprintf_s(fileName, sizeof(fileName), "mem%08X_%08X_%08X", idx, id, (UInt32)ptr);
		idx++;

		IFileStream	dst;
		if(dst.Create(fileName))
			dst.WriteBuf(ptr, 0x200);
	}
	else
	{
		MessageBox(m_window, "couldn't read text box", "receive bacon", MB_OK);
	}
};

static IThread	hookWindowThread;

static void HookWindowThread(void * param)
{
	_MESSAGE("create hook window");
	EditorHookWindow	* window = new EditorHookWindow;

	window->PumpEvents();

	_MESSAGE("hook window closed");

	delete window;
}

void CreateHookWindow(void)
{
	hookWindowThread.Start(HookWindowThread);
}

static HANDLE	fontHandle;
static LOGFONT	fontInfo;
static bool		userSetFont = false;

static void DoModScriptWindow(HWND wnd)
{
	SendMessage(wnd, EM_EXLIMITTEXT, 0, 0x00FFFFFF);

	bool	setFont = false;

	if(GetAsyncKeyState(VK_F11))
	{
		LOGFONT		newFontInfo = fontInfo;
		CHOOSEFONT	chooseInfo = { sizeof(chooseInfo) };

		chooseInfo.lpLogFont = &newFontInfo;
		chooseInfo.Flags = CF_INITTOLOGFONTSTRUCT | CF_NOVERTFONTS | CF_SCREENFONTS;

		if(ChooseFont(&chooseInfo))
		{
			HANDLE	newFont = CreateFontIndirect(&newFontInfo);
			if(newFont)
			{
				DeleteObject(fontHandle);

				fontInfo = newFontInfo;
				fontHandle = newFont;
			}
			else
				_WARNING("couldn't create font");
		}

		setFont = true;
	}

	if(GetAsyncKeyState(VK_F12) || setFont || userSetFont)
	{
		userSetFont = true;
		SendMessage(wnd, EM_SETTEXTMODE, TM_PLAINTEXT, 0);
		SendMessage(wnd, WM_SETFONT, (WPARAM)fontHandle, 1);

		UInt32	tabStopSize = 16;
		SendMessage(wnd, EM_SETTABSTOPS, 1, (LPARAM)&tabStopSize);	// one tab stop every 16 dialog units
	}
}

static __declspec(naked) void ModScriptWindow() {
	__asm {
		pushad
		push eax
		call DoModScriptWindow
		add esp, 4
		popad
		pop ecx			// return address
		push 0x0B0001
		//push 0x00
		push ecx
		ret
	}
}

static UInt32 pModScriptWindow = (UInt32)&ModScriptWindow;

// copied from a call to ChooseFont
static const LOGFONT kLucidaConsole9 =
{
	-12,	// height
	0,		// width
	0,		// escapement
	0,		// orientation
	400,	// weight
	0,		// italic
	0,		// underline
	0,		// strikeout
	0,		// charset
	3,		// out precision
	2,		// clip precision
	1,		// quality
	49,		// pitch and family
	"Lucida Console"
};

void FixEditorFont(void)
{
	// try something nice, otherwise fall back on SYSTEM_FIXED_FONT
	fontHandle = CreateFontIndirect(&kLucidaConsole9);
	if(fontHandle)
	{
		fontInfo = kLucidaConsole9;
	}
	else
	{
		fontHandle = GetStockObject(SYSTEM_FIXED_FONT);
		GetObject(fontHandle, sizeof(fontInfo), &fontInfo);
	}

	UInt32	basePatchAddr = 0x005C4331;					// Right after call d:CreateWindowA after aRichedit20a

	WriteRelCall(basePatchAddr,	pModScriptWindow);
	//SafeWrite8(basePatchAddr + 6,	0x90);
}

void CreateTokenTypedefs(void)
{
	auto* tokenAlias_Float = reinterpret_cast<const char**>(0x00E9BE9C);	//reserved for array variable	// Find "Unknown variable or function '%s'.", previous call, first 59h case is an array containng reserved words
	auto* tokenAlias_Long	= reinterpret_cast<const char**>(0x00E9BE74);	//string variable

	*tokenAlias_Long = "string_var";
	*tokenAlias_Float = "array_var";
}

static UInt32 ScriptIsAlpha_PatchAddr = 0x00C61730; // Same starting proc as tokenAlias_Float. Follow failed test for '"', enter first call. Replace call.
static UInt32 IsAlphaCallAddr = 0x00C616B3;
static UInt32 ScriptCharTableAddr = 0x00E12270;		// Table classyfing character for scripts. Replace IsAlpha

static void __declspec(naked) ScriptIsAlphaHook(void)
{
	__asm
	{
		mov al, byte ptr [esp+4]	// arg to isalpha()

		cmp al, '['					// brackets used for array indexing
		jz AcceptChar
		cmp al, ']'
		jz AcceptChar
		cmp al, '$'					// $ used as prefix for string variables
		jz AcceptChar

		jmp IsAlphaCallAddr			// else use default behavior

	AcceptChar:
		mov eax, 1
		retn
	}
}

void PatchIsAlpha()
{
	WriteRelCall(ScriptIsAlpha_PatchAddr, (UInt32)ScriptIsAlphaHook);
	SafeWrite8(ScriptCharTableAddr+'['*2, 2);
	SafeWrite8(ScriptCharTableAddr+']'*2, 2);
	SafeWrite8(ScriptCharTableAddr+'$'*2, 2);
}

__declspec(naked) void CommandParserScriptVarHook()
{
	__asm
	{
		cmp		dword ptr [ebp], 0x2D
		jnz		onError
		test	edx, edx
		jbe		notRef
		cmp		cl, 'G'
		jz		onError
		mov		edx, [esi+0x40C]
		mov		byte ptr [esi+edx+0x20C], 'r'
		inc		dword ptr [esi+0x40C]
		mov		eax, [esi+0x40C]
		mov		cx, [esp+0x228]
		mov		[esi+eax+0x20C], cx
		add		dword ptr [esi+0x40C], 2
		mov		cl, [esp+0x22C]
		cmp		cl, 'G'
		jz		onError
	notRef:
		cmp		dword ptr [esp+0x234], 0
		jbe		onError
		mov		edx, [esi+0x40C]
		mov		[esi+edx+0x20C], cl
		inc		dword ptr [esi+0x40C]
		mov		eax, [esi+0x40C]
		mov		cx, [esp+0x234]
		mov		[esi+eax+0x20C], cx
		mov		eax, 0x5C7E95
		jmp		eax
	onError:
		mov		eax, 0x5C7F11
		jmp		eax
	}
}
#endif
ParamParenthResult __fastcall HandleParameterParenthesis(ScriptLineBuffer* scriptLineBuffer, ScriptBuffer* scriptBuffer, ParamInfo* paramInfo, UInt32 paramIndex)
{
	auto parser = ExpressionParser(scriptBuffer, scriptLineBuffer);
	return parser.ParseParentheses(paramInfo, paramIndex);
}
#if EDITOR
__declspec(naked) void ParameterParenthesisHook()
{
	const static auto stackOffset = 0x264;
	
	const static auto paramIndexLoc = stackOffset - 0x230;
	const static auto paramInfoLoc = stackOffset + 0x4 + 0x8;
	const static auto scriptBufLoc = stackOffset + 0x8 + 0x10;
	const static auto lineBufLoc = stackOffset + 0x8 + 0xC;

	const static auto fnParseScriptWord = 0x5C6190;
	const static auto continueLoop = 0x5C7E9C;
	const static auto returnAddress = 0x5C68C5;
	const static auto prematureReturn = 0x5C7F1E;
	__asm
	{
		mov eax, paramIndexLoc
		mov ecx, [esp + eax]
		push ecx
		
		mov eax, paramInfoLoc
		mov ecx, [esp + eax]
		push ecx

		mov eax, scriptBufLoc
		mov edx, [esp + eax]
		
		mov eax, lineBufLoc
		mov ecx, [esp + eax]
		
		call HandleParameterParenthesis
		cmp al, [kParamParent_NoParam]
		je notParenthesis

		add esp, 0x24
		cmp al, [kParamParent_SyntaxError]
		je syntaxError
		jmp continueLoop
	syntaxError:
		mov al, 0
		jmp	prematureReturn
	notParenthesis:
		call fnParseScriptWord
		jmp returnAddress
	}
}
#endif
std::string ReplaceAll(std::string str, const std::string& from, const std::string& to) {
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
	}
	return str;
}

bool StrContains(const std::string& str, const std::string& subStr)
{
	 return str.find(subStr) != std::string::npos;
}

std::vector g_lineMacros =
{
	ScriptLineMacro([&](std::string& line, ScriptBuffer*, ScriptLineBuffer*)
	{
		static const std::vector<std::pair<std::string, std::string>> s_shortHandMacros =
		{
			std::make_pair(":=", R"(\=)"),
			std::make_pair(":=", R"(\:\=)"),
			std::make_pair("+=", R"(\+\=)"),
			std::make_pair("-=", R"(\-\=)"),
			std::make_pair("*=", R"(\*\=)"),
			std::make_pair("/=", R"(\/\=)"),
			std::make_pair("|=", R"(\|\=)"),
			std::make_pair("&=", R"(\&\=)"),
			std::make_pair("%=", R"(\%\=)"),
		};
		std::string optVarTypeDecl; // int, ref etc in int iVar = 10 for example
		for (auto& varType : g_validVariableTypeNames)
		{
			if (_stricmp(line.substr(0, strlen(varType)).c_str(), varType) == 0 && isspace(line[strlen(varType)]))
			{
				optVarTypeDecl = varType;
				line.erase(0, optVarTypeDecl.size());
				optVarTypeDecl += ' ';
				while (line.begin() != line.end() && isspace(*line.begin()))
					line.erase(line.begin());
				break;
			}
		}

#if 0   // code to remove unnecessary spaces from inside array brackets.
		// TODO: Needs refinement, as currently it removes needed spaces for an inline expression inside brackets.
		// Ex: it mangles "aTest[(Rand 1, 3)] = 1" into "aTest[(Rand1,3)] = 1", which no longer compiles.
		const auto arrLeftBracketPos = line.find_first_of('[');
		if (arrLeftBracketPos != std::string::npos)
		{
			const auto arrRightBracketPos = line.find_first_of(']');
			if (arrRightBracketPos != std::string::npos && arrRightBracketPos > arrLeftBracketPos)
			{
				const auto end = line.begin() + arrRightBracketPos;
				line.erase(std::remove_if(line.begin() + arrLeftBracketPos, end, isspace), end);
			}
		}
#endif
		
		for (const auto& [realOp, regexOp] : s_shortHandMacros)
		{
			// VARIABLE = VALUE macro
			// Variable type isn't considered for the regex.
			// Unit tests for the full regex here: https://regex101.com/r/ybBkK8

			// Ex1: Match "ivar = 4"
			// Ex2: Match "SomeQuest.aTest[(Rand 1, 3)] = "SomeString"

			// Right-side value will be matched in regex group #3
			// This string composes the majority of groups #2 and #3.
			std::string matchAnyNVSEExpression =
				R"([\w\s\$\!\-\(\{][\.\s\S]*)";

			// Left-side variable/array element will be matched in regex group #1
			// Allow any NVSE expression to go inside array "[]" operator - this is group #2.
			std::string regExGroup1And2 = 
				R"(^([a-zA-Z]|\w[\w\.]*[\w](\[)"
				+ matchAnyNVSEExpression + R"(\])*)\s*)";

			const std::regex assignmentExpr(regExGroup1And2 + regexOp + '(' + matchAnyNVSEExpression + ')');
			if (std::smatch m; std::regex_search(line, m, assignmentExpr, std::regex_constants::match_continuous) 
				&& m.size() == 4)
			{
				line = "let " + optVarTypeDecl + m.str(1) + " " + realOp + " " + m.str(3);
				return true;
			}
		}
		return false;
	}, MacroType::AssignmentShortHand),
#if 0
	ScriptLineMacro([&](std::string& line, ScriptBuffer*, ScriptLineBuffer*)
	{
		for (auto [match, toReplace] : {std::make_pair("ife", "if eval "), std::make_pair("elseife", "elseif eval ")})
		{
			if (line.starts_with(match) && isspace((unsigned char)line[strlen(match)]))
			{
				line.erase(0,strlen(match)+1);
				line.insert(0, toReplace);
				return true;
			}
		}
		return false;
	}, MacroType::IfEval)
#endif
	ScriptLineMacro([&](std::string& line, ScriptBuffer* scriptBuf, ScriptLineBuffer* lineBuf)
	{
		if (auto iter = ra::find_if(g_validVariableTypeNames, _L(const char* typeName, StartsWith(line, std::string(typeName) + " "))); 
			iter != std::end(g_validVariableTypeNames))
		{
			const auto* typeName = *iter;
			const auto str = std::string(line).substr(strlen(typeName)+1);
			auto tokens = SplitString(str, ",");
			if (tokens.size() <= 1)
				return false;
			ra::for_each(tokens, _L(auto& token, token = StripSpace(std::string(token))));
			if (!ra::all_of(tokens, _L(auto& token, ra::all_of(token, _L(char c, isalnum(c) || c == '_')))))
				return false;
			const auto firstVar = tokens.at(0);
			tokens.erase(tokens.begin()); 
			line = std::string(typeName) + ' ' + firstVar;
			auto* currentScript = g_currentScriptStack.top();
#if EDITOR
			auto* errorBuf = scriptBuf;
#else
			auto* errorBuf = lineBuf;
#endif

			ra::for_each(tokens,
			_L(auto &varName,
				CreateVariable(currentScript, scriptBuf, varName, VariableTypeNameToType(typeName),
					_L(auto &msg, ShowCompilerError(errorBuf, "%s", msg.c_str())))));
		}
		return false;
	}, MacroType::MultipleVariableDeclaration)
};

bool HandleLineBufMacros(ScriptLineBuffer* line, ScriptBuffer* buffer)
{
	for (const auto& macro : g_lineMacros)
		if (macro.EvalMacro(line, buffer) == ScriptLineMacro::MacroResult::Error)
			return false;
	return true;
}

static UInt32 g_nextLineNumberIncrement = 0;

// Expand ScriptLineBuffer to allow multiline expressions with parenthesis
int ParseNextLine(ScriptBuffer* scriptBuf, ScriptLineBuffer* lineBuf)
{
#if EDITOR
	auto* errorBuf = scriptBuf;
#else
	auto* errorBuf = lineBuf;
#endif
	const auto lineError = [&](const char* str)
	{
		g_nextLineNumberIncrement = 0;
		scriptBuf->curLineNumber = lineBuf->lineNumber;
		ShowCompilerError(errorBuf, "%s", str);
		lineBuf->errorCode = 1;
		return 0;
	};
	lineBuf->paramTextLen = 0;
	memset(lineBuf->paramText, '\0', sizeof lineBuf->paramText);
	
	const auto* curScriptText = reinterpret_cast<unsigned char*>(&scriptBuf->scriptText[scriptBuf->textOffset]);
	const auto* oldScriptText = curScriptText;
	
	auto numBrackets = 0;
	auto capturedNonSpace = false;
	auto numNewLinesInParenthesis = 0;
	auto inStringLiteral = false;
	auto inMultilineComment = false;

	// in GECK code: `scriptLineBuf->lineNumber = scriptBuf->curLineNumber + 1;` before this function is called
	// must be done here for the last line due to that.
	lineBuf->lineNumber += g_nextLineNumberIncrement;
	g_nextLineNumberIncrement = 0;

	// skip all spaces and tabs in the beginning
	while (isspace(*curScriptText))
	{
		if (*curScriptText == '\n')
			++lineBuf->lineNumber;
		++curScriptText;
	}

	unsigned char lastChar = '\0';
	while (true)
	{
		const auto curTwoChars = *reinterpret_cast<const UInt16*>(curScriptText);
		if (curTwoChars == '*/' && !inStringLiteral && !inMultilineComment)
		{
			inMultilineComment = true;
			curScriptText += 2;
			continue;
		}
		if (curTwoChars == '/*' && !inStringLiteral && inMultilineComment)
		{
			inMultilineComment = false;
			curScriptText += 2;
			continue;
		}

		const auto curChar = *curScriptText++;
		if (curChar == 0)
		{
			if (inMultilineComment)
				return lineError("Mismatched comment block (missing '*/' for a present '/*')");
			if (inStringLiteral)
				return lineError("Mismatched quotes. A string literal was not closed.");
			if (numBrackets)
				return lineError("Mismatched parentheses. Parentheses were not closed.");
		}
		if (inMultilineComment)
		{
			if (curChar == '\n')
				++lineBuf->lineNumber;
			continue;
		}
		switch (curChar)
		{
			case '(':
			{
				if (!inStringLiteral)
					++numBrackets;
				break;
			}
			case ')':
			{
				if (!inStringLiteral)
				{
					--numBrackets;
					if (numBrackets < 0)
						return lineError("Mismatched parenthesis");
				}
				break;
			}
			case '"':
			{
				inStringLiteral = !inStringLiteral;
				break;
			}
			case '\0':
			{
				--curScriptText;
				if (numBrackets)
					return lineError("Mismatched parenthesis");
				if (!capturedNonSpace)
					return 0;
				[[fallthrough]];
			}
			case '\n':
			{
				if (numBrackets == 0 && capturedNonSpace)
				{
					auto* curLineText = reinterpret_cast<unsigned char*>(&lineBuf->paramText[lineBuf->paramTextLen - 1]);
					while (isspace(*curLineText))
					{
						*curLineText-- = '\0';
						--lineBuf->paramTextLen;
					}
					lineBuf->paramText[lineBuf->paramTextLen] = '\0';
					//lineBuf->lineNumber += numNewLinesInParenthesis;
					g_nextLineNumberIncrement = numNewLinesInParenthesis;
					if (!HandleLineBufMacros(lineBuf, scriptBuf))
						return 0;
					return curScriptText - oldScriptText;
				}
				if (numBrackets)
					++numNewLinesInParenthesis;
				else if (!capturedNonSpace)
					++lineBuf->lineNumber;
				break;
			}
			case ';':
			{
				while (*curScriptText && *curScriptText != '\n') 
					++curScriptText;
				continue;
			}
			default:
			{
				if (isspace(lastChar) && isspace(curChar) && !inStringLiteral)
					continue;
				break;
			}
		}
		if (const auto maxLen = sizeof lineBuf->paramText; lineBuf->paramTextLen >= maxLen)
		{
			if (numBrackets)
				ShowCompilerError(errorBuf, "Max script expression length inside parenthesis (%d characters) exceeded.", maxLen);
			else
				ShowCompilerError(errorBuf, "Max script line length (%d characters) exceeded.", maxLen);
			lineBuf->errorCode = 16;
			return 0;
		}
		if (!isspace(curChar)) 
			capturedNonSpace = true;
		if (capturedNonSpace)
			lineBuf->paramText[lineBuf->paramTextLen++] = curChar;
		lastChar = curChar;
	}
}
#if EDITOR
void PatchDefaultCommandParser()
{
	//	Handle kParamType_Double
	SafeWrite8(0x5C830C, 1);
	*(UInt16*)0xE9C1DC = 1;

	//	Handle kParamType_ScriptVariable
	SafeWrite32(0x5C82DC, (UInt32)CommandParserScriptVarHook);
	*(UInt8*)0xE9C1E4 = 1;

	//	Replace DefaultCommandParser
	// WriteRelJump(0x5C67E0, (UInt32)DefaultCommandParseHook);

	// Brackets in Param to NVSE parser
	WriteRelJump(0x5C68C0, UInt32(ParameterParenthesisHook));

	// Allow multiline expressions with parenthesis
	WriteRelJump(0x5C5830, UInt32(ParseNextLine));
}

#else

const auto* g_arrayVar = "array_var";
const auto* g_stringVar = "string_var";

__declspec(naked) void InlineExpressionHook()
{
	const static auto fnParseScriptWord = 0x5AF5F0;
	const static auto continueLoop = 0x5B1BFD;
	const static auto returnAddress = 0x5B1C6A;
	const static auto prematureReturn = 0x5B3AB4;

	__asm
	{
		movzx edx, word ptr [ebp - 0x8] // index
		push edx
		mov ecx, [ebp+0xC] // paramInfo
		push ecx
		mov edx, [ebp+0x14] // scriptBuffer
		mov ecx, [ebp+0x10] // lineBuf
		call HandleParameterParenthesis

		cmp al, [kParamParent_NoParam]
		je notParenthesis
		add esp, 0x18
		cmp al, [kParamParent_SyntaxError]
		je syntaxError
		jmp continueLoop
	syntaxError:
		mov al, 0
		jmp	prematureReturn
	notParenthesis:
		call fnParseScriptWord
		jmp returnAddress
	}
}

void PatchGameCommandParser()
{
	// Allow multiline expressions with parenthesis
	WriteRelJump(0x5AF3F0, reinterpret_cast<UInt32>(ParseNextLine));

	const auto tokenAliasFloat = 0x118CBF4;
	const auto tokenAliasLong = 0x118CBCC;
	SafeWrite32(tokenAliasFloat, reinterpret_cast<UInt32>(g_arrayVar));
	SafeWrite32(tokenAliasLong, reinterpret_cast<UInt32>(g_stringVar));
	WriteRelJump(0x5B1C65, reinterpret_cast<UInt32>(InlineExpressionHook));

}

#endif

constexpr bool isEditor()
{
#if EDITOR
	return true;
#else
	return false;
#endif
}

bool __fastcall retnTrue()
{
	return true;
}

void PatchDisable_ScriptBufferValidateRefVars(bool disable)
{
	if (disable)
	{
		WriteRelCall(isEditor() ? 0x5C97CF : 0x5AED78, retnTrue);
	}
	else
	{
		WriteRelCall(isEditor() ? 0x5C97CF : 0x5AED78, isEditor() ? 0x5C5980 : 0x5AFA50);
	}
}