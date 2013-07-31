/**********************************************************************\
 Filename    :  smTimer.h
 Purpose     :  Class TIMER
 Platform    :  Windows.
\**********************************************************************/

#pragma once

#include "timer.h"
#include "smBase.h"


class SmTimer : public Timer
{
  public:
	SmTimer (SMID smid, SMEV ev)
	{
		Event.SmId = smid;
		Event.Ev   = ev;
	}

    bool Construct ()
	{
		return Timer::Construct(TIMERCB(TimerCb), this);
	}

  public:
	SMEVENT Event;

  protected:
	static void TimerCb (SmTimer* context)
	{
		SmBase::PutEvent (&context->Event, SMQ_LOW);
	}
};

