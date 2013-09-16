/*************************************************************************\
 Filename    :  CallInfo.h
 Purpose     :  CallInfo class for represent caller Number & Name strings
\*************************************************************************/

#pragma once

#include "def.h"
#include "str.h"
#include "DialAppType.h"


/*
 * CallInfo template: supported T type = char & wchar
 * Class object and its dynamic info string are allocated from one memory block.
 */
template<class T> class CallInfo
{
  public:
	CallInfo (T * info) throw();

  public:
	T *				Info;
	DialAppAbonent	InfoParsed;

	DialAppAbonent* GetAbonent()  { return &InfoParsed; }

	bool Compare (CallInfo * x);

  public:
    void* operator new (size_t nSize, T * info);
    void  operator delete (void* p);

  public:
	bool Parse2NumberName ();

  protected:
	// disable the standard new
    void* operator new (size_t nSize);
};


#if 0
// CallInfo<wchar> is not used for now
template<>
inline CallInfo<wchar>::CallInfo (wchar * info) : Info((wchar*)((char*)this + sizeof(CallInfo<wchar>)))
{
	wcscpy ((wchar_t*)Info, info);
}

template<> 
inline void * CallInfo<wchar>::operator new (size_t size, wchar * info)
{
	return malloc (sizeof(CallInfo<wchar>) + (wcslen(info)+2) * sizeof(wchar));
}
#endif


template<>
inline CallInfo<char>::CallInfo (char * info) : Info((char*)this + sizeof(CallInfo<char>))
{
	// this object is already initialized to 0 by calloc
	strcpy ((char*)Info, info);
}

template<> 
inline void * CallInfo<char>::operator new (size_t size, char * info)
{
	return calloc (1, sizeof(CallInfo<char>) + strlen(info) + 1);
}

template<class T> 
inline void CallInfo<T>::operator delete (void* p)
{
	free(p);
}

// KS TODO - move it from inline func
template<>
inline bool CallInfo<char>::Parse2NumberName ()
{
	InfoParsed.Name   = 0;
	InfoParsed.Number = 0;

	STRB str(Info);

	// Search for "Number"
	char* s1 = str.ScanChar('\"');
	if (!s1)
		return false;
	char* s2 = str.ScanCharNext('\"');
	if (!s2)
		return false;
	*s2 = '\0';	
	InfoParsed.Number = s1 + 1;

	// Search for "Name"
	if (s1 = str.ScanCharNext('\"'))
	{
		s2 = str.ScanCharNext('\"');
		if (!s2)
			return true;
		*s2 = '\0';	
		InfoParsed.Name = s1 + 1;
	}
	return true;
}


// Help func for CallInfo<char>::Compare
inline bool CompareStrings (char*s1, char*s2)
{
	if (s1 == s2)
		return true;
	if (!s1 || !s2)
		return false;
	return (strcmp(s1,s2) == 0);
}


template<>
inline bool CallInfo<char>::Compare (CallInfo * x)
{
	if (!x)
		return false;
	if (!CompareStrings(InfoParsed.Number, x->InfoParsed.Number))
		return false;
	//KS: Don't take Name into account for now: the negative effect occurs when name is present and then absent 
	//if (!CompareStrings(InfoParsed.Name, x->InfoParsed.Name))
	//	return false;
	return true;
}
