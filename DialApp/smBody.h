/****************************************************************************\
 Filename    :  SmBody.h
 Purpose     :  Redefining SM State macro
 Note        :  Include this file as a last include to a specific SM cpp-file
\****************************************************************************/

#ifdef STATE
#undef STATE
#endif

#define STATE  STATE_STR
