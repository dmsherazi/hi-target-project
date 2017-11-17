/********************************************************************
 *                Copyright Simcom(shanghai)co. Ltd.                               *
 *---------------------------------------------------------------------
 * FileName      :   eat_nvram.h
 * version       :   0.10
 * Description   :   eat wmmp head files
 * Authors       :   Laiwenjie  
 * Notes         :
 *---------------------------------------------------------------------
 *
 *    HISTORY OF CHANGES
 *---------------------------------------------------------------------
 *0.10  2013-01-11, laiwenjie, create originally.
 *
 ********************************************************************/
#ifndef _eat_nvram_
#define _eat_nvram_
/********************************************************************
 * Include Files
 ********************************************************************/

/********************************************************************
 * Macros
 ********************************************************************/

/********************************************************************
 * Types
 ********************************************************************/

/********************************************************************
 * Extern Variables (Extern /Global)
 ********************************************************************/

/********************************************************************
 * Local Variables:  STATIC
 ********************************************************************/

/********************************************************************
 * External Functions declaration
 ********************************************************************/

/********************************************************************
 * Local Function declaration
 ********************************************************************/
extern eat_bool(* const eat_nvram_write)(u16 u16Para,void* pBuffer,u16 u16Len);
extern eat_bool(* const eat_nvram_read)(u16 u16Para,void* pBuffer,u16 u16Len);
/********************************************************************
 * Local Function
 ********************************************************************/
#endif // _eat_nvram_
