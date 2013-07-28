/**********************************************************************\
 Filename    :  def.h
 Purpose     :  Common definitions header.
 Platform    :  Windows.
\**********************************************************************/

#ifndef _DEF_H
#define _DEF_H


#include <stdarg.h>
#include <stdio.h>

#include <windows.h>
#include <setupapi.h>
#include <mmsystem.h>
#include <mmreg.h>


/***********************************************************************\
							Common types
\***********************************************************************/
typedef const char			cchar;
typedef const wchar_t		wchar;
typedef unsigned char		byte;
typedef unsigned long long	uint64;
typedef unsigned			uint32;
typedef unsigned short		uint16;



/***********************************************************************\
							Common macro
\***********************************************************************/

#ifndef MAX
#define MAX(a,b)        (((a) > (b)) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b)        (((a) < (b)) ? (a) : (b))
#endif

#define ABS(x)          (((x)>=0) ? (x) : -(x))

/*
 ******************************************************************
 Cyclic adding, subtraction
 Note: a caller must check that x1<base & x2<=base
 ******************************************************************
*/
#define CYCLIC_ADD(res,x1,x2,base)  { res = (x1+x2); if (res>=base) res-=base; }

/*
 ******************************************************************
 Cyclic increment and decrement x with respect to base
 ******************************************************************
*/
#define CYCLIC_INC(x,base)      x = ((x)<(base-1)) ? (x+1) : 0
#define CYCLIC_DEC(x,base)      x = (x)            ? (x-1) : (base-1)



/***********************************************************************\
							 ASSERT & VERIFY
\***********************************************************************/

#define VERIFY(x,retval)	{ if (!(x)) { OutputDebugString("Verify failure :"#x); return retval; } }
#define VERIFY_(x)			{ if (!(x)) { OutputDebugString("Verify failure :"#x); return;		  } }
#define VERIFY__(x)			{ if (!(x)) { OutputDebugString("Verify failure :"#x);				  } }

#ifdef _DEBUG
    #define ASSERT(x,retval)	{ if (!(x)) { OutputDebugString("Assert failure :"#x); return retval; } }
	#define ASSERT_(x)			{ if (!(x)) { OutputDebugString("Assert failure :"#x); return;		  } }
	#define ASSERT__(x)			{ if (!(x)) { OutputDebugString("Assert failure :"#x);				  } }
#else
    #define ASSERT(x,retval)
	#define ASSERT_(x)	
	#define ASSERT__(x)
#endif

#define VERIFY_f(x)     VERIFY(x,false)
#define ASSERT_f(x)     ASSERT(x,false)


#endif // _DEF_H
