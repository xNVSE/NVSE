#include "IdentifyEXE.h"
#include "Error.h"
#include "nvse/nvse_version.h"

static bool GetFileVersion(const char * path, VS_FIXEDFILEINFO * info)
{
	bool result = false;

	UInt32	versionSize = GetFileVersionInfoSize(path, NULL);
	if(!versionSize)
	{
		_ERROR("GetFileVersionInfoSize failed (%08X)", GetLastError());
		return false;
	}

	UInt8	* versionBuf = new UInt8[versionSize];
	if(versionBuf)
	{
		if(GetFileVersionInfo(path, NULL, versionSize, versionBuf))
		{
			VS_FIXEDFILEINFO	* retrievedInfo = NULL;
			UInt32				realVersionSize = sizeof(VS_FIXEDFILEINFO);

			if(VerQueryValue(versionBuf, "\\", (void **)&retrievedInfo, (PUINT)&realVersionSize) && retrievedInfo)
			{
				*info = *retrievedInfo;
				result = true;
			}
			else
			{
				_ERROR("VerQueryValue failed (%08X)", GetLastError());
			}
		}
		else
		{
			_ERROR("GetFileVersionInfo failed (%08X)", GetLastError());
		}

		delete [] versionBuf;
	}

	return result;
}

static bool GetFileVersionNumber(const char * path, UInt64 * out)
{
	VS_FIXEDFILEINFO	versionInfo;
	if(!GetFileVersion(path, &versionInfo))
	{
		return false;
	}

	_MESSAGE("dwSignature = %08X", versionInfo.dwSignature);
	_MESSAGE("dwStrucVersion = %08X", versionInfo.dwStrucVersion);
	_MESSAGE("dwFileVersionMS = %08X", versionInfo.dwFileVersionMS);
	_MESSAGE("dwFileVersionLS = %08X", versionInfo.dwFileVersionLS);
	_MESSAGE("dwProductVersionMS = %08X", versionInfo.dwProductVersionMS);
	_MESSAGE("dwProductVersionLS = %08X", versionInfo.dwProductVersionLS);
	_MESSAGE("dwFileFlagsMask = %08X", versionInfo.dwFileFlagsMask);
	_MESSAGE("dwFileFlags = %08X", versionInfo.dwFileFlags);
	_MESSAGE("dwFileOS = %08X", versionInfo.dwFileOS);
	_MESSAGE("dwFileType = %08X", versionInfo.dwFileType);
	_MESSAGE("dwFileSubtype = %08X", versionInfo.dwFileSubtype);
	_MESSAGE("dwFileDateMS = %08X", versionInfo.dwFileDateMS);
	_MESSAGE("dwFileDateLS = %08X", versionInfo.dwFileDateLS);

	UInt64 version = (((UInt64)versionInfo.dwFileVersionMS) << 32) | versionInfo.dwFileVersionLS;

	*out = version;

	return true;
}

const IMAGE_SECTION_HEADER * GetImageSection(const UInt8 * base, const char * name)
{
	const IMAGE_DOS_HEADER		* dosHeader = (IMAGE_DOS_HEADER *)base;
	const IMAGE_NT_HEADERS		* ntHeader = (IMAGE_NT_HEADERS *)(base + dosHeader->e_lfanew);
	const IMAGE_SECTION_HEADER	* sectionHeader = IMAGE_FIRST_SECTION(ntHeader);

	for(UInt32 i = 0; i < ntHeader->FileHeader.NumberOfSections; i++)
	{
		const IMAGE_SECTION_HEADER	* section = &sectionHeader[i];

		if(!strcmp((const char *)section->Name, name))
		{
			return section;
		}
	}

	return NULL;
}

// steam EXE will have the .bind section
bool IsSteamImage(const UInt8 * base)
{
	return GetImageSection(base, ".bind") != NULL;
}

bool IsUPXImage(const UInt8 * base)
{
	return GetImageSection(base, "UPX0") != NULL;
}

struct DebugHeader
{
	char	sig[4];		// "RSDS"
	UInt8	guid[0x10];
	UInt32	age;
	// path follows

	bool SignatureValid(void) const
	{
		return memcmp(sig, "RSDS", 4) == 0;
	}

	bool IsNoGore(bool * probablyValid) const
	{
		*probablyValid = false;

		if(SignatureValid())
		{
			const char	* path = (const char *)(this + 1);

			// path will start with <drive letter> colon backslash
			if(path[1] != ':') return false;
			if(path[2] != '\\') return false;

			// make sure the string isn't stupidly long and only contains printable characters
			for(UInt32 i = 0; i < 0x80; i++)
			{
				char	data = path[i];

				if(!data)
				{
					*probablyValid = true;

					if(strstr(path, "FalloutNVng.pdb"))
					{
						_MESSAGE("pdb path = %s", path);

						return true;
					}

					break;
				}

				if((data < 0) || !isprint(data))
					break;
			}
		}

		return false;
	}
};

