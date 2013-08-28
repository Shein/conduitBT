/**********************************************************************\
 Library     :  Utils
 Filename    :  deblog.h
 Purpose     :  Simple Debug logger
 Author      :  Krasnitsky Sergey
\**********************************************************************/

#pragma once
#pragma managed(push, off)


#include "def.h"
#include "fifo_cse.h"


/*
 *********************************************************************************************
 Debug logger and Exception generating class. 
 *********************************************************************************************
 */
class DebLog
{
  public:
	enum { 
		MsgMaxNum		 =	8,	// Number of strings for simultaneous messages
		MsgMaxSize		 = 250,	// Max size of string buffer for trace/exception messages
		Msg1stPrefixSize =  9, 	// Size of messages 1st prefix, that is Appl name (e.g. "DialApp: ")
		Msg2ndPrefixSize =  9, 	// Size of messages 2nd prefix, that is module name
		MsgPrefixSize = Msg1stPrefixSize + Msg2ndPrefixSize
	};

  public:
	cchar * Module;

  public:
	DebLog(cchar * module) : Module(module) {};

  public:
	static DebLog  GlobalObj;	// Object for printing from the global context (C-style and static code)
	static cchar * ApplName;

  public:
	static void Init (cchar * applname);
	static void End  () {};

  public:
	void LogMsg (cchar * msg, ...)
	{
		va_list  argptr;
		va_start (argptr, msg);
		LogMsgHelper (msg, argptr);
	}

	int IntException (int error, char * msg, ...)
	{
		va_list  argptr;
		va_start (argptr, msg);
		LogMsgHelper(msg, argptr);
		return error;
	}

  public:
	void LogMsgHelper (cchar * msg, va_list args);
};


/*
 *********************************************************************************************
 LogMsg & IntException for the global context (C-style and static code). 
 *********************************************************************************************
 */

inline void LogMsg (cchar * msg, ...)
{
	va_list  argptr;
	va_start (argptr, msg);
	DebLog::GlobalObj.LogMsgHelper(msg, argptr);
}

inline int IntException (int error, char * msg, ...)
{
	va_list  argptr;
	va_start (argptr, msg);
	DebLog::GlobalObj.LogMsgHelper(msg, argptr);
	return error;
}


struct STRBUF
{
    char str[DebLog::MsgMaxSize+1];
};

extern FIFO_SIMPLE_ALLOC<STRBUF,DebLog::MsgMaxNum>  strallocCyclicBuffer;


#pragma managed(pop)
