/****************************************************************************************\
 Library     :  Utils
 Filename    :  fifo_cse.h
 Purpose     :  FIFO manager for Constant Size Elements
 Author      :  Sergey Krasnitsky
\****************************************************************************************/

#ifndef _FIFO_CSE_H
#define _FIFO_CSE_H

#include "def.h"

#define LOCKFUNC_FIFO()


/*
 **************************************************************************
 FIFO_SIMPLE_ALLOC template implements a simple cyclic buffer of one-type 
 elements with one pointer (elements array is part of the class).
 FetchNext() is always returns non-null pointer.
 **************************************************************************
*/
template <class T, int MaxElements> class FIFO_SIMPLE_ALLOC
{
  public:
	enum { Size = MaxElements };

  public:

    FIFO_SIMPLE_ALLOC ()  { Construct (); }

    void Construct ()       { NextInputInd = 0; }
    void Clear ()           { Construct ();  memset (Addr,0,sizeof(Addr)); }

    int  GetMaxElements()   { return MaxElements; }
    T *  GetNextFree()      { return & Addr [NextInputInd]; }
    T *  Element(int i)     { return & Addr [i]; }
    void IncrementNextInput() { CYCLIC_INC (NextInputInd, MaxElements); }
    int  GetNextInputInd()  { return NextInputInd; }

    T * FetchNext ()
    {
        T * ret = & Addr [NextInputInd];
        IncrementNextInput();
        return ret;
    }

    T * FetchNext (int count)
    {
        // Get count contiguous elements
        if (MaxElements-1 - NextInputInd >= count) {
            T * ret = & Addr [NextInputInd];
            NextInputInd += count;
            return ret;
        }
        NextInputInd = count;
        return &Addr[0];
    }

  public:
    T    Addr [MaxElements];

  protected:
    int  NextInputInd;
};



/*
 **************************************************************************
 Standard FIFO implementing a cyclic buffer with two pointers containing 
 one-type elements.
 **************************************************************************
*/
template <class T> class FIFO
{
  public:

    void Construct (T * addr, int maxelements)
    {
        Addr = addr;
        MaxElements = maxelements;
        Clear ();
    }

    void Construct ()
    {
        Clear ();
    }

    void Clear ()
    {
        Full = false;
        Empty = true;
        NextInputInd = FirstOutputInd = 0;
    }

    int  GetMaxElements() { return MaxElements; }
    T *  Element(int i)   { return & Addr[i]; }
    bool IsEmpty ()       { return Empty; }

    int GetCount ()
    {
        if (Empty)
            return 0;

        if (NextInputInd > FirstOutputInd)
            return NextInputInd - FirstOutputInd;

        return NextInputInd + MaxElements - FirstOutputInd;
    }

    T * GetNextFree()
    {
        if (Full)
            return 0;
        return & Addr[NextInputInd];
    }

    T * FetchNext ()
    {
        LOCKFUNC_FIFO();
        if (Full)
            return 0;

        T* pRetElem = & Addr[NextInputInd];
        IncrementNextInput();
        return pRetElem;
    }

    bool PutElement (T new_element)
    {
        LOCKFUNC_FIFO();
        if (Full)
            return false;

        Addr[NextInputInd] = new_element;
        IncrementNextInput();
        return true;
    }

    T * GetFirst ()
    {
        if (Empty)
            return 0;
        return & Addr[FirstOutputInd];
    }

    T GetElement ()
    {
        LOCKFUNC_FIFO();
        if (Empty)
            return (T) 0;

        T ret = Addr[FirstOutputInd];
        IncrementFirstOutput();
        return ret;
    }

    bool ReleaseFirst ()
    {
        LOCKFUNC_FIFO();
        if (Empty)
            return false;
        IncrementFirstOutput();
        return true;
    }

    void IncrementNextInput ()
    {
        CYCLIC_INC (NextInputInd, MaxElements);
        Empty = false;

        if (NextInputInd == FirstOutputInd) 
            Full = true;
    }

  protected:
    void IncrementFirstOutput ()
    {
        CYCLIC_INC (FirstOutputInd, MaxElements);
        Full = false;

        if (NextInputInd == FirstOutputInd) 
            Empty = true;
    }

  protected:
    T  * Addr;
    int  MaxElements;

  private:
    bool  Full, Empty;
    int   NextInputInd, FirstOutputInd;
};



/*
 **************************************************************************
 FIFO_ALLOC template implements a cyclic buffer with two pointers 
 containing one-type elements.
 **************************************************************************
*/
template <class T, int MaxElements_> class FIFO_ALLOC : public FIFO<T>
{
  public:
	enum { Size = MaxElements_ };

  public:
    FIFO_ALLOC ()		{ Construct (); }
    void Construct ()	{ FIFO<T>::Construct (Addr, MaxElements_); }

  public:
    T  Addr [MaxElements_];
};



#endif // _FIFO_CSE_H
