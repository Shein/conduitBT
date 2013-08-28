/*******************************************************************\
 Filename    :  smId.cpp
 Purpose     :  Global SMs enumerators and types
\*******************************************************************/

#include "def.h"
#include "str.h"
#include "stralloc.h"
#include "smId.h"
#include "smBase.h"
#include "enums_impl.h"


IMPL_ENUM (SMID, SM_LIST)
IMPL_ENUM (SMEV, SMEV_LIST)
IMPL_ENUM (SMEV_ATRESPONSE, SMEV_ATRESPONSE_LIST)


cchar* smidFormatEventName (SMEVENT *pEv)
{
	switch (pEv->Ev)
	{
		case SMEV_Failure:
			{
				// Detail the failure event (it's common for all SMs)
				STRB str (strallocGet());
				str.Sprintf ("%s (Error #%d)", enumTable_SMEV[pEv->Ev], pEv->Param.ReportError);
				return (char*) str;
			}

		case SMEV_AtResponse:
			{
				STRB str (strallocGet());
				str.Sprintf ("%s (%s)", enumTable_SMEV[pEv->Ev], enumTable_SMEV_ATRESPONSE[pEv->Param.AtResponse]);
				return (char*) str;
			}

		case SMEV_SelectDevice:
			{
				STRB str (strallocGet());
				str.Sprintf ("%s (%llX)", enumTable_SMEV[pEv->Ev], pEv->Param.BthAddr);
				return (char*) str;
			}

		case SMEV_SendDtmf:
			{
				STRB str (strallocGet());
				str.Sprintf ("%s (%c)", enumTable_SMEV[pEv->Ev], pEv->Param.Dtmf);
				return (char*) str;
			}

		case SMEV_SwitchHeadset:
		case SMEV_SwitchVoice:
			{
				STRB str (strallocGet());
				str.Sprintf ("%s (%d)", enumTable_SMEV[pEv->Ev], pEv->Param.PcSound);
				return (char*) str;
			}
	}

	return enumTable_SMEV[pEv->Ev];
}
