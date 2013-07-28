/********************************************************************************************\
 Library     :  HFP DialApp (Bluetooth HFP library)
 Filename    :  DialApp.h
 Purpose     :  Public h-file for DialApp.dll (together with DialAppType.h)
 Note		 :  All functions may throw C++ exceptions of integer type (DialAppError)
\********************************************************************************************/

#ifndef _DIALAPP_H
#define _DIALAPP_H


#include "DialAppType.h"


/********************************************************************************************\
			Public Functions (throws exceptions of integer type = DialAppError enum)
\********************************************************************************************/
// TODO many user requests error & FIFO 
void   dialappInit				(DialAppCb cb);
void   dialappEnd				();
void   dialappUiSelectDevice	();
void   dialappForgetDevice		();
wchar* dialappGetSelectedDevice ();
int	   dialappGetCurrentState	();
void   dialappCall				(cchar* dialnumber, bool headset = true);
void   dialappAnswer			(bool headset = true);
void   dialappSendDtmf			(cchar dialchar);
void   dialappEndCall			();
void   dialappHeadsetSound		(bool headset);
void   dialappSetDebugMode		(DialAppDebug debugtype, int mode);


#endif // _DIALAPP_H
