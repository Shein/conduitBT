/**********************************************************************\
 Library     :  Utils
 Filename    :  str.h
 Purpose     :  Class for manipulating with text strings.
 Author      :  Sergey Krasnitsky
 Created     :  1.02.2005
\**********************************************************************/

#ifndef _STR_H
#define _STR_H


#include "def.h"



/*
 *********************************************************************
 Base string class. It's the wrapper for all standard string manipulation 
 functions. There is no string length check.

 Class STRB defines 3 operators:
    1. =  copy operator
    2. += concatenation operator
    3. +  multiple concatenation operator 
    4. ==, >, < comparison operators

  - Concatenation execution time is much smaller than that of strncat
  - Len member execution time is much smaller than that os strlen
  - Operator char* returns the pointers to string
  - Operator int converts string to integer
 *********************************************************************
*/
class STRB
{
  public:

    enum { 
        STR2BYTES_MAX = 32 // Max supported byte array size for str2bytes() function
    };

    static bool isdigit (char c) { return (c >= '0' && c <= '9'); }
    static bool ishexdigit (char c) { return (isdigit(c) || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f')); }
    static char tolower (char c) { return (c >= 'A' && c <= 'Z') ? c-'A'+'a' : c; }
    static char hex2char (char h1);
    static char hex2char (char h1, char h2);
    static bool IsBlank  (cchar * str);

    // Convert hex byte sequence like "11:22:AA:BB:CC" or "1122AABBCC" to byte array
    // return number of read characters from input str
    // the digits in output byte array is right aligned
    static int str2bytes (cchar * str, uint8 outbytes[], int outsize);

    // Convert a byte array to a string.
    static int bytes2str (uint8 bytes[], int nbytes, char *strout, bool usecolon = true);

  public:

    STRB (char * buf, int maxlen = 0)    { Construct(buf,maxlen); }
    STRB (cchar * buf, int maxlen = 0)   { Construct((char*)buf,maxlen); }
    STRB (int maxlen = 0)              : Str(0), Spos(0), MaxLen(maxlen), Len(0)   {}

    void  Construct (char * buf, int maxlen = 0, int len = 0)   { Str = Spos = buf; MaxLen = maxlen-1; Len = len; }
    void  Set       (char * buf)                 { Str = Spos = buf;  }
    void  Set       (cchar * buf)                { Str = Spos = (char*) buf;  }
    void  Set       (int len)                    { Str = Spos = &Str[len];  }
    void  SetLen    (int len)                    { Len = len; }
    void  IncLen    (int len)                    { Len += len; }
    void  SetToSeek ()                           { Str = Spos; Len=0; }
    void  SetToEnd  ()                           { Str = Spos = &Str[Len]; Len=0; }
    char* GetStr    ()                           { return Str; }
    int   GetMaxLen ()                           { return MaxLen; }

  public:

    operator void* ()    const { return Str; }
    operator char* ()    const { return Str; }
    operator uint8* ()   const { return (uint8*)Str; }
    operator int ()      const { return atoi(Str); }

    char& operator[] (int i)  { return Str[i]; }

    bool   Valid() { return (Str != 0); }

    void   Clear() { Str[Len=0] = '\0'; }

    int    Copy (cchar * s);
    int    Copy (cchar * s, int size);
    int    Copy (cchar * s, char terminator) { Len = 0; return Add(s,terminator); };

    int    Add  (cchar * s);
    int    Add  (cchar * s, int size);
    int    Add  (cchar * s, char terminator);

    int    CopyEx(cchar * s, int size = 0) { Len = 0; return AddEx(s,size); }
    int    AddEx(cchar * s, int size = 0);

    bool   Comp (cchar * s)         const { return s ? (Strcmp(s) == 0) : false;  }
    bool   Comp (const STRB & c)    const { return Comp((char*)c);  }

    bool   CompEx (cchar * s,      int * n = 0);
    bool   CompEx (const STRB & c, int * n = 0) const { return CompEx ((char*)c, n); }

    bool   operator == (cchar * s)  const { return Comp(s);  }
    bool   operator != (cchar * s)  const { return !Comp(s); }

    bool   operator == (const STRB & c)  const { return Comp((char*)c);  }
    bool   operator != (const STRB & c)  const { return !Comp((char*)c); }

    bool   operator <  (const STRB & c)  const { return Strcmp((char*)c) < 0;  }
    bool   operator >  (const STRB & c)  const { return Strcmp((char*)c) > 0;  }

    STRB & operator =  (cchar * s) { Copy(s); return *this; }
    STRB & operator += (cchar * s) { Add(s);  return *this; }
    STRB & operator +  (cchar * s) { return operator += (s); }

    STRB & operator =  (const STRB & c) { return operator =  ((char*)c); }
    STRB & operator += (const STRB & c) { return operator += ((char*)c); }
    STRB & operator +  (const STRB & c) { return operator += (c); }

    STRB & operator =  (char ch);
    STRB & operator += (char ch);
    STRB & operator +  (char ch)  { return operator += (ch);  }

    STRB & operator =  (int i);
    STRB & operator += (int i);
    STRB & operator +  (int i) { return operator += (i); }

    STRB & operator =  (unsigned i);
    STRB & operator += (unsigned i);
    STRB & operator +  (unsigned i) { return operator += (i); }

	STRB & operator =  (UINT64 i);
	STRB & operator += (UINT64 i);
	STRB & operator +  (UINT64 i) { return operator += (i); }

    STRB & operator =  (const wchar_t * s);

    // Cut off i last chars
    STRB & operator -= (int i);

    int   Length()    const   { return Len; }
    char* End ()      const   { return &Str[Len]; }
    char  Lastchr()   const   { return  Str[Len-1]; }


    //********************************************************************
    // Init search position
    void  SeekStart   ()                { Spos = Str; }
    void  SeekStart   (int abspos)      { Spos = &Str[abspos]; }

    //********************************************************************
    // Standard string functions wrappers
    int   Strlen      ()                { return (Len = (unsigned) strlen(Str)); }
    STRB& Strcpy      (cchar* s)        { strcpy(Str,s);  return *this; }
    STRB& Strcat      (cchar* s)        { strcat(Str,s);  return *this; }
    char* Strstr      (cchar* s)        { return strstr(Str,s);  }
    int   Strcmp      (cchar* s)        const { return strcmp(Str,s); }
    int   Strncmp     (cchar* s, int n) const { return strncmp(Str,s,n); }
    char* Strchr      (char ch)         { return strchr(Str,ch); }
    char* Strrchr     (char ch)         { return strrchr(Str,ch); }
    int   Sprintf     (cchar * sformat, ...);
    int   Vsprintf    (cchar * sformat, va_list arglist);
    char* Strlwr ();

    //********************************************************************
    // Get seek/scan positions
    char* GetSeekPos  () const          { return Spos; }
    char  GetSeekChar () const          { return (*Spos); }
    int   GetSeekOffset() const         { return (int) (Spos - Str); }
    void  SetSeekPos  (char *spos)      { Spos = spos; }

    //********************************************************************
    // Skip methods skip some characters from the current position 
    char* Skip        (int numch = 1)    { return Spos += numch; }
    void  SkipChars   (char ch)          { while(*Spos==ch) ++Spos; }
    void  SkipSpaces  ()                 { SkipChars(' '); }

    //********************************************************************
    // Seek methods look for characters next to the current position 
    char  SeekChar    ()                    { return (*Spos++); }
    char* SeekChar    (char ch)             { return (*Spos==ch) ? ++Spos : 0; }
    char* SeekStr     (cchar* s)            { int l; return (memcmp(Spos,s,l=(int)strlen(s))==0) ? Spos+=l : 0; }
    char* SeekStr     (STRB & s)            { int l; return (memcmp(Spos,(void*)s,l=s.Length())==0) ? Spos+=l : 0; }
    char* SeekStr     (cchar* s, int slen)  { return (memcmp(Spos,s,slen)==0) ? Spos+=slen : 0; }
    char* SeekInt     (int * x, cchar* sformat = "%d");

    //********************************************************************
    // Scan methods look for characters after the current position
    char* ScanChar    (char ch)          { char * s1 = strchr(Spos,ch);    return (s1) ? Spos=s1 : 0; }
    char* ScanCharr   (char ch)          { char * s1 = strrchr(Spos,ch);   return (s1) ? Spos=s1 : 0; }
    char* ScanCharNext(char ch)          { char * s1 = strchr(Spos+1,ch);  return (s1) ? Spos=s1 : 0; }
    char* ScanCharrNext(char ch)         { char * s1 = strrchr(Spos+1,ch); return (s1) ? Spos=s1 : 0; }
    char* ScanStr     (cchar* s)         { char * s1 = strstr(Spos,s);     return (s1) ? Spos=s1 : 0; }
    char* ScanStrNext (cchar* s)         { char * s1 = strstr(Spos+1,s);   return (s1) ? Spos=s1 : 0; }
    char* ScanStrNextSkip (cchar* s);
    char* ScanStrSkip (cchar* s);
    char* ScanStrSkip (STRB & s);

    //********************************************************************
    // Other useful methods
    void  PrnTime (unsigned seconds);
    int   GetTime ();
    int   PrnSetEnd   (cchar * sformat, ...);
    int   CopySetEnd  (cchar * s);
    int   CopySetEnd  (cchar * s, int size);
    void  TruncateBgn (unsigned len);
    void  TruncateEnd (unsigned len);
    char* TrimSpace ();
    char* ToLower ();

  protected:
    bool  IsSizeExceeded (int size)   { return (MaxLen && (unsigned(size) >= MaxLen)); }
    bool  IsLenExceeded ()            { return (MaxLen && (Len >= MaxLen)); }
    bool  IsLenExceeded (int addlen)  { return (MaxLen && ((Len+addlen)>= MaxLen)); }
    
  protected:
    char     * Str, * Spos;
    unsigned   MaxLen, Len;
};


/*
 **************************************************************************
 String template derived from the string base class. Contains the string
 buffer of user-defined size.
 **************************************************************************
*/
template <unsigned MaxSize> class STR : public STRB
{
  public:
    enum { SIZE = MaxSize };

    STR ()              : STRB(StrBuf,MaxSize)  { StrBuf[MaxSize] = '\0'; Clear(); }
    STR (cchar * sinit) : STRB(StrBuf,MaxSize)  { StrBuf[MaxSize] = '\0'; STRB::operator = (sinit); }
	STR (STR&  sinit)   : STRB(StrBuf,MaxSize)  { StrBuf[MaxSize] = '\0'; STRB::operator = (sinit); }

    void Construct () { STRB::Construct(StrBuf,MaxSize); }
    void SetToBegin() { Set(StrBuf); }

	template <class T>
    STR & operator =  (T s)      { return *((STR*) &STRB::operator =  (s)); }
	template <class T>
    STR & operator += (T s)      { return *((STR*) &STRB::operator += (s)); }
	template <class T>
    STR & operator +  (T s)      { return *((STR*) &STRB::operator +  (s)); }

/*
    STR & operator =  (const STRB & c) { return *((STR*) &STRB::operator =  (c)); }
    STR & operator += (const STRB & c) { return *((STR*) &STRB::operator += (c)); }
    STR & operator +  (const STRB & c) { return *((STR*) &STRB::operator +  (c)); }

    STR & operator =  (cchar c)        { return *((STR*) &STRB::operator =  (c)); }
    STR & operator += (cchar c)        { return *((STR*) &STRB::operator += (c)); }
    STR & operator +  (cchar c)        { return *((STR*) &STRB::operator +  (c)); }

    STR & operator =  (int i)          { return *((STR*) &STRB::operator =  (i)); }
    STR & operator += (int i)          { return *((STR*) &STRB::operator += (i)); }
    STR & operator +  (int i)          { return *((STR*) &STRB::operator +  (i)); }
 
    STR & operator =  (unsigned i)     { return *((STR*) &STRB::operator =  (i)); }
    STR & operator += (unsigned i)     { return *((STR*) &STRB::operator += (i)); }
    STR & operator +  (unsigned i)     { return *((STR*) &STRB::operator +  (i)); }

	STR & operator =  (UINT64 i)     { return *((STR*) &STRB::operator =  (i)); }
	STR & operator += (UINT64 i)     { return *((STR*) &STRB::operator += (i)); }
	STR & operator +  (UINT64 i)     { return *((STR*) &STRB::operator +  (i)); }*/


    STR & operator =  (const wchar_t * s) { return *((STR*) &STRB::operator + (s)); }

    STR & operator -= (int i)          { return *((STR*) &STRB::operator -= (i)); }

  protected:
    char StrBuf [SIZE+1];   // Buffer for one string
};


/*
 **************************************************************************
 String class dedicated for a file path.
 **************************************************************************
*/
typedef  STR<100>  SPATHBASE;

class SPATH : public SPATHBASE
{
  public:
    SPATH () : SPATHBASE(), Path(0), Fname(0), Fext(0) {};
    SPATH (cchar * fullpath);
    SPATH (SPATH & path);
    SPATH (cchar * path, cchar * file, cchar * ext = 0)  { Merge(path,file,ext); }

    STRB& operator = (cchar* s)          { return STRB::operator= (s); }
    STRB& operator = (const STRB &s)     { return STRB::operator= (s); }
    STRB& operator = (const wchar_t * s) { return STRB::operator= (s); }

    char* FullPath()  { return (char*)(*this); }

    bool IsParsed () { return (Path!=0); }

    bool Parse (bool separate_ext = true);
    void Merge (cchar * path, cchar * file, cchar * ext = 0);
    bool ReplaceBgn (cchar * swhat, cchar* swith);

  public:
    char *Path;
    char *Fname;
    char *Fext;
    
  protected:
    char StrBuf2 [SIZE+1];   // Additional buffer for one string: Path, Fname & Fext will point to it
};


/*
 **************************************************************************
 Predefined type for use as one line string. 
 The SLINE::SIZE will get the line buffer size. 
 **************************************************************************
*/
typedef  STR<256>  SLINE;


/*
 **************************************************************************
 Predefined type for use as one string. 
 The STRING::SIZE will get the line buffer size. 
 **************************************************************************
*/
typedef  STR<100>  STRING;



inline int STRB::Copy (cchar * s, int size)
{
    if (IsSizeExceeded(size))
        return 0;
    memcpy (Str, s, size);
    Str [Len = size] = '\0';
    return size;
}


inline int STRB::Add (cchar * s, int size)
{
    if (IsLenExceeded(size))
        return 0;
    memcpy (Str+Len, s, size);
    Str [Len+=size] = '\0';
    return size;
}

inline STRB & STRB::operator = (cchar ch)
{
    Str[0] = ch;
    Str[1] = '\0';
    Len = 1;
    return * this;
}


inline STRB & STRB::operator += (cchar ch)
{
    Str[Len] = ch;
    Str[++Len] = '\0';
    return * this;
}


inline STRB & STRB::operator = (int i)
{
    char s[20];
    int32toa (i, s);
    operator=(s);
    return * this;
}


inline STRB & STRB::operator -= (int i)
{
    if (Len >= unsigned(i))
        Str[Len-=i] = '\0';
    return * this;
}


inline void STRB::TruncateEnd (unsigned len)
{
    if (len < Len)
        Str[Len=len] = '\0';
}


inline void STRB::TruncateBgn (unsigned len)
{
	Str += len;
	Len -= len;
}


inline int STRB::Vsprintf (cchar * sformat, va_list arglist)
{
	int ret = vsprintf (Str, sformat, arglist);
	return (Len = ret);
}


/**********************************************************************\
					Temporary taken from str.cpp
\**********************************************************************/

inline int STRB::Sprintf (cchar * sformat, ...)
{
	va_list  argptr;
	va_start (argptr, sformat);
	int ret = vsprintf (Str, sformat, argptr);
	va_end (argptr);
	return (Len = ret);
}


inline int STRB::Copy (cchar * s)
{
    Len = 0;
    if (s) {
        for (; s[Len]; Len++) {
            if (IsLenExceeded())
                break;
            Str[Len] = s[Len];
        }
    }
    Str[Len] = '\0';
    return Len;
}


inline char* STRB::SeekInt (int * x, cchar * sformat)
{
    // sformat must contain only one %d occurrence
    if (sscanf(Spos, sformat, x) != 1) {
        *x = 0;
        return 0;
    }
    
    // now calculate number of digits in *x
    STR<16> s;
    s = *x;

    return Spos += strlen(sformat) - 2 /* %d */ + s.Strlen();
}



#endif // _STR_H
