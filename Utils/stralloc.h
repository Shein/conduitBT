/******************************************************************************\
 Library     :  Utils
 Filename    :  stralloc.h
 Purpose     :  Global string allocator
 Author      :  Sergey Krasnitsky
 Created     :  3.9.2009
\******************************************************************************/

#ifndef _STRALLOC_H
#define _STRALLOC_H


enum {
    STRALLOC_MAXSTRS =  20,
    STRALLOC_MAXLEN  = 200
};


char* strallocGet();


#endif // _STRALLOC_H
