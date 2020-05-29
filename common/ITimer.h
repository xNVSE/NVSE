#pragma once

#include "common/ITypes.h"

/**
 *	A high-resolution timer.
 */
class ITimer
{
	public:
		ITimer();
		~ITimer();

		static void	Init(void);
		static void DeInit(void);

		void	Start(void);

		double	GetElapsedTime(void);	// seconds

	private:
		UInt64	m_qpcBase;	// QPC
		UInt32	m_tickBase;	// timeGetTime

		static double	s_secondsPerCount;
		static TIMECAPS	s_timecaps;
		static bool		s_setTime;

		// safe QPC stuff
		static UInt64	GetQPC(void);

		static UInt64	s_lastQPC;
		static UInt64	s_qpcWrapMargin;
		static bool		s_hasLastQPC;

		static UInt32	s_qpcWrapCount;
		static UInt32	s_qpcInaccurateCount;
};