const UInt8 * GetVirtualAddress(const UInt8 * base, UInt32 addr)
{
	const IMAGE_DOS_HEADER		* dosHeader = (IMAGE_DOS_HEADER *)base;
	const IMAGE_NT_HEADERS		* ntHeader = (IMAGE_NT_HEADERS *)(base + dosHeader->e_lfanew);
	const IMAGE_SECTION_HEADER	* sectionHeader = IMAGE_FIRST_SECTION(ntHeader);

	for(UInt32 i = 0; i < ntHeader->FileHeader.NumberOfSections; i++)
	{
		const IMAGE_SECTION_HEADER	* section = &sectionHeader[i];

		if((addr >= section->VirtualAddress) && (addr <= (section->VirtualAddress + section->SizeOfRawData)))
		{
			UInt32	offset = addr - section->VirtualAddress;

			return base + section->PointerToRawData + offset;
		}
	}

	return NULL;
}

bool IsNoGore_BasicScan(const UInt8 * base, bool * probable)
{
	*probable = false;

	const IMAGE_DOS_HEADER		* dosHeader = (IMAGE_DOS_HEADER *)base;
	const IMAGE_NT_HEADERS		* ntHeader = (IMAGE_NT_HEADERS *)(base + dosHeader->e_lfanew);
	const IMAGE_DEBUG_DIRECTORY	* debugDir =
		(IMAGE_DEBUG_DIRECTORY *)GetVirtualAddress(base, ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress);

	if(!debugDir) return false;
	if(debugDir->Characteristics) return false;
	if(debugDir->Type != IMAGE_DEBUG_TYPE_CODEVIEW) return false;
	if(debugDir->SizeOfData >= 0x100) return false;
	if(debugDir->AddressOfRawData >= 0x10000000) return false;
	if(debugDir->PointerToRawData >= 0x10000000) return false;

	const DebugHeader	* debugHeader = (DebugHeader *)(base + debugDir->PointerToRawData);

	if(!debugHeader->SignatureValid()) return false;

	*probable = true;

	bool	headerProbable;	// thrown away
	return debugHeader->IsNoGore(&headerProbable);
}

// nogore EXE has debug info pointing to FalloutNVng.pdb
// however sometimes the debug info is pointing to the wrong place?
bool IsNoGore(const UInt8 * base)
{
	bool	result = false;
	bool	probable = false;

	// first check the header where it says it should be
	result = IsNoGore_BasicScan(base, &probable);
	if(!probable)
	{
		_MESSAGE("using slow nogore check");

		// keep scanning, now do the slow and manual way
		// look for RSDS header in .rdata

		const IMAGE_SECTION_HEADER	* rdataSection = GetImageSection(base, ".rdata");
		if(rdataSection)
		{
			const UInt8	* sectionBase = base + rdataSection->PointerToRawData;
			UInt32		sectionLen = rdataSection->SizeOfRawData;

			__try
			{
				for(UInt32 i = 0; (i + sizeof(DebugHeader)) <= sectionLen; i += 4)
				{
					const DebugHeader	* header = (const DebugHeader *)(sectionBase + i);

					if(header->IsNoGore(&probable))
					{
						result = true;
						break;
					}

					if(probable)
						break;
				}
			}
			__except(EXCEPTION_EXECUTE_HANDLER)
			{
				_WARNING("exception while scanning for nogore");
			}
		}
	}

	return result;
}

bool ScanEXE(const char * path, bool * isSteam, bool * isNoGore, bool * isUPX)
{
	// open and map the file in to memory
	HANDLE	file = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(file == INVALID_HANDLE_VALUE)
	{
		_ERROR("ScanEXE: couldn't open file (%d)", GetLastError());
		return false;
	}

	bool result = false;

	HANDLE	mapping = CreateFileMapping(file, NULL, PAGE_READONLY, 0, 0, NULL);
	if(mapping)
	{
		const UInt8	* fileBase = (const UInt8 *)MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
		if(fileBase)
		{
			*isSteam = IsSteamImage(fileBase);
			*isNoGore = IsNoGore(fileBase);
			*isUPX = IsUPXImage(fileBase);

			result = true;

			UnmapViewOfFile(fileBase);
		}
		else
		{
			_ERROR("ScanEXE: couldn't map file (%d)", GetLastError());
		}

		CloseHandle(mapping);
	}
	else
	{
		_ERROR("ScanEXE: couldn't create file mapping (%d)", GetLastError());
	}

	CloseHandle(file);

	return result;
}

