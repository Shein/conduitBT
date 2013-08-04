/**********************************************************************\
 Library     :  Utils
 Filename    :  enums_impl.h
 Purpose     :  Application's enumerators implementation macros.
 Author      :  Krasnitsky Sergey
\**********************************************************************/

#ifndef _ENUMS_IMPL_H
#define _ENUMS_IMPL_H

#undef  ENUM_ENTRY
#define ENUM_ENTRY  ENUM_ENTRY_IMPL


#define GetStringArraySize(array)   (sizeof(array) / sizeof (char*))


#define IMPL_ENUM(TNAME,ENLIST)                                                 \
    const char * enumTable_##TNAME [] = { ENLIST };                             \
    int enumTable_##TNAME##Size = GetStringArraySize(enumTable_##TNAME);



#endif // _ENUMS_IMPL_H
