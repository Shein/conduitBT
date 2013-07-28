#pragma once

#include "def.h"


/*
 * CallInfo template: supported T type = cchar & wchar
 */
template<class T> class CallInfo
{
  public:
	CallInfo (T * number) throw();

  public:
	T * Number;

  public:
    void* operator new (size_t nSize, T * number);
    void  operator delete (void* p);

  protected:
	// disable the standard new
    void* operator new (size_t nSize);
};


template<>
inline CallInfo<cchar>::CallInfo (cchar * number) : Number((char*)this + sizeof(CallInfo<cchar>))
{
	strcpy ((char*)Number, number);
}

template<>
inline CallInfo<wchar>::CallInfo (wchar * number) : Number((wchar*)((char*)this + sizeof(CallInfo<wchar>)))
{
	wcscpy ((wchar_t*)Number, number);
}

template<> 
inline void * CallInfo<cchar>::operator new (size_t size, cchar * number)
{
	return malloc (sizeof(CallInfo<cchar>) + strlen(number) + 1);
}

template<> 
inline void * CallInfo<wchar>::operator new (size_t size, wchar * number)
{
	return malloc (sizeof(CallInfo<wchar>) + (wcslen(number)+2) * sizeof(wchar));
}


template<class T> 
inline void CallInfo<T>::operator delete (void* p)
{
	free(p);
}

