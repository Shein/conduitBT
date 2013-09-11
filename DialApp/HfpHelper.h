/*******************************************************************\
 Filename    :  HfpHelper.h
 Purpose     :  HFP SM help classes
\*******************************************************************/

#ifndef _HFPHELPER_H_
#define _HFPHELPER_H_

#include "str.h"
#include "HfpSm.h"


/*
 ****************************************************************************************
 Help class to represent HFP indicators (AT+CIND commands).
 ****************************************************************************************
 */
class HfpIndicators
{
  public:
	HfpIndicators() : Constructed(false)
	{
		NumServicesPresent = NumStatusesPresent = 0;
		memset (ServIdxes, 0, sizeof(ServIdxes));
		memset (ServStatuses, 0, sizeof(ServStatuses));
	}

	bool  Construct	(char* mapping);
	void  SetStatuses (char* statuses);
	int   GetCurrentState ();	// return HfpSm::STATE or -1 when not detected

  public:
	bool Constructed;

  protected:
	enum {
		CALL,
		CALLSETUP,
		CALLHELD,
		NumServices,
		MaxServices = 10
	};

  protected:
	int  ServIdxes  [NumServices];
	int  ServStatuses [NumServices];
	int  NumServicesPresent;
	int  NumStatusesPresent;

  protected:
	static STRB ServNames[NumServices];
};


#endif  // _HFPHELPER_H_