bool IdentifyEXE(const char * procName, bool isEditor, std::string * dllSuffix, ProcHookInfo * hookInfo)
{
	UInt64	version;
	bool	isSteam, isNoGore, isUPX;

	// check file version
	if(!GetFileVersionNumber(procName, &version))
	{
		PrintLoaderError("Couldn't retrieve EXE version information.");
		return false;
	}

	_MESSAGE("version = %016I64X", version);

	// check for Steam EXE
	if(!ScanEXE(procName, &isSteam, &isNoGore, &isUPX))
	{
		PrintLoaderError("Failed to identify Steam EXE.");
		return false;
	}

	if (isNoGore)
	{
		PrintLoaderError("You have the german No Gore version of FalloutNV.exe which is not supported by xNVSE. To fix this search online for a \"German Uncut Fallout New Vegas Patch\" or request a different EXE from Steam customer support.");
		return false;
	}

	// ### how to tell the difference between nogore versions and standard without a global checksum?
	// ### since we're mapping the exe to check for steam anyway, checksum the .text segment maybe?

	_MESSAGE(isSteam ? "steam exe" : "normal exe");
	if(isNoGore) _MESSAGE("nogore");
	if(isUPX) _MESSAGE("upx");

	hookInfo->version = version;
	hookInfo->noGore = isNoGore;

	if(isUPX)
	{
		hookInfo->procType = kProcType_Packed;
	}
	else if(isSteam)
	{
		hookInfo->procType = kProcType_Steam;
	}
	else
	{
		hookInfo->procType = kProcType_Normal;
	}

	bool result = false;

	if(isEditor)
	{
		switch(version)
		{
			case 0x0001000100000106:	// 1.1.0.262 (original release)
				hookInfo->hookCallAddr = 0x00C61BA1;
				hookInfo->loadLibAddr = 0x00D2218C;
				*dllSuffix = "1_1";
				PrintLoaderError("Please update to the latest version of the GECK.");
				break;

			case 0x00010003000001C4:	// 1.3.0.452
				hookInfo->hookCallAddr = 0x00C62AF1;
				hookInfo->loadLibAddr = 0x00D2318C;
				*dllSuffix = "1_3";
				PrintLoaderError("Please update to the latest version of the GECK.");
				break;

			case 0x0001000400000206:	// 1.4.0.518
				hookInfo->hookCallAddr = 0x00C62BC1;
				hookInfo->loadLibAddr = 0x00D2318C;
				*dllSuffix = "1_4";
				result = true;
				break;

			default:
				PrintLoaderError("You have an unknown version of the GECK. Please check https://github.com/xNVSE/NVSE/releases to make sure you're using the latest version of xNVSE. (version = %016I64X)", version);
				break;
		}
	}
	else
	{
		if(isUPX)
		{
			PrintLoaderError("Packed versions of the game are not supported.");
		}
		else
		{
			const UInt64 kCurVersion = 0x000100040000020D;	// 1.4.0.525
			const UInt64 kEpicVersion = 0x000100040000053A;	// 1.4.0.1338

			if(version < kCurVersion)
			{
				PrintLoaderError("You are most likely using a pirated copy of the game which is unsupported by xNVSE. Please use a Steam or GOG copy.");
			}
			else if(version == kCurVersion)
			{
				if(isNoGore)
				{
					hookInfo->hookCallAddr = 0x00ECC3FB;
					hookInfo->loadLibAddr = 0x00FDF0B0;
					*dllSuffix = "1_4ng";
					result = true;
				}
				else
				{
					hookInfo->hookCallAddr = 0x00ECC46B;
					hookInfo->loadLibAddr = 0x00FDF0B0;
					*dllSuffix = "1_4";
					result = true;
				}
			}
			else if (version == kEpicVersion)
			{
				// Could use other adresses, just picked this one out of many.
				UInt32 constexpr addrToCheckForPatch = 0x00000290;
				if (*reinterpret_cast<UInt8*>(addrToCheckForPatch) == 0x4C)
				{
					result = true;
				}
				else if (*reinterpret_cast<UInt8*>(addrToCheckForPatch) == 0xA0)
				{
					PrintLoaderError("You are using the Epic Games Store version of the game, which is unsupported by xNVSE. You may use this patcher to make your exe compatible: https://www.nexusmods.com/newvegas/mods/81281");
				}
				else
				{
					PrintLoaderError("You are likely using an unknown Epic Games Store version. Let us know about it by writing a report on any xNVSE report channel.");
				}
			}
			else
			{
				PrintLoaderError("You are using a newer version of Fallout than this version of xNVSE supports. If the patch to this version just came out, please be patient while we update our code. In the meantime, please check https://github.com/xNVSE/NVSE to make sure you're using the latest version of xNVSE. (version = %016I64X %08X)", version, PACKED_NVSE_VERSION);
			}
		}
	}

	return result;
}
