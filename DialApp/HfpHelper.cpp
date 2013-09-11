/*******************************************************************\
 Filename    :  HfpHelper.cpp
 Purpose     :  HFP SM help classes
\*******************************************************************/

#include "def.h"
#include "deblog.h"
#include "HfpHelper.h"
#include "InHand.h"


STRB HfpIndicators::ServNames[NumServices] = { "call", "callsetup", "callheld" };


bool HfpIndicators::Construct (char* mapping)
{
	char* s1, *s2;
	int   i, n = 0;

	STRB str(mapping);

	// 1st ScanCharNext instead ScanChar is ok because the first mapping char should be '('
	for (i = 0;  i < MaxServices;  i++)
	{
		if ( !(s1 = str.ScanCharNext('\"')) )
			break;
		if ( !(s2 = str.ScanCharNext('\"')) )
			break;
		*s2 = '\0';
		for (int j = 0; j < NumServices; j++)
			if (ServNames[j] == (cchar*)(s1+1)) {
				ServIdxes[j] = i + 1;  // HFP indicator index starts from 1
				n++;
				break;
			}
	}

	NumServicesPresent = i;

	if (n != NumServices) {
		LogMsg ("ParseAndSetAtIndicators failed");
		return (Constructed=false);
	}

	InHand::SetIndicatorsNumbers (ServIdxes[CALL], ServIdxes[CALLSETUP], ServIdxes[CALLHELD]);
	return (Constructed=true);
}


void  HfpIndicators::SetStatuses (char* statuses)
{
	STRB str(statuses);
	int	 idx, i, x;

	for (i = 0;  i < NumServicesPresent;  i++)
	{
		// detect idx for i (CALL, CALLSETUP, ...)
		idx = -1;
		for (int j = 0; j < NumServices; j++)
			if (i == ServIdxes[j]-1) // HFP indicator index starts from 1
				{ idx = j;  break; }

		char* s = str.SeekInt(&x);
		if (!s)
			break;
		if (idx >= 0)
			ServStatuses[idx] = x;
		if (str.SeekChar() == '\0')
			{ i++; break; }
	}
	NumStatusesPresent = i;
}


int HfpIndicators::GetCurrentState ()
{
	// Here let's analyze the current state.
	// For now we detect only STATE_HfpConnected (no calls) and  STATE_Calling, STATE_Ringing, STATE_InCall.
	// Complicated cases (3-way calls, etc) meanwhile are out of the scope...

	if (!Constructed || (NumServicesPresent != NumStatusesPresent))
		return -1;

	// See remarks at InHandMng::SetIndicatorsNumbers
	if (ServStatuses[CALL] == 1)
		return HfpSm::STATE_InCall;
	if (ServStatuses[CALLSETUP] == 1)
		return HfpSm::STATE_Ringing;
	if (ServStatuses[CALLSETUP] == 2 || ServStatuses[CALLSETUP] == 3)
		return HfpSm::STATE_Calling;

	return HfpSm::STATE_HfpConnected;
}
