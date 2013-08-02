/*******************************************************************\
 Filename    :  smID.h
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
    ENUM_ENTRY (SMEV, Failure				),	\
    ENUM_ENTRY (SMEV, Timeout				),	\
    ENUM_ENTRY (SMEV, SelectDevice			),	\
    ENUM_ENTRY (SMEV, ForgetDevice			),	\
    ENUM_ENTRY (SMEV, Disconnected			),	\
    ENUM_ENTRY (SMEV, ConnectStart			),	\
    ENUM_ENTRY (SMEV, Connected				),	\
    ENUM_ENTRY (SMEV, HfpConnectStart		),	\
    ENUM_ENTRY (SMEV, HfpConnected			),	\
    ENUM_ENTRY (SMEV, ServiceConnectStart	),	\
    ENUM_ENTRY (SMEV, ServiceConnected		),	\
    ENUM_ENTRY (SMEV, IncomingCall			),	\
    ENUM_ENTRY (SMEV, StartOutgoingCall		),	\
    ENUM_ENTRY (SMEV, Answer				),	\
    ENUM_ENTRY (SMEV, SendDtmf				),	\
    ENUM_ENTRY (SMEV, EndCall				),	\
    ENUM_ENTRY (SMEV, HeadsetOn				),	\
    ENUM_ENTRY (SMEV, HeadsetOff			),	\
    ENUM_ENTRY (SMEV, AtResponse			),	\
	ENUM_ENTRY (SMEV, PutOnHold				)




/*
   SMEV_AtResponse event extension (SMEV_PAR, AtResponse field)
 */
enum SMEV_ATRESPONSE
{
	SMEV_AtResponse_Ok						,
	SMEV_AtResponse_Error					,
	SMEV_AtResponse_CallSetup_None			,
	SMEV_AtResponse_CallSetup_Incoming		,
	SMEV_AtResponse_CallSetup_Outgoing		,
	SMEV_AtResponse_CallSetup_RemoteAlert	,
	SMEV_AtResponse_CallIdentity			,
};



DECL_ENUM (SMID, SM_LIST)
DECL_ENUM (SMEV, SMEV_LIST)



/* 
   Events parameters union.
*/

template<class T> class CallInfo;

union SMEV_PAR
{
    uint64					BthAddr;
    bool					ReportFailure;
	char					dtmf;
	struct {
		SMEV_ATRESPONSE		AtResponse;
		CallInfo<wchar>	   *Abonent;
	};
	struct {
		CallInfo<cchar>	   *CallNumber;
		bool				HeadsetOn;
	};
};



#endif // _SMID_H
