#include "Options.h"

Options g_options;

Options::Options()
:m_launchCS(false)
,m_setPriority(false)
,m_priority(0)
,m_crcOnly(false)
,m_optionsOnly(false)
,m_waitForClose(false)
,m_verbose(false)
,m_moduleInfo(false)
,m_skipLauncher(true)
,m_fpsLimit(0)
,m_appID(0)
{
	//
}

Options::~Options()
{
	//
}

// disable "switch statement contains 'default' but no 'case' labels"
#pragma warning (push)
#pragma warning (disable : 4065)

bool Options::Read(int argc, char ** argv)
{
	if(argc >= 1)
	{
		// remove app name
		argc--;
		argv++;

		int	freeArgCount = 0;

		while(argc > 0)
		{
			char	* arg = *argv++;
			argc--;

			if(arg[0] == '-')
			{
				// switch
				arg++;

				if(!_stricmp(arg, "editor"))
				{
					m_launchCS = true;
				}
				else if(!_stricmp(arg, "priority"))
				{
					if(argc >= 1)
					{
						arg = *argv++;
						argc--;

						m_setPriority = true;

						if(!_stricmp(arg, "above_normal"))
						{
							m_priority = ABOVE_NORMAL_PRIORITY_CLASS;
						}
						else if(!_stricmp(arg, "below_normal"))
						{
							m_priority = BELOW_NORMAL_PRIORITY_CLASS;
						}
						else if(!_stricmp(arg, "high"))
						{
							m_priority = HIGH_PRIORITY_CLASS;
						}
						else if(!_stricmp(arg, "idle"))
						{
							m_priority = IDLE_PRIORITY_CLASS;
						}
						else if(!_stricmp(arg, "normal"))
						{
							m_priority = NORMAL_PRIORITY_CLASS;
						}
						else if(!_stricmp(arg, "realtime"))
						{
							m_priority = REALTIME_PRIORITY_CLASS;
						}
						else
						{
							m_setPriority = false;

							_ERROR("couldn't read priority argument (%s)", arg);
							return false;
						}
					}
					else
					{
						_ERROR("priority not specified");
						return false;
					}
				}
				else if(!_stricmp(arg, "altexe"))
				{
					if(argc >= 1)
					{
						m_altEXE = *argv++;
						argc--;
					}
					else
					{
						_ERROR("exe path not specified");
						return false;
					}
				}
				else if(!_stricmp(arg, "altdll"))
				{
					if(argc >= 1)
					{
						m_altDLL = *argv++;
						argc--;
					}
					else
					{
						_ERROR("dll path not specified");
						return false;
					}
				}
				else if(!_stricmp(arg, "crconly"))
				{
					m_crcOnly = true;
				}
				else if(!_stricmp(arg, "h") || !_stricmp(arg, "help"))
				{
					m_optionsOnly = true;
				}
				else if(!_stricmp(arg, "waitforclose"))
				{
					m_waitForClose = true;
				}
				else if(!_stricmp(arg, "fpslimit"))
				{
					if(argc >= 1)
					{
						const char	* fpsLimitStr = *argv++;
						argc--;

						if(sscanf_s(fpsLimitStr, "%d", &m_fpsLimit) != 1)
						{
							_ERROR("couldn't read fps limit as an integer (%s)", fpsLimitStr);
							return false;
						}
					}
					else
					{
						_ERROR("fps limit not specified");
						return false;
					}
				}
				else if(!_stricmp(arg, "v"))
				{
					m_verbose = true;
				}
				else if(!_stricmp(arg, "minfo"))
				{
					m_moduleInfo = true;
				}
				else if(!_stricmp(arg, "noskiplauncher"))
				{
					m_skipLauncher = false;
				}
				else if(!_stricmp(arg, "appid"))
				{
					if(argc >= 1)
					{
						const char	* appIDStr = *argv++;
						argc--;

						if(sscanf_s(appIDStr, "%d", &m_appID) != 1)
						{
							_ERROR("couldn't read appID as an integer (%s)", appIDStr);
							return false;
						}
					}
					else
					{
						_ERROR("appID not specified");
						return false;
					}
				}
				else
				{
					_ERROR("unknown switch (%s)", arg);
					return false;
				}
			}
			else
			{
				// free arg

				switch(freeArgCount)
				{
					default:
						_ERROR("too many free args (%s)", arg);
						return false;
				}
			}
		}
	}

	return Verify();
}

#pragma warning (pop)

void Options::PrintUsage(void)
{
	gLog.SetPrintLevel(IDebugLog::kLevel_VerboseMessage);

	_MESSAGE("usage: nvse_loader [options]");
	_MESSAGE("");
	_MESSAGE("options:");
	_MESSAGE("  -h, -help - print this options list");
	_MESSAGE("  -editor - launch the construction set");
	_MESSAGE("  -priority <level> - set the launched program\'s priority");
	_MESSAGE("    above_normal");
	_MESSAGE("    below_normal");
	_MESSAGE("    high");
	_MESSAGE("    idle");
	_MESSAGE("    normal");
	_MESSAGE("    realtime");
	_MESSAGE("  -altexe <path> - set alternate exe path");
	_MESSAGE("  -altdll <path> - set alternate dll path");
	_MESSAGE("  -crconly - just identify the EXE, don't launch anything");
	_MESSAGE("  -waitforclose - wait for the launched program to close");
	_MESSAGE("                  designed for use with AlacrityPC and similar");
	_MESSAGE("  -fpslimit <max fps> - use FPSLimiter to limit framerate");
	_MESSAGE("                        Limiter_D3D9.dll and HookHelper.dll must be copied to");
	_MESSAGE("                        the fallout directory for this to work");
	_MESSAGE("  -v - print verbose messages to the console");
	_MESSAGE("  -minfo - log information about the DLLs loaded in to the target process");
	_MESSAGE("  -noskiplauncher - does not skip the default Bethesda launcher window");
	_MESSAGE("                    note: specifying this option may cause compatibility problems");
	_MESSAGE("  -appid <id> - choose a different steam appid (use 22490 for enplczru)");
}

bool Options::Verify(void)
{
	// nothing to verify currently

	return true;
}
