#pragma once

#include "def.h"


/*
 * CallInfo template: supported T type = char & wchar
 * Class object and its dynamic info string are allocated from one memory block.
 */
template<class T> class CallInfo
{
  public:
	CallInfo (T * info) throw();

  public:
	T * Info;

  public:
    void* operator new (size_t nSize, T * info);
    void  operator delete (void* p);

  protected:
	// disable the standard new
    void* operator new (size_t nSize);
};


template<>
inline CallInfo<char>::CallInfo (char * info) : Info((char*)this + sizeof(CallInfo<char>))
{
	strcpy ((char*)Info, info);
}

template<>
inline CallInfo<wchar>::CallInfo (wchar * info) : Info((wchar*)((char*)this + sizeof(CallInfo<wchar>)))
{
	wcscpy ((wchar_t*)Info, info);
}

template<> 
inline void * CallInfo<char>::operator new (size_t size, char * info)
{
	return malloc (sizeof(CallInfo<char>) + strlen(info) + 1);
}

template<> 
inline void * CallInfo<wchar>::operator new (size_t size, wchar * info)
{
	return malloc (sizeof(CallInfo<wchar>) + (wcslen(info)+2) * sizeof(wchar));
}


template<class T> 
inline void CallInfo<T>::operator delete (void* p)
{
	free(p);
}

