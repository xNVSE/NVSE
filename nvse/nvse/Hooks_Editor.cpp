#include "Hooks_Editor.h"
#include "SafeWrite.h"
#include "GameTypes.h"
#include "richedit.h"

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

#if 0
static void FixErrorReportBug(void)
{
	// bethesda passes strings containing user input to printf-like functions
	// this causes crashes when the user input contains tokens printf is interested in
	// so we fix it

	// move the entire block of code before the call to printf down, add a new argument pointing to "%s"

	const UInt32	kBlockMoveDelta =	5;

#error	const UInt32	kBlockMoveSrc =		0x00500001;							// inside ShowCompilerError, one past last referebnce
#error	const UInt32	kBlockMoveDst =		kBlockMoveSrc + kBlockMoveDelta;
#error	const UInt32	kBlockMoveSize =	0x00500035 - kBlockMoveSrc;
#error	const UInt32	kFormatStrPos =		0x0092BBE4;	// "%s"
#error	const UInt32	kStackFixupPos =	0x0B + 2;
#error	const UInt32	kPCRelFixups[] =	{ 0x00 + 1, 0x06 + 1, 0x28 + 1 };

	UInt8	tempBuf[kBlockMoveSize];

	memcpy(tempBuf, (void *)kBlockMoveSrc, kBlockMoveSize);

	// jumps/calls are pc-relative, we're moving code so fix them up
	*((UInt32 *)&tempBuf[kPCRelFixups[0]]) -= kBlockMoveDelta;
	*((UInt32 *)&tempBuf[kPCRelFixups[1]]) -= kBlockMoveDelta;
	*((UInt32 *)&tempBuf[kPCRelFixups[2]]) -= kBlockMoveDelta;

	// added a new arg, so we need to clean it off the stack
	tempBuf[kStackFixupPos] += 4;

	SafeWriteBuf(kBlockMoveDst, tempBuf, kBlockMoveSize);

	SafeWrite8(kBlockMoveSrc + 0, 0x68);	// push "%s"
	SafeWrite32(kBlockMoveSrc + 1, kFormatStrPos);
}
#endif

void CreateTokenTypedefs(void)
{
char** tokenAlias_Float = (char**)0x00E9BE9C;	//reserved for array variable	// Find "Unknown variable or function '%s'.", previous call, first 59h case is an array containng reserved words
char** tokenAlias_Long	= (char**)0x00E9BE74;	//string variable

	*tokenAlias_Long = "string_var";
	*tokenAlias_Float = "array_var";
}

#if 0 // Already done in Commands_table
static void PatchConditionalCommands(void)
{
	// editor will not accept commands outside of the vanilla opcode range
	// for use as conditionals in dialog/packages/quests/etc
	// nop out a conditional branch to fix

#error	SafeWrite16(0x00457DD0, 0x9090);
}
#endif

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

#endif
