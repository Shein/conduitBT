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
typedef unsigned long long	int64;
typedef int					int32;
typedef short				int16;
typedef signed char			int8;
typedef unsigned long long	uint64;
typedef unsigned			uint32;
typedef unsigned short		uint16;
typedef unsigned char		uint8;



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

#define VERIFY(x,retval)		{ if (!(x)) { ::LogMsg("Verify failure :"#x); return retval; } }
#define VERIFY_(x)				{ if (!(x)) { ::LogMsg("Verify failure :"#x); return;		 } }
#define VERIFY__(x)				{ if (!(x)) { ::LogMsg("Verify failure :"#x);				 } }

#ifdef _DEBUG
    #define ASSERT(x,retval)	{ if (!(x)) { ::LogMsg("Assert failure :"#x); return retval; } }
	#define ASSERT_(x)			{ if (!(x)) { ::LogMsg("Assert failure :"#x); return;		 } }
	#define ASSERT__(x)			{ if (!(x)) { ::LogMsg("Assert failure :"#x);				 } }
#else
    #define ASSERT(x,retval)
	#define ASSERT_(x)	
	#define ASSERT__(x)
#endif

#define VERIFY_f(x)     VERIFY(x,false)
#define VERIFY_0(x)     VERIFY(x,0)

#define ASSERT_f(x)     ASSERT(x,false)
#define ASSERT_0(x)     ASSERT(x,0)



/***********************************************************************\
                        Some short standard functions
\***********************************************************************/

#define vsnprintf _vsnprintf
#define snprintf  _snprintf

inline int uint32toa (uint32 val, char *str)  { return sprintf(str,"%u",val);  }
inline int uint16toa (uint16 val, char *str)  { return sprintf(str,"%u",val);  }
inline int uint8toa  (uint8  val, char *str)  { return sprintf(str,"%u",val);  }
inline int int32toa  (int32  val, char *str)  { return sprintf(str,"%d",val);  }
inline int int16toa  (int16  val, char *str)  { return sprintf(str,"%d",val);  }
inline int int8toa   (int8   val, char *str)  { return sprintf(str,"%d",val);  }


#endif // _DEF_H
