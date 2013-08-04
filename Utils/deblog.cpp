/**********************************************************************\
 Library     :  Utils
 Filename    :  deblog.cpp
 Purpose     :  Simple Debug logger
 Author      :  Krasnitsky Sergey
\**********************************************************************/

#pragma managed(push, off)

#include "def.h"
#include "deblog.h"
#include "fifo_cse.h"


/***********************************************************************************************\
										Static data
\***********************************************************************************************/
DebLog  DebLog::GlobalObj("-------");

struct STRBUF
{
    char str[DebLog::MsgMaxSize+1];
};

FIFO_SIMPLE_ALLOC<STRBUF,DebLog::MsgMaxNum>  strallocCyclicBuffer;




/***********************************************************************************************\
									Protected Static functions
\***********************************************************************************************/

void DebLog::LogMsgHelper (cchar * msg, va_list args)
{
	char * newstr = strallocCyclicBuffer.FetchNext()->str;
	memcpy (newstr + Msg1stPrefixSize, Module, Msg2ndPrefixSize - 2);
	vsnprintf (newstr+MsgPrefixSize, MsgMaxSize-MsgPrefixSize, msg, args);
	OutputDebugString (newstr);
}


/***********************************************************************************************\
									Public Static functions
\***********************************************************************************************/

void DebLog::Init (cchar * applname)
{
	char *s;
	for (int i=0; i<strallocCyclicBuffer.Size; i++) {
		s = strallocCyclicBuffer.Addr[i].str;
		memcpy (s, applname, Msg1stPrefixSize - 2);
		s[Msg1stPrefixSize - 2] = ':';
		s[Msg1stPrefixSize - 1] = ' ';
		s[MsgPrefixSize - 2]    = ':';
		s[MsgPrefixSize - 1]    = ' ';
	}
}


#pragma managed(pop)
