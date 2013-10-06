/*******************************************************************\
 Filename    :  smId.h
 Purpose     :  Global SMs enumerators and types
\*******************************************************************/

#ifndef _SMID_H
#define _SMID_H

#include "def.h"
#include "enums.h"


/*
   List of all State Machines in the system
 */
#define SM_LIST           \
    ENUM_ENTRY (SM, HFP)


/*
   List of all State Machines Events in the system
 */
#define SMEV_LIST	\
    ENUM_ENTRY (SMEV, Error					),	\
    ENUM_ENTRY (SMEV, Timeout				),	\
    ENUM_ENTRY (SMEV, SelectDevice			),	\
    ENUM_ENTRY (SMEV, ForgetDevice			),	\
    ENUM_ENTRY (SMEV, Disconnect			),	\
    ENUM_ENTRY (SMEV, ConnectStart			),	\
    ENUM_ENTRY (SMEV, Connected				),	\
    ENUM_ENTRY (SMEV, HfpConnectStart		),	\
    ENUM_ENTRY (SMEV, ServiceConnectStart	),	\
    ENUM_ENTRY (SMEV, ServiceConnected		),	\
    ENUM_ENTRY (SMEV, AtResponse			),	\
    ENUM_ENTRY (SMEV, StartOutgoingCall		),	\
    ENUM_ENTRY (SMEV, Answer				),	\
    ENUM_ENTRY (SMEV, CallEnd				),	\
    ENUM_ENTRY (SMEV, CallStart				),	\
    ENUM_ENTRY (SMEV, CallEnded				),	\
    ENUM_ENTRY (SMEV, SwitchHeadset			),	\
    ENUM_ENTRY (SMEV, SwitchVoice			),	\
    ENUM_ENTRY (SMEV, SendDtmf				),	\
	ENUM_ENTRY (SMEV, PutOnHold				),	\
	ENUM_ENTRY (SMEV, CallWaiting			),	\
	ENUM_ENTRY (SMEV, CallHeld				)


/*
   SMEV_AtResponse event extension (SMEV_PAR, AtResponse field)
 */
#define SMEV_ATRESPONSE_LIST	\
	ENUM_ENTRY (SMEV_AtResponse,  Ok						),	\
	ENUM_ENTRY (SMEV_AtResponse,  Error						),	\
	ENUM_ENTRY (SMEV_AtResponse,  CurrentPhoneIndicators	),	\
	ENUM_ENTRY (SMEV_AtResponse,  CallSetup_None			),	\
	ENUM_ENTRY (SMEV_AtResponse,  CallSetup_Incoming		),	\
	ENUM_ENTRY (SMEV_AtResponse,  CallSetup_Outgoing		),	\
	ENUM_ENTRY (SMEV_AtResponse,  CallSetup_RemoteAlert		),	\
	ENUM_ENTRY (SMEV_AtResponse,  CallHeld_None				),	\
	ENUM_ENTRY (SMEV_AtResponse,  CallHeld_HeldAndActive	),	\
	ENUM_ENTRY (SMEV_AtResponse,  CallHeld_HeldOnly			),	\
	ENUM_ENTRY (SMEV_AtResponse,  CallWaiting_Ringing		),	\
	ENUM_ENTRY (SMEV_AtResponse,  CallWaiting_Stopped		),	\
	ENUM_ENTRY (SMEV_AtResponse,  ListCurrentCalls			),	\
	ENUM_ENTRY (SMEV_AtResponse,  CallingLineId				)



DECL_ENUM (SMID, SM_LIST)
DECL_ENUM (SMEV, SMEV_LIST)

DECL_ENUM (SMEV_ATRESPONSE, SMEV_ATRESPONSE_LIST)


/* 
   Events parameters union.
*/

template<class T> class CallInfo;

struct SMEV_PAR
{
	union {
		uint64					BthAddr;
		bool					PcSound;
		char					Dtmf;
		CallInfo<char>*			CallNumber;
	};

	struct {
		SMEV_ATRESPONSE			AtResponse;
		union {
		  CallInfo<wchar>*		InfoWch;
		  CallInfo<char>*		InfoCh;
		  int					IndicatorsState;	// actual when AtResponse = SMEV_AtResponse_CurrentPhoneIndicators
		};
	};

    int							ReportError;
};


struct SMEVENT;


cchar * smidFormatEventName (SMEVENT *pEv);


#endif // _SMID_H
