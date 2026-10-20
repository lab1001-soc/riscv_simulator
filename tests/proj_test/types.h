#ifndef _TYPES_H_
#define _TYPES_H_

//#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef enum
{
	RAM_UseBTSRam=0,
	RAM_UseM0MRam,
}RAM_RegionUse;

#ifndef _BYTE_T_DECLARED
typedef unsigned char       byte;
#define _BYTE_T_DECLARED
#endif
#ifndef _WORD_T_DECLARED
typedef unsigned short      word;
#define _WORD_T_DECLARED
#endif

#if 1
#ifndef _UINT8_T_DECLARED
typedef unsigned char 		uint8_t;
#define _UINT8_T_DECLARED
#endif
#ifndef _UINT16_T_DECLARED
typedef unsigned short  	uint16_t;
#define _UINT16_T_DECLARED
#endif
#ifndef _UINT32_T_DECLARED
typedef unsigned int 		uint32_t;
#define _UINT32_T_DECLARED
#endif
#ifndef _UINT64_T_DECLARED
typedef unsigned long long	uint64_t;
#define _UINT64_T_DECLARED
#endif
#ifndef _BYTE_T_DECLARED
typedef unsigned char       byte;
#define _BYTE_T_DECLARED
#endif
#ifndef _WORD_T_DECLARED
typedef unsigned short      word;
#define _WORD_T_DECLARED
#endif
#ifndef _UINT_T_DECLARED
typedef unsigned int uint;
#define _UINT_T_DECLARED
#endif
#ifndef _ULL_T_DECLARED
typedef unsigned long long ull;
#define _ULL_T_DECLARED
#endif

#ifndef _INT8_T_DECLARED
typedef signed char 		int8_t;
#define _INT8_T_DECLARED
#endif
#ifndef _INT16_T_DECLARED
typedef signed short  	    int16_t;
#define _INT16_T_DECLARED
#endif
#ifndef _INT32_T_DECLARED
typedef signed long 		int32_t;
#define _INT32_T_DECLARED
#endif
#ifndef _INT64_T_DECLARED
typedef signed long long	int64_t;
#define _INT64_T_DECLARED
#endif


#ifndef BOOL
typedef unsigned char 		BOOL;
#endif

#ifndef bool
typedef enum {false = 0, true =1} bool;
#endif

#endif

#define     __O     volatile                  /*!< defines 'write only' permissions     */
#define     __IO    volatile                  /*!< defines 'read / write' permissions   */
#define     __I    	const volatile            /*!< defines 'read only' permissions   */

#ifndef Boolean
#ifndef IS_BOOLEAN
typedef enum {FALSE = 0, TRUE =1} Boolean;
#define IS_BOOLEAN(bool) ((bool == FALSE) || (bool == TRUE))
#endif
#endif


#ifndef FunctionalState
#ifndef IS_FUNCTIONAL_STATE
typedef enum {DISABLE = 0, ENABLE =1} FunctionalState;
#define IS_FUNCTIONAL_STATE(state) ((state== DISABLE) || (state == ENABLE))
#endif
#endif

#ifndef FunctionalState
typedef enum {ERROR = 0, SUCCESS = 1} ErrorStatus;
#define IS_ERROR_STATE(status) ((status== ERROR) || (status == SUCCESS))
#endif

#ifndef FlagStatus
#ifndef IS_FLAG_STATUS_RESET
typedef enum {RESET = 0, SET = !RESET} FlagStatus, ITStatus;
#define IS_FLAG_STATUS_RESET(state) ((state== RESET) || (state == RESET))
#endif
#endif





#endif //_YC11XX_H

