/******************************************************************************\
 Library     :  Utils
 Filename    :  stralloc.cpp
 Purpose     :  Global string allocator
 Author      :  Sergey Krasnitsky
 Created     :  3.9.2009
\******************************************************************************/

#include "def.h"
#include "fifo_cse.h"
#include "stralloc.h"



struct STRALLOC_ST
{
    char str[STRALLOC_MAXLEN];
};



FIFO_SIMPLE_ALLOC<STRALLOC_ST,STRALLOC_MAXSTRS>  strallocCyclicBuffer;



char* strallocGet()
{
    return strallocCyclicBuffer.FetchNext()->str;
}
