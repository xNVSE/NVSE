#pragma once

class BSAudioListener
{
public:
	BSAudioListener();
	~BSAudioListener();

	virtual BSAudioListener* Destroy(bool doFree);
	virtual void	Unk_01(void);
	virtual void	Unk_02(void);
	virtual void	Unk_03(void);
	virtual void	Unk_04(void);
	virtual void	Unk_05(void);
	virtual void	Unk_06(void);
	virtual void	Unk_07(void);
	virtual void	Unk_08(void);
	virtual void	Unk_09(void);
	virtual void	Unk_0A(void);
	virtual void	Unk_0B(void);
	virtual void	Unk_0C(void);
};

// 64
class BSWin32AudioListener : public BSAudioListener
{
public:
	BSWin32AudioListener();
	~BSWin32AudioListener();

	UInt32			unk04[14];		// 04
	float			flt3C;			// 3C
	UInt32			unk40[9];		// 40
};

// 230
class BSGameSound
{
public:
	BSGameSound();
	~BSGameSound();

	virtual BSGameSound* Destroy(bool doFree);
	virtual void	Unk_01(void);
	virtual void	Unk_02(void);
	virtual void	Unk_03(void);
	virtual void	Unk_04(void);
	virtual void	SetPaused(bool doSet);
	virtual void	Unk_06(void);
	virtual void	Unk_07(void);
	virtual void	Unk_08(void);
	virtual void	Unk_09(void);
	virtual void	Unk_0A(void);
	virtual void	Unk_0B(void);
	virtual void	Unk_0C(void);
	virtual bool	Unk_0D(void);
	virtual bool	Unk_0E(void);
	virtual bool	Unk_0F(float arg1);
	virtual void	Unk_10(void);
	virtual bool	Unk_11(void);
	virtual void	Unk_12(void);
	virtual void	Unk_13(float arg1, float arg2, float arg3);
	virtual void	Unk_14(float arg1, float arg2, float arg3);
	virtual void	Unk_15(NiVector3& arg1);
	virtual void	Unk_16(void);
	virtual void	Unk_17(float arg1, float arg2);
	virtual void	Unk_18(UInt16 arg1, UInt16 arg2, UInt16 arg3, UInt16 arg4, UInt16 arg5);
	virtual bool	Unk_19(float arg1);
	virtual float	Unk_1A(void);
	virtual void	Seek(UInt32 timePoint);

	UInt32			mapKey;					// 004
	UInt32			flags008;				// 008
	UInt32			unk00C;					// 00C
	UInt32			flags010;				// 010
	UInt32			duration;				// 014
	UInt16			staticAttenuation;		// 018	dB * -1000
	UInt16			unk01A;					// 01A
	UInt16			unk01C;					// 01C
	UInt16			unk01E;					// 01E
	UInt16			unk020;					// 020
	UInt16			unk022;					// 022
	float			flt024;					// 024
	float			flt028;					// 028
	float			flt02C;					// 02C
	UInt32			unk030;					// 030
	UInt16			unk034;					// 034
	char			filePath[MAX_PATH];		// 036
	UInt16			unk13A;					// 13A
	float			flt13C;					// 13C
	float			flt140;					// 140
	UInt32			unk144[3];				// 144
	float			flt150;					// 150
	UInt32			unk154[18];				// 154
	UInt32* unk19C;				// 19C
	UInt32			unk1A0[26];				// 1A0
	UInt32* unk208;				// 208
	UInt32			unk20C[9];				// 20C
};

// ??
class BSWin32Audio
{
public:
	BSWin32Audio();
	~BSWin32Audio();

	virtual void		 Destroy(bool doFree);
	virtual void		 Unk_01(void);
	virtual void		 Unk_02(void);
	virtual void	 	 Unk_03(void);
	virtual void		 Unk_04(void);
	virtual BSGameSound* CreateGameSound(const char* filePath);
	virtual void		 Unk_06(void);
	virtual void		 Unk_07(void);

	UInt32					unk004[3];		// 004
	BSWin32AudioListener*   listener;		// 010
	UInt32					unk014[3];		// 014
	bool					(*sub_82D150)(UInt32*, UInt32*, UInt32*, UInt32*);	// 020
	bool					(*sub_82D280)(UInt32*, UInt32*, UInt32*, UInt32*);	// 024
	bool					(*sub_5E3630)(UInt32*);	// 028
	UInt32					(*sub_82D400)(UInt32*, TESSound*, UInt32*);	// 02C
	void					(*sub_832C40)(void);	// 030
	void					(*sub_832C80)(void);	// 034

	static BSWin32Audio* GetSingleton() { return *(BSWin32Audio**)0x11F6D98; };
};

// 0C
struct Sound
{
	UInt32 unk00;
	UInt8 byte04;
	UInt8 pad05[3];
	UInt32 unk08;

	Sound() : unk00(0xFFFFFFFF), byte04(0), unk08(0) {}


	Sound(const char* soundPath, UInt32 flags)
	{
		// ThisStdCall(0xAD7550, *(BSWin32Audio**)0x11F6D98, this, soundPath, flags);
		ThisStdCall(0xAD7550, BSWin32Audio::GetSingleton(), this, soundPath, flags);
	}

	void Play()
	{
		ThisStdCall(0xAD8830, this, 0);
	}
};