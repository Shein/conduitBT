/**********************************************************************\
 Filename    :  enums.h
 Purpose     :  Application's enumerators.
 Author      :  Krasnitsky Sergey
\**********************************************************************/

#ifndef _ENUMS_H
#define _ENUMS_H


#define ENUM_ENTRY_DECL(eprefix,ename)              eprefix##_##ename
#define ENUM_ENTRY_IMPL(eprefix,ename)              #ename

#define ENUM_ENTRY   ENUM_ENTRY_DECL



#define DECL_ENUM(TNAME,ENLIST)                                 \
    typedef enum                                                \
    {                                                           \
        ENLIST,                                                 \
        TNAME##_NUMS,                                           \
        TNAME##_NONE = -1                                       \
    } TNAME;                                                    \
                                                                \
    extern const char *enumTable_##TNAME [];                    \
    extern int   enumTable_##TNAME##Size;                       \
                                                                \
    const char* enum_##TNAME (int val);                      


#endif /* _ENUMS_H */
