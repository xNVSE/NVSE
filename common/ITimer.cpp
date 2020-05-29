#include "ITimer.h"

// QueryPerformanceCounter is very accurate, but hardware bugs can cause it to return inaccurate results
// this code uses multimedia timers to check for glitches in QPC

double		ITimer::s_secondsPerCount = 0;
TIMECAPS	ITimer::s_timecaps = { 0 };
bool		ITimer::s_setTime = false;
UInt64		ITimer::s_lastQPC = 0;
UInt64		ITimer::s_qpcWrapMargin = 0;
bool		ITimer::s_hasLastQPC = false;
UInt32		ITimer::s_qpcWrapCount = 0;
UInt32		ITimer::s_qpcInaccurateCount = 0;

ITimer::ITimer()
:m_qpcBase(0), m_tickBase(0)
{
	Init();
}

ITimer::~ITimer()
{

}

void ITimer::Init(void)
{
	if(!s_secondsPerCount)
	{
		// init qpc
		UInt64	countsPerSecond;
		BOOL res = QueryPerformanceFrequency((LARGE_INTEGER *)&countsPerSecond);

		ASSERT_STR(res, "ITimer: no high-resolution timer support");

		s_secondsPerCount = 1.0 / countsPerSecond;

		s_qpcWrapMargin = (UInt64)(-((SInt64)(countsPerSecond * 60)));	// detect if we've wrapped around by a delta greater than this - also limits max time
		_MESSAGE("s_qpcWrapMargin: %016I64X", s_qpcWrapMargin);
		_MESSAGE("wrap time: %fs", ((double)0xFFFFFFFFFFFFFFFF) * s_secondsPerCount);

		// init multimedia timer
		timeGetDevCaps(&s_timecaps, sizeof(s_timecaps));

		_MESSAGE("min timer period = %d", s_timecaps.wPeriodMin);

		s_setTime = (timeBeginPeriod(s_timecaps.wPeriodMin) == TIMERR_NOERROR);
		if(!s_setTime)
			_WARNING("couldn't change timer period");
	}
}

void ITimer::DeInit(void)
{
	if(s_secondsPerCount)
	{
		if(s_setTime)
		{
			timeEndPeriod(s_timecaps.wPeriodMin);
			s_setTime = false;
		}

		if(s_qpcWrapCount)
			_MESSAGE("s_qpcWrapCount: %d", s_qpcWrapCount);

		s_secondsPerCount = 0;
	}
}

void ITimer::Start(void)
{
	m_qpcBase = GetQPC();
	m_tickBase = timeGetTime();
}

double ITimer::GetElapsedTime(void)
{
	UInt64	qpcNow = GetQPC();
	UInt32	tickNow = timeGetTime();

	UInt64	qpcDelta = qpcNow - m_qpcBase;
	UInt64	tickDelta = tickNow - m_tickBase;

	double	qpcSeconds = ((double)qpcDelta) * s_secondsPerCount;
	double	tickSeconds = ((double)tickDelta) * 0.001;	// ticks are in milliseconds
	double	qpcTickDelta = qpcSeconds - tickSeconds;

	if(qpcTickDelta < 0) qpcTickDelta = -qpcTickDelta;

	// if they differ by more than one second, something's wrong, return
	if(qpcTickDelta > 1)
	{
		s_qpcInaccurateCount++;
		return tickSeconds;
	}
	else
	{
		return qpcSeconds;
	}
}

UInt64 ITimer::GetQPC(void)
{
	UInt64	now;

	QueryPerformanceCounter((LARGE_INTEGER *)&now);

	if(s_hasLastQPC)
	{
		UInt64	delta = now - s_lastQPC;

		if(delta > s_qpcWrapMargin)
		{
			// we've gone back in time, return a kludged value

			s_lastQPC = now;
			now = s_lastQPC + 1;

			s_qpcWrapCount++;
		}
		else
		{
			s_lastQPC = now;
		}
	}
	else
	{
		s_hasLastQPC = true;
		s_lastQPC = now;
	}

	return now;
}
